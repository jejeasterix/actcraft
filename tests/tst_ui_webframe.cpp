// Cadre web (L8-T30, S27) : chaque requête du cadre passe par la politique (L8-T29), sans moteur web.
//
// Le modèle des cadres va chercher la page lui-même après la décision, suit les redirections une à une et
// réécrit le HTML avant de le donner à la vue : ces tests le vérifient sans moteur, par un serveur local qui
// compte les requêtes. Une règle posée une fois dans le QML de la vue (adresse donnée au moteur) échouerait
// ici sans moteur (le serveur ne voit rien) comme avec moteur (la sous-ressource ou la redirection refusée
// atteint le second serveur).

#include "actcraft/ui/documentcontroller.h"
#include "actcraft/ui/webframeitem.h"
#include "actcraft/ui/webpolicy.h"

#include <actcraft/core/document.h>
#include <actcraft/render/pagepainter.h>

#include <QByteArray>
#include <QElapsedTimer>
#include <QGuiApplication>
#include <QHash>
#include <QHostAddress>
#include <QImage>
#include <QPointer>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QQuickItem>
#include <QQuickWindow>
#include <QRegularExpression>
#include <QSignalSpy>
#include <QStandardPaths>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTemporaryDir>
#include <QtTest>

#include <algorithm>
#include <functional>
#include <memory>

#if ACTCRAFT_WEBFRAME
#include <QtWebView/QtWebView>
#endif

using namespace actcraft;
using Verdict = ui::WebContentPolicy::Verdict;

namespace {

// Serveur HTTP d'essai sur 127.0.0.1, port libre : compte les requêtes, sert une page ou une redirection.
class TestServer : public QObject
{
public:
    TestServer()
    {
        connect(&m_server, &QTcpServer::newConnection, this, [this] { accept(); });
        m_listening = m_server.listen(QHostAddress::LocalHost, 0);
    }
    bool listening() const { return m_listening; }
    QUrl url(const QString& path) const
    {
        return QUrl(QStringLiteral("http://127.0.0.1:%1%2").arg(m_server.serverPort()).arg(path));
    }
    QString origin() const { return QStringLiteral("127.0.0.1:%1/").arg(m_server.serverPort()); }
    void page(const QString& path, const QByteArray& html) { m_pages.insert(path, html); }
    void redirect(const QString& path, const QUrl& to) { m_redirects.insert(path, to); }
    int requests() const { return static_cast<int>(m_paths.size()); }
    QStringList paths() const { return m_paths; }
    void reset() { m_paths.clear(); }

private:
    void accept()
    {
        while (QTcpSocket* socket = m_server.nextPendingConnection()) {
            connect(socket, &QTcpSocket::disconnected, socket, &QObject::deleteLater);
            connect(socket, &QTcpSocket::readyRead, this, [this, socket] { read(socket); });
        }
    }
    void read(QTcpSocket* socket)
    {
        QByteArray& buffer = m_buffers[socket];
        buffer += socket->readAll();
        if (!buffer.contains("\r\n\r\n"))
            return;
        const QList<QByteArray> line = buffer.left(buffer.indexOf("\r\n")).split(' ');
        m_buffers.remove(socket);
        const QString path = line.size() > 1 ? QString::fromLatin1(line.at(1)) : QString();
        m_paths.append(path);
        QByteArray response;
        if (m_redirects.contains(path)) {
            response = "HTTP/1.1 302 Found\r\nLocation: " + m_redirects.value(path).toEncoded() +
                       "\r\nContent-Length: 0\r\nConnection: close\r\n\r\n";
        } else if (m_pages.contains(path)) {
            const QByteArray body = m_pages.value(path);
            response = "HTTP/1.1 200 OK\r\nContent-Type: text/html; charset=utf-8\r\nContent-Length: " +
                       QByteArray::number(body.size()) + "\r\nConnection: close\r\n\r\n" + body;
        } else {
            response = "HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\nConnection: close\r\n\r\n";
        }
        socket->write(response);
        socket->disconnectFromHost();
    }

    QTcpServer m_server;
    bool m_listening = false;
    QHash<QString, QByteArray> m_pages;
    QHash<QString, QUrl> m_redirects;
    QHash<QTcpSocket*, QByteArray> m_buffers;
    QStringList m_paths;
};

QVariant role(const ui::WebFrameItems& items, int row, int role)
{
    return items.data(items.index(row, 0), role);
}

QString content(const ui::WebFrameItems& items, int row)
{
    return role(items, row, ui::WebFrameItems::ContentRole).toString();
}

// Document de la session, cadres branchés.
struct Fixture
{
    ui::DocumentController document;
    ui::WebFrameItems frames;
    Fixture()
    {
        document.newDocument(1024, 768);
        frames.setDocument(&document);
    }
};

// Enregistre le document de `from` en .actcraft puis le rouvre dans `to` : `to` tient un document reçu.
bool saveAndReopen(Fixture& from, Fixture& to, const QString& path)
{
    from.document.saveToUrl(QUrl::fromLocalFile(path));
    if (!QTest::qWaitFor([&] { return !from.document.isBusy(); }, 10000))
        return false;
    to.document.openUrl(QUrl::fromLocalFile(path));
    return QTest::qWaitFor([&] { return !to.document.isBusy() && to.document.fileUrl().isValid(); }, 10000);
}

} // namespace

class TstUiWebFrame : public QObject
{
    Q_OBJECT
private slots:
    void initTestCase();
    void subResourcesPassThroughThePolicy();
    void redirectPassesThroughThePolicy();
    void receivedRemoteFrameSendsNoRequest();
    void unblockThenRequestReachesTheServer();
    void otherPageStaysBlocked();
    void insertFromTypedHtml();
    void resizeIsOneStep();
    void snapshotPlacesAnImage();
    void roundTripKeepsTheFrame();
    void paintedAsANeutralFrame();
    void viewFetchesOnlyDecidedSubResources();
    void prepareHtmlLeavesNoUndecidedAddress_data();
    void prepareHtmlLeavesNoUndecidedAddress();
    void prepareHtmlKeepsDecidedAddresses();
    void prepareHtmlStaysLinearOn16MB();
    void engineNavigationAlwaysReturnsToTheModel();
    void receivedCodeFrameUnblocks();
};

namespace {

// Politique d'essai de prepareHtml : seul l'hôte ok.example passe ; chaque adresse demandée est notée.
struct OnlyOk
{
    QStringList asked;
    Verdict operator()(const QUrl& url)
    {
        asked.append(url.toString());
        return url.host() == QLatin1String("ok.example") ? Verdict::Load : Verdict::Refuse;
    }
};

// Ce que le moteur lirait comme adresses dans la sortie de prepareHtml : les balises et le contenu des
// feuilles de style (le texte n'est jamais demandé), entités numériques et échappements CSS rendus, en
// minuscules.
QString addressBearing(const QString& html)
{
    static const QRegularExpression tagRe(
        QStringLiteral(R"re(<[^>"']*(?:"[^"]*"[^>"']*|'[^']*'[^>"']*)*>)re"));
    static const QRegularExpression styleRe(QStringLiteral(R"re(<style\b[^>]*>(.*?)</style)re"),
                                            QRegularExpression::CaseInsensitiveOption |
                                                QRegularExpression::DotMatchesEverythingOption);
    static const QRegularExpression escapeRe(
        QStringLiteral(R"h(&#([xX]?)([0-9a-fA-F]+);?|\\([0-9a-fA-F]{1,6})\s?)h"));
    QString kept;
    for (auto it = tagRe.globalMatch(html); it.hasNext();)
        kept += it.next().captured(0) + QLatin1Char('\n');
    for (auto it = styleRe.globalMatch(html); it.hasNext();)
        kept += it.next().captured(1) + QLatin1Char('\n');
    QString out;
    qsizetype last = 0;
    for (auto it = escapeRe.globalMatch(kept); it.hasNext();) {
        const QRegularExpressionMatch m = it.next();
        out += QStringView(kept).mid(last, m.capturedStart() - last);
        bool ok = false;
        const char32_t code = m.capturedLength(3) > 0
                                  ? m.captured(3).toUInt(&ok, 16)
                                  : m.captured(2).toUInt(&ok, m.captured(1).isEmpty() ? 10 : 16);
        out += ok ? QString::fromUcs4(&code, 1) : m.captured(0);
        last = m.capturedEnd();
    }
    out += QStringView(kept).mid(last);
    return out.replace(QStringLiteral("&amp;"), QStringLiteral("&")).toLower();
}

} // namespace

void TstUiWebFrame::initTestCase()
{
    QStandardPaths::setTestModeEnabled(true);
}

// Le cœur de la tâche : la page saisie est lue par le modèle (le serveur la voit sans moteur), la
// sous-ressource d'une origine non saisie ne figure pas dans le HTML donné à la vue et n'est jamais demandée
// ; celle de l'origine saisie y reste, en adresse absolue, et la règle de sécurité du contenu n'autorise
// qu'elle.
void TstUiWebFrame::subResourcesPassThroughThePolicy()
{
    TestServer typed;
    TestServer other;
    QVERIFY(typed.listening() && other.listening());
    const QUrl spy = other.url(QStringLiteral("/spy.png"));
    typed.page(QStringLiteral("/page"), "<html><head><link rel=\"stylesheet\" href=\"" +
                                            other.url("/s.css").toEncoded() +
                                            "\"></head><body><p>distant</p><img src=\"/ok.png\"><img src=\"" +
                                            spy.toEncoded() + "\"></body></html>");
    Fixture f;
    QVERIFY(f.frames.insertFrame({}, typed.url(QStringLiteral("/page")).toString()));
    QCOMPARE(f.frames.count(), 1);
    QTRY_VERIFY_WITH_TIMEOUT(typed.requests() >= 1, 5000);
    QTRY_VERIFY_WITH_TIMEOUT(content(f.frames, 0).contains(QStringLiteral("distant")), 5000);
    const QString html = content(f.frames, 0);
    QVERIFY2(!html.contains(other.origin()), qPrintable(html));
    QVERIFY2(html.contains(typed.url(QStringLiteral("/ok.png")).toString()), qPrintable(html));
    QVERIFY2(html.contains(QStringLiteral("Content-Security-Policy")), qPrintable(html));
    QVERIFY2(html.contains(QStringLiteral("default-src 'none'")), qPrintable(html));
    QCOMPARE(role(f.frames, 0, ui::WebFrameItems::BaseUrlRole).toUrl(), typed.url(QStringLiteral("/page")));
    QTest::qWait(300);
    QCOMPARE(other.requests(), 0);
    QCOMPARE(typed.paths(), QStringList{QStringLiteral("/page")}); // ok.png : à la vue, pas au modèle
}

// Une redirection est une requête neuve : vers l'origine saisie, elle est suivie ; vers une autre, refusée
// avant d'être envoyée.
void TstUiWebFrame::redirectPassesThroughThePolicy()
{
    TestServer typed;
    TestServer other;
    QVERIFY(typed.listening() && other.listening());
    typed.redirect(QStringLiteral("/start"), typed.url(QStringLiteral("/final")));
    typed.page(QStringLiteral("/final"), "<p>arrivee</p>");
    typed.redirect(QStringLiteral("/away"), other.url(QStringLiteral("/landing")));
    other.page(QStringLiteral("/landing"), "<p>ailleurs</p>");

    Fixture f;
    QVERIFY(f.frames.insertFrame({}, typed.url(QStringLiteral("/start")).toString()));
    QTRY_VERIFY_WITH_TIMEOUT(content(f.frames, 0).contains(QStringLiteral("arrivee")), 5000);
    QCOMPARE(typed.paths(), (QStringList{QStringLiteral("/start"), QStringLiteral("/final")}));
    QCOMPARE(role(f.frames, 0, ui::WebFrameItems::BaseUrlRole).toUrl(), typed.url(QStringLiteral("/final")));

    QVERIFY(f.frames.insertFrame({}, typed.url(QStringLiteral("/away")).toString()));
    QCOMPARE(f.frames.count(), 2);
    QTRY_VERIFY_WITH_TIMEOUT(typed.paths().contains(QStringLiteral("/away")), 5000);
    QTRY_VERIFY_WITH_TIMEOUT(role(f.frames, 1, ui::WebFrameItems::RefusedRole).toBool(), 5000);
    QTest::qWait(300);
    QCOMPARE(other.requests(), 0);
    QVERIFY(!content(f.frames, 1).contains(QStringLiteral("ailleurs")));
}

// Document reçu dont le cadre vise le serveur : rien n'est demandé, le cadre est bloqué.
void TstUiWebFrame::receivedRemoteFrameSendsNoRequest()
{
    TestServer server;
    QVERIFY(server.listening());
    server.page(QStringLiteral("/page"), "<p>distant</p>");
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    Fixture writer;
    QVERIFY(writer.frames.insertFrame({}, server.url(QStringLiteral("/page")).toString()));
    // Cadre en code HTML qui vise le même serveur : dans le document reçu, repère et rien de demandé.
    QVERIFY(writer.frames.insertFrame(
        QStringLiteral("<p>code</p><img src=\"%1\">").arg(server.url(QStringLiteral("/img.png")).toString()),
        {}));
    QTRY_VERIFY_WITH_TIMEOUT(server.requests() >= 1, 5000); // saisi dans la session : lu
    server.reset(); // avant la réouverture : une requête du lecteur serait comptée
    Fixture reader;
    QVERIFY(saveAndReopen(writer, reader, dir.filePath(QStringLiteral("recu.actcraft"))));
    QCOMPARE(reader.frames.count(), 2);
    QCOMPARE(reader.frames.blockedCount(), 2);
    QVERIFY(role(reader.frames, 0, ui::WebFrameItems::BlockedRole).toBool());
    QVERIFY(!content(reader.frames, 0).contains(QStringLiteral("distant")));
    QVERIFY(role(reader.frames, 1, ui::WebFrameItems::BlockedRole).toBool());
    QVERIFY2(!content(reader.frames, 1).contains(server.origin()), qPrintable(content(reader.frames, 1)));
    QTest::qWait(1000);
    QCOMPARE(server.requests(), 0);
}

// Déblocage confirmé : la requête part, le contenu arrive ; refusé : rien.
void TstUiWebFrame::unblockThenRequestReachesTheServer()
{
    TestServer server;
    QVERIFY(server.listening());
    server.page(QStringLiteral("/page"), "<p>distant</p>");
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    Fixture writer;
    QVERIFY(writer.frames.insertFrame({}, server.url(QStringLiteral("/page")).toString()));
    QTRY_VERIFY_WITH_TIMEOUT(server.requests() >= 1, 5000);
    server.reset(); // avant la réouverture : une requête du lecteur serait comptée
    Fixture reader;
    QVERIFY(saveAndReopen(writer, reader, dir.filePath(QStringLiteral("recu.actcraft"))));

    reader.frames.requestUnblock(0);
    QVERIFY(reader.frames.unblockPending());
    reader.frames.answer(false);
    QVERIFY(!reader.frames.unblockPending());
    QTest::qWait(300);
    QCOMPARE(server.requests(), 0);
    QCOMPARE(reader.frames.blockedCount(), 1);

    reader.frames.requestUnblock(0);
    reader.frames.answer(true);
    QTRY_VERIFY_WITH_TIMEOUT(server.requests() >= 1, 5000);
    QCOMPARE(reader.frames.blockedCount(), 0);
    QTRY_VERIFY_WITH_TIMEOUT(content(reader.frames, 0).contains(QStringLiteral("distant")), 5000);
}

// Le déblocage vaut pour cette page de ce document : la page suivante reste bloquée, sans requête.
void TstUiWebFrame::otherPageStaysBlocked()
{
    TestServer server;
    QVERIFY(server.listening());
    server.page(QStringLiteral("/a"), "<p>page a</p>");
    server.page(QStringLiteral("/b"), "<p>page b</p>");
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    Fixture writer;
    QVERIFY(writer.frames.insertFrame({}, server.url(QStringLiteral("/a")).toString()));
    writer.document.nextPage();
    QCOMPARE(writer.document.pageCount(), 2);
    QCOMPARE(writer.document.currentIndex(), 1);
    QVERIFY(writer.frames.insertFrame({}, server.url(QStringLiteral("/b")).toString()));
    QTRY_VERIFY_WITH_TIMEOUT(server.requests() >= 2, 5000);
    server.reset(); // avant la réouverture : une requête du lecteur serait comptée
    Fixture reader;
    QVERIFY(saveAndReopen(writer, reader, dir.filePath(QStringLiteral("recu.actcraft"))));
    reader.document.showPage(0);
    QCOMPARE(reader.frames.blockedCount(), 1);
    reader.frames.requestUnblock(0);
    reader.frames.answer(true);
    QTRY_VERIFY_WITH_TIMEOUT(server.paths().contains(QStringLiteral("/a")), 5000);

    reader.document.showPage(1);
    QCOMPARE(reader.frames.count(), 1);
    QCOMPARE(reader.frames.blockedCount(), 1);
    QTest::qWait(500);
    QVERIFY(!server.paths().contains(QStringLiteral("/b")));
    // Revenir à la page débloquée la recharge sans redemander.
    reader.document.showPage(0);
    QCOMPARE(reader.frames.blockedCount(), 0);
}

// Boîte du cadre : code saisi → un cadre, une étape ; la vue reçoit le code réécrit, sans adresse distante.
void TstUiWebFrame::insertFromTypedHtml()
{
    Fixture f;
    QVERIFY(!f.frames.insertFrame({}, {}));
    QCOMPARE(f.frames.count(), 0);
    QVERIFY(f.frames.insertFrame(QStringLiteral("<h1>Bonjour</h1><img src=\"http://192.0.2.1/x.png\">"), {}));
    QCOMPARE(f.frames.count(), 1);
    QCOMPARE(f.frames.blockedCount(), 0);
    QCOMPARE(role(f.frames, 0, ui::WebFrameItems::HtmlRole).toString(),
             QStringLiteral("<h1>Bonjour</h1><img src=\"http://192.0.2.1/x.png\">"));
    const QString html = content(f.frames, 0);
    QVERIFY2(html.contains(QStringLiteral("<h1>Bonjour</h1>")), qPrintable(html));
    QVERIFY2(!html.contains(QStringLiteral("192.0.2.1")), qPrintable(html)); // origine jamais saisie
    QVERIFY(f.document.canUndo());
    f.document.undo();
    QCOMPARE(f.frames.count(), 0);
}

// Poignée relâchée : une étape ; annuler rend la boîte d'avant, annuler encore retire le cadre.
void TstUiWebFrame::resizeIsOneStep()
{
    Fixture f;
    QVERIFY(f.frames.insertFrame(QStringLiteral("<p>x</p>"), {}));
    const QRectF before = role(f.frames, 0, ui::WebFrameItems::BoundsRole).toRectF();
    QVERIFY(!before.isEmpty());
    const QRectF after(40, 50, 320, 180);
    QVERIFY(f.frames.resizeFrame(0, after));
    QCOMPARE(role(f.frames, 0, ui::WebFrameItems::BoundsRole).toRectF(), after);
    f.document.undo();
    QCOMPARE(f.frames.count(), 1);
    QCOMPARE(role(f.frames, 0, ui::WebFrameItems::BoundsRole).toRectF(), before);
    f.document.undo();
    QCOMPARE(f.frames.count(), 0);
}

// Image fixe : la zone du cadre devient une image de la page, en une étape ; le cadre reste.
void TstUiWebFrame::snapshotPlacesAnImage()
{
    Fixture f;
    QVERIFY(f.frames.insertFrame(QStringLiteral("<p>x</p>"), {}));
    const QRectF bounds = role(f.frames, 0, ui::WebFrameItems::BoundsRole).toRectF();
    QVERIFY(!f.frames.snapshot(0)); // sans moyen de capture : rien
    QRectF asked;
    f.frames.setGrabber([&asked](const QRectF& zone) {
        asked = zone;
        QImage image(64, 48, QImage::Format_ARGB32_Premultiplied);
        image.fill(Qt::red);
        return image;
    });
    const int objects = f.document.shownObjectCount();
    QVERIFY(f.frames.snapshot(0));
    QCOMPARE(asked, bounds);
    QCOMPARE(f.document.shownObjectCount(), objects + 1);
    QCOMPARE(f.frames.count(), 1);
    f.document.undo();
    QCOMPARE(f.document.shownObjectCount(), objects);
    QCOMPARE(f.frames.count(), 1);
}

// .actcraft enregistré puis rouvert : code, adresse et boîte du cadre gardés.
void TstUiWebFrame::roundTripKeepsTheFrame()
{
    TestServer server;
    QVERIFY(server.listening());
    const QString address = server.url(QStringLiteral("/x")).toString();
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    Fixture writer;
    QVERIFY(writer.frames.insertFrame(QStringLiteral("<p>garde</p>"), address));
    QVERIFY(writer.frames.resizeFrame(0, QRectF(10, 20, 300, 200)));
    Fixture reader;
    QVERIFY(saveAndReopen(writer, reader, dir.filePath(QStringLiteral("aller-retour.actcraft"))));
    QCOMPARE(reader.frames.count(), 1);
    QCOMPARE(role(reader.frames, 0, ui::WebFrameItems::HtmlRole).toString(), QStringLiteral("<p>garde</p>"));
    QCOMPARE(role(reader.frames, 0, ui::WebFrameItems::AddressRole).toString(), address);
    QCOMPARE(role(reader.frames, 0, ui::WebFrameItems::BoundsRole).toRectF(), QRectF(10, 20, 300, 200));
    QVERIFY(role(reader.frames, 0, ui::WebFrameItems::BlockedRole).toBool()); // reçu, distant
}

// Miniatures, exports, rendu logiciel : le repère neutre de paintPage (L8-T3), jamais le contenu web, aucune
// ressource demandée.
void TstUiWebFrame::paintedAsANeutralFrame()
{
    Fixture f;
    QVERIFY(f.frames.insertFrame(QStringLiteral("<body style=\"background:#ff0000\">rouge</body>"), {}));
    QVERIFY(f.frames.resizeFrame(0, QRectF(100, 100, 300, 200)));
    int lookups = 0;
    const render::ImageLookup counting = [&lookups](const std::string&) {
        ++lookups;
        return QImage();
    };
    const QImage image = render::renderPage(f.document.document().page(0), counting, 1.0)
                             .convertToFormat(QImage::Format_RGB32);
    int marked = 0;
    for (int x = 105; x < 395; ++x)
        marked += std::min(image.pixelColor(x, 99).red(), image.pixelColor(x, 100).red()) < 230 ? 1 : 0;
    QVERIFY2(marked > 40, qPrintable(QString::number(marked))); // bord haut : des tirets
    QCOMPARE(image.pixelColor(250, 200), QColor(Qt::white));    // intérieur : pas le rouge de la page
    QCOMPARE(lookups, 0);
}

// Avec le moteur : la vue ne demande que les sous-ressources passées. Une balise que la réécriture n'aurait
// pas vue (ajoutée ici après coup) est arrêtée par la règle de sécurité du contenu. Hors écran, si la vue ne
// charge rien, le moteur manque : saut nommé.
void TstUiWebFrame::viewFetchesOnlyDecidedSubResources()
{
#if !ACTCRAFT_WEBFRAME
    QSKIP("ACTCRAFT_WEBFRAME absent");
#else
    TestServer typed;
    TestServer other;
    QVERIFY(typed.listening() && other.listening());
    typed.page(QStringLiteral("/page"), "<p>distant</p><img src=\"/ok.png\"><img src=\"" +
                                            other.url(QStringLiteral("/spy.png")).toEncoded() + "\">");
    typed.page(QStringLiteral("/ok.png"), "x");
    Fixture f;
    QVERIFY(f.frames.insertFrame({}, typed.url(QStringLiteral("/page")).toString()));
    QTRY_VERIFY_WITH_TIMEOUT(content(f.frames, 0).contains(QStringLiteral("distant")), 5000);
    const QString html =
        content(f.frames, 0) +
        QStringLiteral("<img src=\"%1\">").arg(other.url(QStringLiteral("/missed.png")).toString());
    QQmlEngine engine;
    QQmlComponent component(&engine);
    component.setData("import QtQuick\nimport QtWebView\nWebView { width: 400; height: 300; "
                      "settings.javaScriptEnabled: false; settings.allowFileAccess: false; "
                      "settings.localContentCanAccessFileUrls: false }",
                      QUrl());
    if (component.isError())
        QSKIP("moteur web indisponible hors écran");
    QQuickWindow window;
    window.resize(400, 300);
    std::unique_ptr<QObject> created(component.create());
    auto* view = qobject_cast<QQuickItem*>(created.get());
    if (!view)
        QSKIP("moteur web indisponible hors écran");
    view->setParentItem(window.contentItem());
    window.show();
    const QUrl base = role(f.frames, 0, ui::WebFrameItems::BaseUrlRole).toUrl();
    typed.reset();
    QVERIFY(QMetaObject::invokeMethod(view, "loadHtml", Q_ARG(QString, html), Q_ARG(QUrl, base)));
    const bool loaded =
        QTest::qWaitFor([&] { return typed.paths().contains(QStringLiteral("/ok.png")); }, 8000);
    if (!loaded)
        QSKIP("moteur web indisponible hors écran");
    QTest::qWait(500);
    QCOMPARE(other.requests(), 0);
#endif
}

// Contenu piégé : chaque forme que le parseur HTML du moteur lit comme une balise (séparateur `/`, attributs
// collés, casse, entités, fin de style en `</style/`) ou que CSS lit comme une adresse (`url()` à parenthèse
// dans la chaîne, `@import` collé ou commenté, échappements, image-set) ne laisse aucune adresse
// d'evil.example dans les balises ni les feuilles de style de la sortie. Un `<` que la réécriture ne
// reconnaît pas devient du texte : il ne demande rien.
void TstUiWebFrame::prepareHtmlLeavesNoUndecidedAddress_data()
{
    QTest::addColumn<QString>("html");
    const auto row = [](const char* name, const char* html) {
        QTest::newRow(name) << QString::fromUtf8(html);
    };
    row("meta refresh /", R"h(<meta/http-equiv="refresh" content="0;url=https://evil.example/r">)h");
    row("meta refresh casse", R"h(<META HTTP-EQUIV=Refresh CONTENT="0; URL=https://evil.example/r">)h");
    row("meta http-equiv",
        R"h(<meta http-equiv="content-security-policy" content="img-src https://evil.example">)h");
    row("link preconnect /", R"h(<link/rel=preconnect href=https://evil.example/>)h");
    row("link dns-prefetch", R"h(<link rel="dns-prefetch" href="//evil.example">)h");
    row("link preload",
        R"h(<link rel=preload as=image href="https://evil.example/p.png" imagesrcset="https://evil.example/q.png 2x">)h");
    row("link prefetch", R"h(<link rel="prefetch" href="https://evil.example/p">)h");
    row("a /", R"h(<a/href="https://evil.example/a">x</a>)h");
    row("attributs colles", R"h(<a href="#x"onclick="y"ping="https://evil.example/ping">x</a>)h");
    row("img /src", R"h(<img/src="https://evil.example/i.png">)h");
    row("srcset", R"h(<img src="/ok.png" srcset="https://evil.example/s.png 1x, /ok2.png 2x">)h");
    row("source",
        R"h(<picture><source srcset="https://evil.example/s.png"><source src="https://evil.example/v"></picture>)h");
    row("object", R"h(<object data="https://evil.example/o"></object>)h");
    row("embed", R"h(<embed src="https://evil.example/e">)h");
    row("iframe srcdoc", R"h(<iframe srcdoc="&lt;img src=https://evil.example/sd.png&gt;"></iframe>)h");
    row("iframe /srcdoc", R"h(<iframe/srcdoc='<img src="https://evil.example/sd.png">'></iframe>)h");
    row("style url", R"h(<div style="background:url(https://evil.example/bg.png)">x</div>)h");
    row("style url parenthese", R"h(<div style='background:url("https://evil.example/a)b.png")'>x</div>)h");
    row("style url entite", R"h(<div style="background:u&#114;l(https://evil.example/bg.png)">x</div>)h");
    row("style url echappement", R"h(<div style="background:u\72l(https://evil.example/bg.png)">x</div>)h");
    row("@import", R"h(<style>@import "https://evil.example/a.css";</style>)h");
    row("@import colle", R"h(<style>@import'https://evil.example/a.css';</style>)h");
    row("@import commentaire", R"h(<style>@import/**/"https://evil.example/a.css";</style>)h");
    row("@import url", R"h(<style>@import url(https://evil.example/a.css);</style>)h");
    row("@import echappement", R"h(<style>@\69mport "https://evil.example/a.css";</style>)h");
    row("image-set", R"h(<style>.x{background-image:image-set("https://evil.example/a.png" 1x)}</style>)h");
    row("url parenthese", R"h(<style>.y{background:url('https://evil.example/q)q.png')}</style>)h");
    row("fin de style /", R"h(<style>a{}</style/><img src=https://evil.example/st.png></style>)h");
    row("style svg", R"h(<svg><style><img src="https://evil.example/br.png"></style></svg>)h");
    row("base", R"h(<base href="https://evil.example/"><img src="/ok.png">)h");
    row("formulaire",
        R"h(<form action="https://evil.example/f"><button formaction="https://evil.example/fa">x</button></form>)h");
    row("svg xlink",
        R"h(<svg><image xlink:href="https://evil.example/x.png"/><a xlink:href="https://evil.example/l"><text>t</text></a></svg>)h");
    row("svg set",
        R"h(<svg><a href="#t"><set attributeName="href" to="https://evil.example/set"/><text>t</text></a></svg>)h");
    row("svg animate",
        R"h(<svg><a href="#t"><animate attributeName="xlink:href" values="https://evil.example/n"/></a></svg>)h");
    row("svg fill", R"h(<svg><rect fill="url(https://evil.example/f.svg#p)"/></svg>)h");
    row("casse", R"h(<IMG SRC="HTTPS://EVIL.EXAMPLE/UP.PNG"><A HREF="HTTPS://EVIL.EXAMPLE/UP">x</A>)h");
    row("entites",
        R"h(<img src="&#x68;ttps://&#101;vil.example/e.png"><img src="https&colon;//evil.example/c.png">)h");
    row("commentaire", R"h(<!-- <img src="https://evil.example/c.png"> --><!--><img src="/ok.png">)h");
    row("balise ouverte", R"h(<img src="/ok.png"><img src="https://evil.example/u.png")h");
    row("tabulation verticale", "<img\vsrc=\"https://evil.example/vt.png\">");
    row("video",
        R"h(<video poster="https://evil.example/p.png" src="https://evil.example/v"><track src="https://evil.example/t"></video>)h");
    row("background",
        R"h(<body background="https://evil.example/b.png"><table background="https://evil.example/t.png"></table>)h");
    row("input image", R"h(<input type=image src="https://evil.example/in.png">)h");
    row("cdata", R"h(<svg><![CDATA[<img src="https://evil.example/cd.png">]]></svg>)h");
}

void TstUiWebFrame::prepareHtmlLeavesNoUndecidedAddress()
{
    QFETCH(QString, html);
    for (const QUrl& base : {QUrl(QStringLiteral("https://ok.example/dir/page.html")), QUrl()}) {
        OnlyOk decide;
        const auto prepared = ui::WebFrameItems::prepareHtml(html, base, std::ref(decide));
        const QString read = addressBearing(prepared.html);
        QVERIFY2(!read.contains(QStringLiteral("evil")), qPrintable(prepared.html));
        for (const QUrl& link : prepared.links)
            QVERIFY(link.isValid()); // cible d'un renvoi interne : décidée au clic, jamais dans la sortie
    }
}

// Témoin du précédent : les formes ordinaires et à séparateur `/` gardent leurs adresses décidées, absolues ;
// une valeur d'attribut garde ses entités telles quelles (ni doublées ni perdues).
void TstUiWebFrame::prepareHtmlKeepsDecidedAddresses()
{
    const QString html = QStringLiteral(
        R"h(<img/src="/a.png"><a/href="/b">l</a><link rel=stylesheet href=/s.css>)h"
        R"h(<div style="background:url('/bg.png')">x</div><style>@import "/i.css"; .c{background:url(/c.png)}</style>)h"
        R"h(<img src="/x.png" srcset="/s1.png 1x, /s2.png 2x"><p title="a &amp; b &eacute;">t</p>)h");
    OnlyOk decide;
    const auto prepared = ui::WebFrameItems::prepareHtml(
        html, QUrl(QStringLiteral("https://ok.example/dir/")), std::ref(decide));
    const QString& out = prepared.html;
    for (const char* expected :
         {"src=\"https://ok.example/a.png\"", "href=\"#actcraft-link-0\"", "https://ok.example/s.css",
          "https://ok.example/bg.png", "@import \"https://ok.example/i.css\"", "https://ok.example/c.png",
          "https://ok.example/s1.png 1x, https://ok.example/s2.png 2x", "title=\"a &amp; b &eacute;\""})
        QVERIFY2(out.contains(QString::fromUtf8(expected)),
                 qPrintable(QString::fromUtf8(expected) + QStringLiteral(" | ") + out));
    QCOMPARE(prepared.links, QVector<QUrl>{QUrl(QStringLiteral("https://ok.example/b"))});
}

// Page de 16 Mo, piégée pour les motifs à retour arrière (commentaires, styles et balises jamais fermés) : la
// réécriture reste linéaire. Le temps est imprimé (preuve), la borne est large (construction de débogage).
void TstUiWebFrame::prepareHtmlStaysLinearOn16MB()
{
    const QString chunk = QStringLiteral(
        R"h(<p a=1/b=2 c='3'>t</p><img/src="/a.png" srcset="/b.png 1x"><style>.x{background:url("/c.png")}</style>)h"
        R"h(<a href="/d">d</a><!-- c -->)h");
    const QString trap = QStringLiteral("<a b=c/d/e/f/g <style x <!--x url(\"q ");
    QString big;
    big.reserve(16 * 1024 * 1024 + chunk.size() + trap.size());
    while (big.size() < 8 * 1024 * 1024)
        big += chunk;
    while (big.size() < 16 * 1024 * 1024)
        big += trap;
    OnlyOk decide;
    QElapsedTimer timer;
    timer.start();
    const auto prepared =
        ui::WebFrameItems::prepareHtml(big, QUrl(QStringLiteral("https://ok.example/")), std::ref(decide));
    const qint64 ms = timer.elapsed();
    qInfo("prepareHtml : %lld ms pour %lld caracteres", static_cast<long long>(ms),
          static_cast<long long>(big.size()));
    QVERIFY2(ms < 20000, qPrintable(QString::number(ms)));
    QVERIFY(prepared.html.contains(QStringLiteral("https://ok.example/c.png")));
    QVERIFY(!prepared.html.contains(QStringLiteral("<!--")));
}

// Navigation que le moteur prendrait de lui-même, même vers la page affichée : rendue au modèle (vrai : la
// vue s'arrête), sinon la page serait chargée par le moteur, sans la règle de sécurité du contenu. Seuls le
// contenu donné (about:blank) et ses renvois de fragment restent à la vue.
void TstUiWebFrame::engineNavigationAlwaysReturnsToTheModel()
{
    TestServer typed;
    QVERIFY(typed.listening());
    typed.page(QStringLiteral("/page"), "<p>distant</p>");
    Fixture f;
    QVERIFY(f.frames.insertFrame({}, typed.url(QStringLiteral("/page")).toString()));
    QTRY_VERIFY_WITH_TIMEOUT(content(f.frames, 0).contains(QStringLiteral("distant")), 5000);
    QVERIFY(!f.frames.followLink(0, QUrl(QStringLiteral("about:blank"))));
    QVERIFY(!f.frames.followLink(0, QUrl(QStringLiteral("about:blank#haut"))));
    typed.reset();
    const QUrl shown = typed.url(QStringLiteral("/page"));
    QVERIFY(f.frames.followLink(0, shown));
    QVERIFY(f.frames.followLink(0, QUrl(shown.toString() + QStringLiteral("#ancre"))));
    QTRY_VERIFY_WITH_TIMEOUT(typed.paths().contains(QStringLiteral("/page")), 5000); // relue par le modèle
}

// Cadre en code HTML d'un document reçu : repère, puis déblocage confirmé → ses sous-ressources passent.
void TstUiWebFrame::receivedCodeFrameUnblocks()
{
    TestServer server;
    QVERIFY(server.listening());
    server.page(QStringLiteral("/page"), "<p>distant</p>");
    const QString image = server.url(QStringLiteral("/img.png")).toString();
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    Fixture writer;
    QVERIFY(writer.frames.insertFrame({}, server.url(QStringLiteral("/page")).toString()));
    QVERIFY(writer.frames.insertFrame(QStringLiteral("<p>code</p><img src=\"%1\">").arg(image), {}));
    QVERIFY2(content(writer.frames, 1).contains(image),
             qPrintable(content(writer.frames, 1))); // origine saisie
    QTRY_VERIFY_WITH_TIMEOUT(server.requests() >= 1, 5000);
    Fixture reader;
    QVERIFY(saveAndReopen(writer, reader, dir.filePath(QStringLiteral("code.actcraft"))));
    QVERIFY(role(reader.frames, 1, ui::WebFrameItems::BlockedRole).toBool());
    QVERIFY(!content(reader.frames, 1).contains(image));
    reader.frames.requestUnblock(1);
    QVERIFY(reader.frames.unblockPending());
    reader.frames.answer(true);
    QVERIFY(!role(reader.frames, 1, ui::WebFrameItems::BlockedRole).toBool());
    QVERIFY2(content(reader.frames, 1).contains(image), qPrintable(content(reader.frames, 1)));
}

int main(int argc, char** argv)
{
#if ACTCRAFT_WEBFRAME
    QtWebView::initialize();
#endif
    QGuiApplication app(argc, argv);
    TstUiWebFrame test;
    return QTest::qExec(&test, argc, argv);
}

#include "tst_ui_webframe.moc"
