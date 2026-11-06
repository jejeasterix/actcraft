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

#include <QByteArray>
#include <QGuiApplication>
#include <QHash>
#include <QHostAddress>
#include <QImage>
#include <QPointer>
#include <QSignalSpy>
#include <QStandardPaths>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTemporaryDir>
#include <QtTest>

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
    QString origin() const { return QStringLiteral("127.0.0.1:%1").arg(m_server.serverPort()); }
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
};

void TstUiWebFrame::initTestCase()
{
    QStandardPaths::setTestModeEnabled(true);
}

// Le cœur de la tâche : la page saisie est lue par le modèle (le serveur la voit sans moteur), la sous-ressource
// d'une origine non saisie ne figure pas dans le HTML donné à la vue et n'est jamais demandée ; celle de
// l'origine saisie y reste, en adresse absolue, et la règle de sécurité du contenu n'autorise qu'elle.
void TstUiWebFrame::subResourcesPassThroughThePolicy()
{
    TestServer typed;
    TestServer other;
    QVERIFY(typed.listening() && other.listening());
    const QUrl spy = other.url(QStringLiteral("/spy.png"));
    typed.page(QStringLiteral("/page"), "<html><head><link rel=\"stylesheet\" href=\"" + other.url("/s.css").toEncoded() +
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
    QTRY_VERIFY_WITH_TIMEOUT(server.requests() >= 1, 5000); // saisi dans la session : lu
    Fixture reader;
    QVERIFY(saveAndReopen(writer, reader, dir.filePath(QStringLiteral("recu.actcraft"))));
    server.reset();
    QCOMPARE(reader.frames.count(), 1);
    QCOMPARE(reader.frames.blockedCount(), 1);
    QVERIFY(role(reader.frames, 0, ui::WebFrameItems::BlockedRole).toBool());
    QVERIFY(!content(reader.frames, 0).contains(QStringLiteral("distant")));
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
    Fixture reader;
    QVERIFY(saveAndReopen(writer, reader, dir.filePath(QStringLiteral("recu.actcraft"))));
    server.reset();

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
    Fixture reader;
    QVERIFY(saveAndReopen(writer, reader, dir.filePath(QStringLiteral("recu.actcraft"))));
    server.reset();
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
