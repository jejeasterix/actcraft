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
#include "support/testserver.h"

#include <actcraft/core/document.h>
#include <actcraft/core/pageedits.h>
#include <actcraft/actions/profilestore.h>
#include <actcraft/render/pagepainter.h>
#include <actcraft/ui/commands.h>
#include <actcraft/ui/preferences.h>
#include <actcraft/ui/profilecontroller.h>
#include <actcraft/ui/webframeengine.h>

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
#include <QSettings>
#include <QSignalSpy>
#include <QStandardPaths>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTemporaryDir>
#include <QThread>
#include <QThreadPool>
#include <QtTest>

#include <algorithm>
#include <atomic>
#include <functional>
#include <limits>
#include <memory>

using namespace actcraft;
using Verdict = ui::WebContentPolicy::Verdict;

namespace {

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
    // Tout ce fichier éprouve le chemin de la politique : gardes allumées, sauf demande (cadrage n° 9, DA5).
    explicit Fixture(bool guards = true)
    {
        frames.setGuardsEnabled(guards);
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
    void prepareHtmlKeepsRawTextAsText();
    void prepareHtmlDropsScripts();
    void prepareHtmlStaysLinear();
    void rewriteLeavesTheInterfaceThreadAtOnce();
    void staleRewriteIsDropped();
    void engineNavigationAlwaysReturnsToTheModel();
    void receivedCodeFrameUnblocks();
    void unblockDuringRewriteHolds();
    void hrefOutsideSvgResourceBecomesALink();
    void navigationToLocalContentDropsTheInFlightReply_data();
    void navigationToLocalContentDropsTheInFlightReply();
    void replyOfARecreatedRowIsNotTakenForANewNavigation();
    void unblockFollowsThePageNotItsNumber();
    void dialogRefusesEmptyFields();
    void rebuildConvertsOnlyChangedHtml();
    void guardsFollowTheProfileSetting();
    void unrelatedProfileChangeReloadsNothing();
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

// Seconde lecture, naïve, de toute la sortie : sans rendre les entités (`&lt;meta` est du texte), sans suivre
// les guillemets ni l'état du moteur, chaque segment d'un `<` littéral au `>` suivant, en minuscules. Elle
// voit ce qu'un moteur en état RCDATA, RAWTEXT ou étranger lirait comme balise là où la première lecture suit
// les guillemets comme la réécriture. La règle posée en tête par la réécriture (CSP, referrer) est retirée.
QStringList naiveTags(const QString& html)
{
    const QString head = QStringLiteral("<meta name=\"referrer\" content=\"no-referrer\">");
    const qsizetype at = html.indexOf(head);
    const QString body = (at < 0 ? html : html.mid(at + head.size())).toLower();
    QStringList spans;
    for (qsizetype lt = body.indexOf(u'<'); lt >= 0; lt = body.indexOf(u'<', lt + 1)) {
        const qsizetype gt = body.indexOf(u'>', lt);
        spans.append(gt < 0 ? body.mid(lt) : body.mid(lt, gt - lt + 1));
    }
    return spans;
}

// Page ordinaire d'au moins `size` caractères, finie par `marker` dans un paragraphe.
QString bigPage(qsizetype size, const QString& marker)
{
    const QString chunk = QStringLiteral(
        R"h(<p a=1/b=2 c='3'>t</p><img/src="/a.png" srcset="/b.png 1x"><style>.x{background:url("/c.png")}</style>)h"
        R"h(<a href="/d">d</a><!-- c -->)h");
    QString big;
    big.reserve(size + chunk.size() + marker.size() + 7);
    while (big.size() < size)
        big += chunk;
    return big + QStringLiteral("<p>") + marker + QStringLiteral("</p>");
}

} // namespace

void TstUiWebFrame::initTestCase()
{
    QStandardPaths::setTestModeEnabled(true);
}

void TstUiWebFrame::navigationToLocalContentDropsTheInFlightReply_data()
{
    QTest::addColumn<bool>("local");
    QTest::newRow("fichier local") << true;
    QTest::newRow("about:blank") << false;
}

namespace {

// Occupe tous les fils de QThreadPool::globalInstance() jusqu'à release() : les lectures et réécritures
// lancées entre-temps restent en file, donc leur résultat n'arrive pas avant l'instant choisi par le test.
class PoolGate
{
public:
    PoolGate() : m_pool(QThreadPool::globalInstance()), m_jobs(m_pool->maxThreadCount()) {}
    ~PoolGate()
    {
        release();
        m_pool->waitForDone(10000);
    }
    void close()
    {
        for (int i = 0; i < m_jobs; ++i) {
            m_pool->start([this] {
                ++m_running;
                while (!m_open)
                    QThread::msleep(2);
            });
        }
    }
    bool closed() const { return m_running.load() == m_jobs; }
    void release() { m_open = true; }

private:
    QThreadPool* m_pool;
    int m_jobs;
    std::atomic<int> m_running{0};
    std::atomic<bool> m_open{false};
};

} // namespace

// L8-C12, constat 5. `fetched` jugeait une réponse périmée sur le seul `serial` ; la navigation vers un
// fichier local ou about: ne le faisait avancer qu'à l'arrivée du contenu (finishContent), donc pendant la
// lecture une réponse réseau en vol était acceptée et écrasait le contenu neuf. Le test retient la réponse du
// serveur, retourne au contenu local en tenant sa lecture en file (PoolGate), puis relâche la réponse : elle
// arrive pendant la lecture et ne doit rien changer. Sous l'ancien contrôle, le contenu devient « reseau » et
// l'adresse celle du serveur.
void TstUiWebFrame::navigationToLocalContentDropsTheInFlightReply()
{
    QFETCH(bool, local);
    TestServer server;
    QVERIFY(server.listening());
    server.page(QStringLiteral("/slow"), "<p>reseau</p>");
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString path = dir.filePath(QStringLiteral("local.html"));
    {
        QFile file(path);
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.write("<p>local</p>");
    }
    // Un fichier local n'est permis que dans le dossier du document : document enregistré, puis rouvert.
    Fixture writer;
    Fixture f;
    QVERIFY(saveAndReopen(writer, f, dir.filePath(QStringLiteral("recu.actcraft"))));
    // Rang 0 : saisit l'adresse du serveur, ce qui l'autorise pour le document (le rang 1 y navigue).
    const QUrl slow = server.url(QStringLiteral("/slow"));
    QVERIFY(f.frames.insertFrame({}, slow.toString()));
    QTRY_VERIFY_WITH_TIMEOUT(content(f.frames, 0).contains(QStringLiteral("reseau")), 5000);
    // Rang 1 : le cadre éprouvé, sur du contenu local.
    const QString start = local ? QUrl::fromLocalFile(path).toString() : QStringLiteral("about:blank");
    QVERIFY(f.frames.insertFrame({}, start));
    QCOMPARE(f.frames.count(), 2);
    QTRY_VERIFY_WITH_TIMEOUT(!role(f.frames, 1, ui::WebFrameItems::LoadingRole).toBool(), 5000);
    QVERIFY(!role(f.frames, 1, ui::WebFrameItems::RefusedRole).toBool());
    QVERIFY(!role(f.frames, 1, ui::WebFrameItems::BlockedRole).toBool());
    if (local)
        QTRY_VERIFY_WITH_TIMEOUT(content(f.frames, 1).contains(QStringLiteral("local")), 5000);

    server.hold(QStringLiteral("/slow"));
    server.reset();
    QVERIFY(f.frames.followLink(1, slow)); // navigation du moteur : décidée, puis demandée
    QTRY_VERIFY_WITH_TIMEOUT(server.paths().contains(QStringLiteral("/slow")), 5000);
    QVERIFY(role(f.frames, 1, ui::WebFrameItems::LoadingRole).toBool()); // la réponse est en vol

    PoolGate gate;
    gate.close();
    QTRY_VERIFY_WITH_TIMEOUT(gate.closed(), 5000);
    f.frames.goBack(1); // retour au contenu local : sa lecture attend dans la file
    QVERIFY(role(f.frames, 1, ui::WebFrameItems::LoadingRole).toBool());
    server.release();
    QTest::qWait(300); // la réponse arrive pendant la lecture : elle doit être abandonnée
    gate.release();
    QTRY_VERIFY_WITH_TIMEOUT(!role(f.frames, 1, ui::WebFrameItems::LoadingRole).toBool(), 5000);
    QTest::qWait(200);
    const QString shown = content(f.frames, 1);
    QVERIFY2(!shown.contains(QStringLiteral("reseau")), qPrintable(shown));
    QVERIFY(role(f.frames, 1, ui::WebFrameItems::BaseUrlRole).toUrl() != slow);
    if (local)
        QVERIFY2(shown.contains(QStringLiteral("local")), qPrintable(shown));
}

// L8-C12 essai 2. Le compteur de navigation était propre à chaque ligne et repartait de 0 quand `rebuild` la
// recréait (changement de page) : après un aller-retour de page, la deuxième navigation de la ligne neuve
// portait le même numéro que la navigation lente d'avant, dont la réponse tardive était alors acceptée.
void TstUiWebFrame::replyOfARecreatedRowIsNotTakenForANewNavigation()
{
    TestServer server;
    QVERIFY(server.listening());
    server.page(QStringLiteral("/a"), "<p>depart</p>");
    server.page(QStringLiteral("/b"), "<p>lente</p>");
    server.page(QStringLiteral("/c"), "<p>derniere</p>");
    Fixture f;
    QVERIFY(f.frames.insertFrame({}, server.url(QStringLiteral("/a")).toString()));
    QTRY_VERIFY_WITH_TIMEOUT(content(f.frames, 0).contains(QStringLiteral("depart")), 5000);
    f.document.nextPage();
    QCOMPARE(f.document.pageCount(), 2);
    f.document.showPage(0);
    QTRY_VERIFY_WITH_TIMEOUT(content(f.frames, 0).contains(QStringLiteral("depart")), 5000);
    server.hold(QStringLiteral("/b"));
    server.reset();
    QVERIFY(f.frames.followLink(0, server.url(QStringLiteral("/b")))); // en vol
    QTRY_VERIFY_WITH_TIMEOUT(server.paths().contains(QStringLiteral("/b")), 5000);
    f.document.showPage(1);
    QTRY_COMPARE_WITH_TIMEOUT(f.frames.count(), 0, 5000);
    f.document.showPage(0);
    QTRY_COMPARE_WITH_TIMEOUT(f.frames.count(), 1, 5000);
    QTRY_VERIFY_WITH_TIMEOUT(content(f.frames, 0).contains(QStringLiteral("depart")), 5000);
    QVERIFY(f.frames.followLink(0, server.url(QStringLiteral("/c"))));
    QTRY_VERIFY_WITH_TIMEOUT(content(f.frames, 0).contains(QStringLiteral("derniere")), 5000);
    server.release(); // la réponse lente arrive : elle ne vaut plus
    QTest::qWait(300);
    const QString shown = content(f.frames, 0);
    QVERIFY2(shown.contains(QStringLiteral("derniere")) && !shown.contains(QStringLiteral("lente")),
             qPrintable(shown));
}

// L8-C12, constat 7. « OK » avec l'adresse et le code vides appelait insertFrame("", "") : le modèle refuse,
// mais la boîte se fermait comme si tout allait bien. « OK » est grisé tant que rien n'est saisi (blancs
// compris), et accepter à vide ne crée rien. Sous l'ancienne boîte, « OK » est actif d'emblée.
void TstUiWebFrame::dialogRefusesEmptyFields()
{
    const QString qmlDir =
        QDir::cleanPath(QFileInfo(QStringLiteral(__FILE__)).absolutePath() + QStringLiteral("/../ui/qml"));
    QQmlEngine engine;
    QQmlComponent component(&engine);
    component.setData(QByteArrayLiteral(R"qml(
import QtQuick
import QtQuick.Controls.Basic
Item {
    width: 800
    height: 600
    property alias dialog: d
    WebFrameDialog { id: d }
    function box() {
        for (let i = 0; i < d.data.length; ++i) {
            if (d.data[i].objectName === "webFrameDialogBox")
                return d.data[i]
        }
        return null
    }
    function okEnabled() { return box().standardButton(Dialog.Ok).enabled }
}
)qml"),
                      QUrl::fromLocalFile(qmlDir + QStringLiteral("/harness.qml")));
    std::unique_ptr<QObject> root(component.create());
    QVERIFY2(root, qPrintable(component.errorString()));
    QQuickWindow window;
    qobject_cast<QQuickItem*>(root.get())->setParentItem(window.contentItem());
    window.resize(800, 600);
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));
    auto call = [&](const char* name) {
        QVariant value;
        QMetaObject::invokeMethod(root.get(), name, Q_RETURN_ARG(QVariant, value));
        return value;
    };
    QObject* dialog = root->property("dialog").value<QObject*>();
    QVERIFY(dialog);
    Fixture f;
    auto open = [&] {
        QVERIFY(QMetaObject::invokeMethod(dialog, "open",
                                          Q_ARG(QVariant, QVariant::fromValue<QObject*>(&f.frames))));
        QTRY_VERIFY(dialog->property("opened").toBool());
    };
    open();
    QVERIFY(!call("okEnabled").toBool()); // deux champs vides : « OK » grisé
    QVERIFY(dialog->setProperty("address", QStringLiteral("   ")));
    QVERIFY(dialog->setProperty("code", QStringLiteral("\n  ")));
    QVERIFY(!call("okEnabled").toBool()); // des blancs ne sont pas une saisie
    QObject* box = call("box").value<QObject*>();
    QVERIFY(box);
    QVERIFY(QMetaObject::invokeMethod(box, "accept")); // accepter à vide ne crée rien
    QCOMPARE(f.frames.count(), 0);
    QTRY_VERIFY(!dialog->property("opened").toBool());

    open();
    QVERIFY(dialog->setProperty("code", QStringLiteral("<p>x</p>")));
    QVERIFY(call("okEnabled").toBool()); // du code : « OK » actif
    QVERIFY(QMetaObject::invokeMethod(box, "accept"));
    QCOMPARE(f.frames.count(), 1);
    QCOMPARE(role(f.frames, 0, ui::WebFrameItems::HtmlRole).toString(), QStringLiteral("<p>x</p>"));
}

// L8-C12, constat 9. Le déblocage d'une page était rangé sous son numéro : débloquer la page 1, la retirer,
// et la page qui prend son numéro chargeait son contenu distant sans déblocage. Il suit maintenant la page
// (ses cadres), pas son rang. Sous l'ancien rangement par numéro, la page b est lue (le serveur voit /b).
void TstUiWebFrame::unblockFollowsThePageNotItsNumber()
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
    QTRY_VERIFY_WITH_TIMEOUT(content(reader.frames, 0).contains(QStringLiteral("page a")), 5000);

    // La page a est retirée : la page b prend son numéro, sans avoir été débloquée.
    reader.document.execute(std::make_unique<core::RemovePagesCommand>(std::vector<std::size_t>{0}));
    QCOMPARE(reader.document.pageCount(), 1);
    QCOMPARE(reader.frames.count(), 1);
    QTRY_COMPARE_WITH_TIMEOUT(reader.frames.blockedCount(), 1, 5000);
    QTest::qWait(500);
    QVERIFY2(!server.paths().contains(QStringLiteral("/b")), qPrintable(server.paths().join(u' ')));
    // Le déblocage de cette page, lui, fonctionne.
    reader.frames.requestUnblock(0);
    reader.frames.answer(true);
    QTRY_VERIFY_WITH_TIMEOUT(server.paths().contains(QStringLiteral("/b")), 5000);
    QCOMPARE(reader.frames.blockedCount(), 0);
}

// L8-C12, constat 6. `rebuild` reconvertissait en QString le HTML de chaque cadre à chaque commande, même
// inchangé. Le compteur de conversions du modèle le montre : une poignée relâchée n'en fait aucune, une
// insertion une seule (le cadre neuf). Sous l'ancienne boucle : deux, puis trois.
void TstUiWebFrame::rebuildConvertsOnlyChangedHtml()
{
    Fixture f;
    QVERIFY(f.frames.insertFrame(QStringLiteral("<p>un</p>"), {}));
    QVERIFY(f.frames.insertFrame(QStringLiteral("<p>deux</p>"), {}));
    QCOMPARE(f.frames.count(), 2);

    const std::uint64_t beforeResize = ui::WebFrameItems::htmlConversionCount();
    const QRectF old = role(f.frames, 0, ui::WebFrameItems::BoundsRole).toRectF();
    QVERIFY(f.frames.resizeFrame(0, QRectF(old.x(), old.y(), old.width() + 40, old.height() + 20)));
    QTRY_COMPARE(role(f.frames, 0, ui::WebFrameItems::BoundsRole).toRectF().width(), old.width() + 40);
    QCOMPARE(ui::WebFrameItems::htmlConversionCount() - beforeResize, std::uint64_t{0});

    const std::uint64_t beforeInsert = ui::WebFrameItems::htmlConversionCount();
    QVERIFY(f.frames.insertFrame(QStringLiteral("<p>trois</p>"), {}));
    QCOMPARE(f.frames.count(), 3);
    QCOMPARE(role(f.frames, 2, ui::WebFrameItems::HtmlRole).toString(), QStringLiteral("<p>trois</p>"));
    QCOMPARE(role(f.frames, 0, ui::WebFrameItems::HtmlRole).toString(), QStringLiteral("<p>un</p>"));
    QCOMPARE(ui::WebFrameItems::htmlConversionCount() - beforeInsert, std::uint64_t{1});
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
    // Code HTML : repère posé au retour de la réécriture, faite hors du fil de l'interface.
    QTRY_COMPARE_WITH_TIMEOUT(reader.frames.blockedCount(), 2, 10000);
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
    QTRY_VERIFY_WITH_TIMEOUT(!content(f.frames, 0).isEmpty(), 10000); // réécrit hors du fil de l'interface
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

#ifdef Q_OS_MACOS
#define ACTCRAFT_ENGINE_REQUIRED(ok) QVERIFY2(ok, "moteur web absent sur Mac")
#else
#define ACTCRAFT_ENGINE_REQUIRED(ok)                                                                         \
    do {                                                                                                     \
        if (!(ok))                                                                                           \
            QSKIP("moteur web indisponible hors écran");                                                     \
    } while (false)
#endif

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
#ifdef Q_OS_WIN
    // Reste PC 4 : le moteur (Qt WebEngine) n'est pas prouvé hors écran sous Windows ; ce test charge
    // QtWebEngine et se saute donc là.
    if (QGuiApplication::platformName() == QLatin1String("offscreen"))
        QSKIP("Qt WebEngine hors écran sous Windows : preuve au PC (Reste PC 4)");
#endif
    QQmlEngine engine;
    QQmlComponent component(&engine);
    component.setData("import QtQuick\nimport QtWebEngine\nWebEngineView { width: 400; height: 300; "
                      "settings.javascriptEnabled: false; settings.localContentCanAccessFileUrls: false; "
                      "settings.localContentCanAccessRemoteUrls: false }",
                      QUrl());
    ACTCRAFT_ENGINE_REQUIRED(!component.isError());
    QQuickWindow window;
    window.resize(400, 300);
    std::unique_ptr<QObject> created(component.create());
    auto* view = qobject_cast<QQuickItem*>(created.get());
    ACTCRAFT_ENGINE_REQUIRED(view != nullptr);
    view->setParentItem(window.contentItem());
    window.show();
    typed.reset();
    // Comme WebFrameLayer.qml : sans base, les adresses du contenu sont absolues.
    QVERIFY(QMetaObject::invokeMethod(view, "loadHtml", Q_ARG(QString, html), Q_ARG(QUrl, QUrl())));
    const bool loaded =
        QTest::qWaitFor([&] { return typed.paths().contains(QStringLiteral("/ok.png")); }, 8000);
    ACTCRAFT_ENGINE_REQUIRED(loaded);
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
    // Éléments où le moteur ne lit aucune balise, seulement leur fin (RCDATA : textarea, title ; RAWTEXT :
    // xmp, noembed, noframes, iframe, script) : une valeur d'attribut qui porte cette fin puis une balise
    // n'en fait sortir aucune.
    for (const char* element : {"textarea", "title", "xmp", "noembed", "noframes", "iframe", "script"}) {
        const QString name = QString::fromLatin1(element);
        QTest::newRow(qPrintable(QStringLiteral("fin de %1 dans une valeur").arg(name)))
            << QString::fromUtf8(
                   R"h(<%1><a title="</%1><meta http-equiv=refresh content='0;url=https://evil.example/r'>">x</a></%1>)h")
                   .arg(name);
    }
    // Là où la réécriture lit des balises et où le moteur peut lire du texte (`<textarea/>` reste ouvert en
    // HTML, noscript selon l'état de JavaScript) : seules les valeurs sans `<` ni `>` gardent la sortie.
    // L'adresse n'est pas d'evil.example (la première lecture lit aussi la valeur d'un title) : c'est la
    // balise meta que la seconde lecture cherche.
    row("textarea /",
        R"h(<textarea/><a title="</textarea><meta http-equiv=refresh content='0;url=https://t.example/r'>">x</a>)h");
    row("noscript",
        R"h(<noscript><a title="</noscript><meta http-equiv=refresh content='0;url=https://t.example/r'>">x</a></noscript>)h");
    row("meta quelconque",
        R"h(<meta name="description" content="d"><meta property="og:image" content="https://evil.example/og.png">)h");
    // Contenu étranger (SVG, MathML) : le moteur y rend les entités du <style>.
    row("style svg entite",
        R"h(<svg><style>.x{background:u&#114;l(https://evil.example/f.png)}</style></svg>)h");
    row("style svg entite nommee",
        R"h(<svg><style>.x{background:url&lpar;https://evil.example/n.png&rpar;}</style></svg>)h");
    row("style mathml",
        R"h(<math><mtext><table><mglyph><style>&#64;import "https://evil.example/m.css";</style></mglyph></table></mtext></math>)h");
}

void TstUiWebFrame::prepareHtmlLeavesNoUndecidedAddress()
{
    QFETCH(QString, html);
    for (const QUrl& base : {QUrl(QStringLiteral("https://ok.example/dir/page.html")), QUrl()}) {
        OnlyOk decide;
        const auto prepared = ui::WebFrameItems::prepareHtml(html, base, std::ref(decide));
        const QString read = addressBearing(prepared.html);
        QVERIFY2(!read.contains(QStringLiteral("evil")), qPrintable(prepared.html));
        for (const QString& span : naiveTags(prepared.html)) { // aucune balise meta hors de la règle de tête
            QVERIFY2(!span.startsWith(QStringLiteral("<meta")), qPrintable(prepared.html));
            QVERIFY2(!span.contains(QStringLiteral("evil")), qPrintable(prepared.html));
        }
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

// Contenu d'un élément RCDATA (textarea, title) : du texte pour le moteur, rendu ici en texte (`<` en `&lt;`,
// entités gardées), jamais lu comme balise ni demandé ; xmp (RAWTEXT, montré) devient un pre de texte échappé
// ; une valeur d'attribut ne garde ni `<` ni `>` ni guillemet ; une feuille sans entité garde ses `&` (CSS
// imbriqué).
void TstUiWebFrame::prepareHtmlKeepsRawTextAsText()
{
    const QString html = QString::fromUtf8(
        R"h(<textarea><img src="/t.png"> 1 < 2 &amp; 3</textarea><title>a<b</title><xmp><b>&</b></xmp>)h"
        R"h(<p title="x<y>z" lang='l"m'>p</p><style>.a{&:hover{color:red}}</style>)h");
    OnlyOk decide;
    const auto prepared =
        ui::WebFrameItems::prepareHtml(html, QUrl(QStringLiteral("https://ok.example/")), std::ref(decide));
    const QString& out = prepared.html;
    QVERIFY2(decide.asked.isEmpty(), qPrintable(decide.asked.join(u' ') + QStringLiteral(" | ") + out));
    for (const char* expected :
         {R"h(<textarea>&lt;img src="/t.png"> 1 &lt; 2 &amp; 3</textarea>)h", "<title>a&lt;b</title>",
          "<pre>&lt;b&gt;&amp;&lt;/b&gt;</pre>", R"h(title="x&lt;y&gt;z")h", R"h(lang="l&quot;m")h",
          "<style>.a{&:hover{color:red}}</style>"})
        QVERIFY2(out.contains(QString::fromUtf8(expected)),
                 qPrintable(QString::fromUtf8(expected) + QStringLiteral(" | ") + out));
}

// JavaScript coupé : un script n'est jamais demandé, ni par src ni par href (SVG), même vers une origine
// autorisée ; rien de lui n'entre dans la règle de sécurité du contenu (img-src compris).
void TstUiWebFrame::prepareHtmlDropsScripts()
{
    const QString html = QString::fromUtf8(
        R"h(<svg><script href="/s.js" xlink:href="/t.js">a()</script></svg><script src="/u.js"></script><p>reste</p>)h");
    OnlyOk decide;
    const auto prepared =
        ui::WebFrameItems::prepareHtml(html, QUrl(QStringLiteral("https://ok.example/")), std::ref(decide));
    QVERIFY2(decide.asked.isEmpty(), qPrintable(decide.asked.join(u' ')));
    QVERIFY2(prepared.allowed.isEmpty(), qPrintable(prepared.allowed.join(u' ')));
    QVERIFY2(!prepared.html.contains(QStringLiteral(".js")), qPrintable(prepared.html));
    QVERIFY2(prepared.html.contains(QStringLiteral("<p>reste</p>")), qPrintable(prepared.html));
}

// Réécriture linéaire, mesurée sur N puis 2N caractères d'une page piégée pour les motifs à retour arrière
// (commentaires, styles et balises jamais fermés) : le temps double, il ne quadruple pas (rapport < 3,
// meilleure de deux passes, plancher de 20 ms). Les temps sont imprimés (preuve).
void TstUiWebFrame::prepareHtmlStaysLinear()
{
    const QString trap = QStringLiteral("<a b=c/d/e/f/g <style x <!--x url(\"q ");
    const auto page = [&trap](qsizetype size) {
        QString big = bigPage(size / 2, QStringLiteral("x"));
        big.reserve(size + trap.size());
        while (big.size() < size)
            big += trap;
        return big;
    };
    const auto best = [](const QString& html) {
        qint64 ms = std::numeric_limits<qint64>::max();
        for (int pass = 0; pass < 2; ++pass) {
            OnlyOk decide;
            QElapsedTimer timer;
            timer.start();
            const auto prepared = ui::WebFrameItems::prepareHtml(
                html, QUrl(QStringLiteral("https://ok.example/")), std::ref(decide));
            ms = std::min(ms, timer.elapsed());
            if (!prepared.html.contains(QStringLiteral("https://ok.example/c.png")) ||
                prepared.html.contains(QStringLiteral("<!--")))
                return qint64(-1);
        }
        return ms;
    };
    const qsizetype n = 2 * 1024 * 1024;
    const qint64 once = best(page(n));
    const qint64 twice = best(page(2 * n));
    qInfo("prepareHtml : %lld ms pour %lld caracteres, %lld ms pour le double", static_cast<long long>(once),
          static_cast<long long>(n), static_cast<long long>(twice));
    QVERIFY(once >= 0 && twice >= 0);
    QVERIFY2(twice < 3 * std::max<qint64>(once, 20),
             qPrintable(QStringLiteral("%1 ms puis %2 ms").arg(once).arg(twice)));
}

// Page de 4 Mo saisie : insertFrame rend la main avant la réécriture, faite hors du fil de l'interface (BRIEF
// § 4.1) ; le contenu réécrit arrive ensuite par le modèle.
void TstUiWebFrame::rewriteLeavesTheInterfaceThreadAtOnce()
{
    const QString big = bigPage(4 * 1024 * 1024, QStringLiteral("FIN-DE-PAGE"));
    Fixture f;
    QElapsedTimer timer;
    timer.start();
    QVERIFY(f.frames.insertFrame(big, {}));
    const qint64 ms = timer.elapsed();
    qInfo("insertFrame : %lld ms pour %lld caracteres", static_cast<long long>(ms),
          static_cast<long long>(big.size()));
    QVERIFY2(ms < 50, qPrintable(QString::number(ms)));
    QCOMPARE(f.frames.count(), 1);
    QTRY_VERIFY_WITH_TIMEOUT(content(f.frames, 0).contains(QStringLiteral("<p>FIN-DE-PAGE</p>")), 60000);
    QVERIFY(!role(f.frames, 0, ui::WebFrameItems::LoadingRole).toBool());
}

// Cadre retiré pendant la réécriture, un autre posé à sa place : le résultat arrivé après coup est abandonné
// (même règle que la réponse réseau d'un cadre passé à une autre adresse).
void TstUiWebFrame::staleRewriteIsDropped()
{
    Fixture f;
    QVERIFY(f.frames.insertFrame(bigPage(2 * 1024 * 1024, QStringLiteral("PREMIER")), {}));
    f.document.undo();
    QCOMPARE(f.frames.count(), 0);
    QVERIFY(f.frames.insertFrame(QStringLiteral("<p>SECOND</p>"), {}));
    QTRY_VERIFY_WITH_TIMEOUT(content(f.frames, 0).contains(QStringLiteral("SECOND")), 60000);
    QVERIFY(QThreadPool::globalInstance()->waitForDone(60000));
    QTest::qWait(100); // résultats en file livrés
    QCOMPARE(f.frames.count(), 1);
    QVERIFY(content(f.frames, 0).contains(QStringLiteral("SECOND")));
    QVERIFY(!content(f.frames, 0).contains(QStringLiteral("PREMIER")));
    // Même cadre relu deux fois avant le retour : seule la dernière réécriture est rendue (la vue recharge
    // une fois).
    const int serial = role(f.frames, 0, ui::WebFrameItems::SerialRole).toInt();
    f.frames.reload(0);
    f.frames.reload(0);
    QVERIFY(QThreadPool::globalInstance()->waitForDone(60000));
    QTest::qWait(100);
    QCOMPARE(role(f.frames, 0, ui::WebFrameItems::SerialRole).toInt(), serial + 1);
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
    QTRY_VERIFY2_WITH_TIMEOUT(content(writer.frames, 1).contains(image),
                              qPrintable(content(writer.frames, 1)),
                              10000); // origine saisie
    QTRY_VERIFY_WITH_TIMEOUT(server.requests() >= 1, 5000);
    Fixture reader;
    QVERIFY(saveAndReopen(writer, reader, dir.filePath(QStringLiteral("code.actcraft"))));
    QTRY_VERIFY_WITH_TIMEOUT(role(reader.frames, 1, ui::WebFrameItems::BlockedRole).toBool(), 10000);
    QVERIFY(!content(reader.frames, 1).contains(image));
    reader.frames.requestUnblock(1);
    QVERIFY(reader.frames.unblockPending());
    reader.frames.answer(true);
    QVERIFY(!role(reader.frames, 1, ui::WebFrameItems::BlockedRole).toBool());
    QTRY_VERIFY2_WITH_TIMEOUT(content(reader.frames, 1).contains(image),
                              qPrintable(content(reader.frames, 1)), 10000);
}

// Un href hors <a>, <area>, <link> et des éléments SVG à ressource (ici MathML) est un lien pour le moteur :
// il passe par la voie des liens (renvoi interne), l'adresse absolue n'est jamais gardée.
void TstUiWebFrame::hrefOutsideSvgResourceBecomesALink()
{
    OnlyOk decide;
    const auto prepared = ui::WebFrameItems::prepareHtml(
        QStringLiteral(
            R"h(<math><mi href="https://ok.example/m" xlink:href="https://ok.example/x">clic</mi></math>)h"
            R"h(<svg><image href="https://ok.example/i.png"/></svg>)h"),
        QUrl(QStringLiteral("https://ok.example/")), std::ref(decide));
    const qsizetype from = prepared.html.indexOf(QStringLiteral("<math"));
    const QString mathml = prepared.html.mid(from, prepared.html.indexOf(QStringLiteral("</math>")) - from);
    QVERIFY2(!mathml.contains(QStringLiteral("ok.example")), qPrintable(prepared.html));
    QVERIFY2(mathml.contains(QStringLiteral("href=\"#")), qPrintable(prepared.html));
    QVERIFY2(prepared.html.contains(QStringLiteral("<image href=\"https://ok.example/i.png\"")),
             qPrintable(prepared.html));
}

// Déblocage confirmé pendant la réécriture d'un gros cadre : la réécriture en cours (faite avec la politique
// d'avant) est refaite, le cadre rendu laisse passer l'image.
void TstUiWebFrame::unblockDuringRewriteHolds()
{
    TestServer server;
    QVERIFY(server.listening());
    server.page(QStringLiteral("/page"), "<p>distant</p>");
    const QString image = server.url(QStringLiteral("/fin.png")).toString();
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    Fixture writer;
    QVERIFY(writer.frames.insertFrame({}, server.url(QStringLiteral("/page")).toString()));
    QVERIFY(writer.frames.insertFrame(
        bigPage(2 * 1024 * 1024, QStringLiteral("FIN")) + QStringLiteral("<img src=\"%1\">").arg(image), {}));
    QTRY_VERIFY_WITH_TIMEOUT(content(writer.frames, 1).contains(QStringLiteral("<p>FIN</p>")), 60000);
    QVERIFY(QThreadPool::globalInstance()->waitForDone(60000));
    Fixture reader;
    QVERIFY(saveAndReopen(writer, reader, dir.filePath(QStringLiteral("gros.actcraft"))));
    QCOMPARE(reader.frames.count(), 2);
    QVERIFY(role(reader.frames, 0, ui::WebFrameItems::BlockedRole).toBool());
    QVERIFY(role(reader.frames, 1, ui::WebFrameItems::LoadingRole).toBool()); // réécriture en cours
    reader.frames.requestUnblock(0);
    QVERIFY(reader.frames.unblockPending());
    reader.frames.answer(true);
    QTRY_VERIFY_WITH_TIMEOUT(!role(reader.frames, 1, ui::WebFrameItems::LoadingRole).toBool(), 60000);
    QVERIFY(QThreadPool::globalInstance()->waitForDone(60000));
    QTest::qWait(100);
    QVERIFY2(content(reader.frames, 1).contains(image), "image decidee apres deblocage");
    QVERIFY(!role(reader.frames, 1, ui::WebFrameItems::BlockedRole).toBool());
}

namespace {

// Profil d'essai, construit comme tst_ui_classroomlock : aucun réglage du poste touché.
struct ProfileFixture
{
    QTemporaryDir dir;
    ui::Commands commands;
    std::unique_ptr<ui::Preferences> preferences;
    std::unique_ptr<ui::ProfileController> profile;
    ProfileFixture()
    {
        QSettings::setDefaultFormat(QSettings::IniFormat);
        QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, dir.path());
        preferences = std::make_unique<ui::Preferences>();
        actions::ProfileStore store(dir.filePath(QStringLiteral("profiles")),
                                    QStringLiteral(":/actcraft/actions/profiles"),
                                    ui::ProfileController::builtinOrder());
        profile = std::make_unique<ui::ProfileController>(std::move(store), preferences.get(), &commands);
        profile->start();
    }
};

} // namespace

// Effet immédiat, sans relancer (S3, E12) : la ligne est relue et son `serial` avance.
void TstUiWebFrame::guardsFollowTheProfileSetting()
{
    ProfileFixture p;
    Fixture f(false);
    QVERIFY(f.frames.insertFrame(QStringLiteral("<p>a</p>"), {}));
    QTRY_VERIFY(content(f.frames, 0).contains(QStringLiteral("a")));
    f.frames.setProfile(p.profile.get());
    QVERIFY(!f.frames.guardsEnabled());
    QSignalSpy changed(&f.frames, &ui::WebFrameItems::guardsEnabledChanged);
    const int before = role(f.frames, 0, ui::WebFrameItems::SerialRole).toInt();
    p.profile->setSetting(QStringLiteral("tools"), QStringLiteral("web-frame-guards"), true);
    QVERIFY(f.frames.guardsEnabled());
    QCOMPARE(changed.count(), 1);
    QTRY_VERIFY(role(f.frames, 0, ui::WebFrameItems::SerialRole).toInt() > before);
}

// `profileChanged` part aussi à chaque couleur ou outil récent (cadrage n° 10) : valeur inchangée, rien.
void TstUiWebFrame::unrelatedProfileChangeReloadsNothing()
{
    ProfileFixture p;
    Fixture f(false);
    QVERIFY(f.frames.insertFrame(QStringLiteral("<p>a</p>"), {}));
    QTRY_VERIFY(content(f.frames, 0).contains(QStringLiteral("a")));
    f.frames.setProfile(p.profile.get());
    QTest::qWait(100);
    const int before = role(f.frames, 0, ui::WebFrameItems::SerialRole).toInt();
    p.profile->pushRecentTool(QStringLiteral("pen"));
    QTest::qWait(200);
    QCOMPARE(role(f.frames, 0, ui::WebFrameItems::SerialRole).toInt(), before);
}

int main(int argc, char** argv)
{
    actcraft::ui::prepareWebFrameEngine();
    QGuiApplication app(argc, argv);
    TstUiWebFrame test;
    return QTest::qExec(&test, argc, argv);
}

#include "tst_ui_webframe.moc"
