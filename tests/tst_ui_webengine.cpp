// Le vrai moteur (cadrage n° 30) : lancé hors écran sur Mac, saut nommé sous Windows hors écran (Reste PC 4).
#include "startup.h"
#include "support/presentationharness.h"
#include "support/testserver.h"
#include "support/webframehost.h"

#include <actcraft/ui/webframeengine.h>

#include <QClipboard>
#include <QFile>
#include <QGuiApplication>
#include <QPointer>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QtQml/qqmlextensionplugin.h>
#include <QtTest>

#include <memory>

Q_IMPORT_QML_PLUGIN(ActCraft_UiPlugin)

using actcraft::ui::WebFrameItems;
using presentationharness::findItem;

namespace {

// La vue seule : réglages du moteur, sans modèle ni couche.
struct DirectView
{
    QQmlEngine engine;
    QQuickWindow window;
    std::unique_ptr<QQuickItem> root;
    QQuickItem* web = nullptr;

    bool start(bool guards)
    {
        window.resize(640, 480);
        window.show();
        if (!QTest::qWaitForWindowExposed(&window))
            return false;
        QQmlComponent component(&engine, "ActCraft.Ui", "WebFrameEngineView");
        root.reset(qobject_cast<QQuickItem*>(
            component.createWithInitialProperties({{QStringLiteral("guardsEnabled"), guards}})));
        if (!root) {
            qWarning() << component.errors();
            return false;
        }
        root->setSize(QSizeF(640, 480));
        root->setParentItem(window.contentItem());
        web = findItem(root.get(), QStringLiteral("webFrameEngine"));
        return web != nullptr;
    }
    void load(const QString& html, const QUrl& base = {})
    {
        root->setProperty("html", html);
        root->setProperty("baseUrl", base);
        QMetaObject::invokeMethod(root.get(), "load");
    }
    void loadFile(const QUrl& file)
    {
        root->setProperty("contentFile", file);
        QMetaObject::invokeMethod(root.get(), "load");
    }
    QString title() const { return web->property("title").toString(); }
};

const int kLoadMs = 60000;

// Suit les demandes de téléchargement du profil par défaut : la vue n'en accepte aucune. `acceptIt` sert de
// témoin positif (la demande acceptée écrit bien un fichier, donc l'observable discrimine).
struct DownloadWatch
{
    std::unique_ptr<QObject> owner;

    bool start(QQmlEngine& engine, const QString& dir, bool acceptIt)
    {
        QQmlComponent component(&engine);
        component.setData(QByteArray("import QtQuick\nimport QtWebEngine\nQtObject {\n"
                                     "  property bool acceptIt: false\n  property string dir: \"\"\n"
                                     "  property int requests: 0\n"
                                     "  Component.onCompleted: WebEngine.defaultProfile.downloadPath = dir\n"
                                     "  property Connections watch: Connections {\n"
                                     "    target: WebEngine.defaultProfile\n"
                                     "    function onDownloadRequested(request) {\n"
                                     "      requests += 1\n"
                                     "      if (acceptIt) request.accept()\n"
                                     "    }\n"
                                     "  }\n}\n"),
                          QUrl());
        owner.reset(component.createWithInitialProperties(
            {{QStringLiteral("acceptIt"), acceptIt}, {QStringLiteral("dir"), dir}}));
        if (!owner) {
            qWarning() << component.errors();
            return false;
        }
        return true;
    }
    int requests() const { return owner->property("requests").toInt(); }
};

QVariant roleOf(const WebFrameHost& host, int role)
{
    return host.frames->data(host.frames->index(0, 0), role);
}

QObject* profileOf(QQuickItem* web)
{
    return web->property("profile").value<QObject*>();
}

} // namespace

class TstUiWebEngine : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase()
    {
#ifdef Q_OS_WIN
        if (QGuiApplication::platformName() == QLatin1String("offscreen"))
            QSKIP("Qt WebEngine hors écran sous Windows : preuve au PC (Reste PC 4)");
#endif
    }

    // prepareWebFrameEngine() partage les contextes GL avant QGuiApplication : sa perte doit faire tomber un
    // test.
    void shareOpenGLContextsIsSet() { QVERIFY(QCoreApplication::testAttribute(Qt::AA_ShareOpenGLContexts)); }

    // Premier test du fichier : l'environnement est relu au lancement du premier moteur.
    void prepareSetsTheProcessLimitOnlyWhenEmpty()
    {
        qunsetenv("QTWEBENGINE_CHROMIUM_FLAGS");
        actcraft::ui::prepareWebFrameEngine();
        QCOMPARE(qgetenv("QTWEBENGINE_CHROMIUM_FLAGS"), QByteArray("--renderer-process-limit=2"));
        qputenv("QTWEBENGINE_CHROMIUM_FLAGS", "--autre");
        actcraft::ui::prepareWebFrameEngine();
        QCOMPARE(qgetenv("QTWEBENGINE_CHROMIUM_FLAGS"), QByteArray("--autre"));
        qputenv("QTWEBENGINE_CHROMIUM_FLAGS", "--renderer-process-limit=2");
    }

    // Aucun coût tant qu'aucun cadre n'est montré (O146).
    void noViewBeforeTheFirstFrame()
    {
        presentationharness::MainWindow main;
        QVERIFY(main.window());
        QVERIFY(main.item(QStringLiteral("webFrameLayer")));
        QCOMPARE(main.item(QStringLiteral("webFrameEngine")), nullptr);
    }

    // Preuve réelle : Loader dans le module statique, import différé, vraie page chargée.
    void realFrameLoadsThroughTheLoader()
    {
        presentationharness::MainWindow main;
        QVERIFY(main.window());
        QQuickItem* layer = main.item(QStringLiteral("webFrameLayer"));
        QVERIFY(layer);
        QObject* frames = layer->findChild<QObject*>(QStringLiteral("appWebFrames"));
        QVERIFY(frames);
        bool inserted = false;
        QVERIFY(QMetaObject::invokeMethod(frames, "insertFrame", Q_RETURN_ARG(bool, inserted),
                                          Q_ARG(QString, QStringLiteral("<title>CADRE OK</title><p>x</p>")),
                                          Q_ARG(QString, QString())));
        QVERIFY(inserted);
        QTRY_COMPARE_WITH_TIMEOUT(layer->property("loadedViews").toInt(), 1, 60000);
        QQuickItem* web = main.item(QStringLiteral("webFrameEngine"));
        QVERIFY(web);
        QCOMPARE(web->property("title").toString(), QStringLiteral("CADRE OK"));
    }

    // Module absent du paquet : même branche qu'aujourd'hui, avertissement et cadre vide, sans plantage (n°
    // 5).
    void loaderErrorLeavesAnEmptyFrame()
    {
        WebFrameHost host;
        QVERIFY(host.start());
        host.layer->setProperty("viewSource", QUrl::fromLocalFile(QStringLiteral(
                                                  ACTCRAFT_SOURCE_DIR "/tests/qml/AbsentModuleView.qml")));
        QTest::ignoreMessage(QtWarningMsg,
                             QRegularExpression(QStringLiteral(".*Cadre web : vue indisponible.*")));
        QVERIFY(host.frames->insertFrame(QStringLiteral("<p>a</p>"), {}));
        QTRY_VERIFY_WITH_TIMEOUT(findItem(host.root.get(), QStringLiteral("webFrameLoader")), 5000);
        QQuickItem* loader = findItem(host.root.get(), QStringLiteral("webFrameLoader"));
        QTRY_COMPARE_WITH_TIMEOUT(loader->property("status").toInt(), 3 /* Loader.Error */, 10000);
        QCOMPARE(findItem(host.root.get(), QStringLiteral("webFrameView")), nullptr);
    }

    // S1 : le script d'une page locale s'exécute (gardes coupées, le défaut).
    void javaScriptRunsGuardsOff()
    {
        WebFrameHost host;
        QVERIFY(host.start());
        QVERIFY(host.frames->insertFrame(
            QStringLiteral("<title>avant</title><script>document.title='JS OK'</script>"), {}));
        QTRY_COMPARE_WITH_TIMEOUT(host.layer->property("loadedViews").toInt(), 1, kLoadMs);
        QQuickItem* web = findItem(host.root.get(), QStringLiteral("webFrameEngine"));
        QVERIFY(web);
        QTRY_COMPARE_WITH_TIMEOUT(web->property("title").toString(), QStringLiteral("JS OK"), 10000);
    }

    // S3 : gardes allumées, JavaScript coupé sur le vrai moteur.
    void javaScriptIsOffGuardsOn()
    {
        DirectView view;
        QVERIFY(view.start(true));
        QSignalSpy loaded(view.root.get(), SIGNAL(pageLoaded()));
        view.load(QStringLiteral("<title>avant</title><script>document.title='JS'</script>"));
        QTRY_COMPARE_WITH_TIMEOUT(loaded.count(), 1, kLoadMs);
        QTest::qWait(300);
        QCOMPARE(view.title(), QStringLiteral("avant"));
    }

    // n° 11, 12, E11 : document d'origine file:, image distante chargée gardes coupées, refusée gardes
    // allumées. Gardes allumées, load() n'utilise pas la base (document sans base : l'image y passerait) :
    // seul le fichier du HTML lourd (contentFile) donne cette origine.
    void remoteImageFromAFileOrigin_data()
    {
        QTest::addColumn<bool>("guards");
        QTest::addColumn<bool>("viaFile");
        QTest::addColumn<int>("requests");
        QTest::newRow("base file, gardes coupees") << false << false << 1;
        QTest::newRow("contentFile, gardes coupees") << false << true << 1;
        QTest::newRow("contentFile, gardes allumees") << true << true << 0;
    }

    void remoteImageFromAFileOrigin()
    {
        QFETCH(bool, guards);
        QFETCH(bool, viaFile);
        QFETCH(int, requests);
        TestServer server;
        QVERIFY(server.listening());
        server.page(QStringLiteral("/p.png"), "x");
        const QString html =
            QStringLiteral("<img src=\"%1\">").arg(server.url(QStringLiteral("/p.png")).toString());
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        DirectView view;
        QVERIFY(view.start(guards));
        QSignalSpy loaded(view.root.get(), SIGNAL(pageLoaded()));
        if (viaFile) {
            QFile file(dir.filePath(QStringLiteral("cadre.html")));
            QVERIFY(file.open(QIODevice::WriteOnly));
            file.write(html.toUtf8());
            file.close();
            view.loadFile(QUrl::fromLocalFile(file.fileName()));
        } else {
            view.load(html, QUrl::fromLocalFile(dir.path() + u'/'));
        }
        QTRY_COMPARE_WITH_TIMEOUT(loaded.count(), 1, kLoadMs);
        QTest::qWait(800);
        QCOMPARE(server.requests(), requests);
    }

    // n° 18 : image réelle du cadre, posée par la commande, une étape.
    void snapshotInsertsAnImage()
    {
        presentationharness::MainWindow main;
        QVERIFY(main.window());
        QQuickItem* layer = main.item(QStringLiteral("webFrameLayer"));
        QVERIFY(layer);
        QObject* frames = layer->findChild<QObject*>(QStringLiteral("appWebFrames"));
        QVERIFY(frames);
        bool inserted = false;
        QVERIFY(QMetaObject::invokeMethod(
            frames, "insertFrame", Q_RETURN_ARG(bool, inserted),
            Q_ARG(QString, QStringLiteral("<body style='background:#c81e1e'>x")), Q_ARG(QString, QString())));
        QTRY_COMPARE_WITH_TIMEOUT(layer->property("loadedViews").toInt(), 1, kLoadMs);
        QVERIFY(main.waitReady());
        const int before = main.pageObjectCount();
        QVERIFY(main.run(QStringLiteral("webframe.snapshot")));
        QTRY_COMPARE_WITH_TIMEOUT(layer->property("snapshotCount").toInt(), 1, 20000);
        QTRY_COMPARE_WITH_TIMEOUT(main.pageObjectCount(), before + 1, 10000);
    }

    // Revue de T5 : pageLoaded précède la peinture. L'image posée porte les pixels de la page (carré bleu en
    // haut à gauche, fond rouge ailleurs), pas seulement une taille non nulle.
    void snapshotWaitsForPaintedPixels()
    {
        WebFrameHost host;
        QVERIFY(host.start());
        QVERIFY(host.frames->insertFrame(
            QStringLiteral("<style>html,body{margin:0;height:100%}body{background:#c81e1e}"
                           "div{position:absolute;left:0;top:0;width:40px;height:40px;background:#1e1ec8}"
                           "</style><div></div>"),
            {}));
        QTRY_COMPARE_WITH_TIMEOUT(host.layer->property("loadedViews").toInt(), 1, kLoadMs);
        const int before = host.document.shownObjectCount();
        QVERIFY(QMetaObject::invokeMethod(host.layer, "takeSnapshot", Q_ARG(QVariant, QVariant())));
        QTRY_COMPARE_WITH_TIMEOUT(host.layer->property("snapshotCount").toInt(), 1, 20000);
        QCOMPARE(host.document.shownObjectCount(), before + 1);
        const auto& resources = host.document.document().resources();
        QCOMPARE(resources.size(), std::size_t(1));
        const QImage image =
            QImage::fromData(resources.begin()->second.data, "PNG").convertToFormat(QImage::Format_RGB32);
        QVERIFY(image.width() > 60 && image.height() > 60);
        QCOMPARE(image.pixelColor(8, 8), QColor(0x1e, 0x1e, 0xc8));
        QCOMPARE(image.pixelColor(image.width() - 8, image.height() - 8), QColor(0xc8, 0x1e, 0x1e));
    }

    // n° 38, E11 : 3 Mo chargés par le fichier temporaire, gardes coupées et allumées.
    void heavyHtmlLoads_data()
    {
        QTest::addColumn<bool>("guards");
        QTest::newRow("gardes coupees") << false;
        QTest::newRow("gardes allumees") << true;
    }

    void heavyHtmlLoads()
    {
        QFETCH(bool, guards);
        WebFrameHost host;
        QVERIFY(host.start());
        host.frames->setGuardsEnabled(guards);
        QVERIFY(host.frames->insertFrame(
            QStringLiteral("<title>GROS</title><body>") + QString(3 * 1024 * 1024, u'a'), {}));
        QTRY_COMPARE_WITH_TIMEOUT(host.layer->property("loadedViews").toInt(), 1, kLoadMs);
        QQuickItem* web = findItem(host.root.get(), QStringLiteral("webFrameEngine"));
        QVERIFY(web);
        QTRY_COMPARE_WITH_TIMEOUT(web->property("title").toString(), QStringLiteral("GROS"), 10000);
    }

    // OB52, erreur 153 : un HTML lourd qui intègre le lecteur YouTube garde l'origine https de l'identité (le
    // Referer de l'iframe /embed/), pas celle du fichier ; un rechargement la garde. Sans réseau : l'adresse
    // du lecteur n'est qu'une chaîne, elle suffit à la détection.
    void heavyPlayerHtmlKeepsTheIdentityOrigin()
    {
        WebFrameHost host;
        QVERIFY(host.start());
        QVERIFY(host.frames->insertFrame(
            QStringLiteral("<body><p id=a>") + QString(3 * 1024 * 1024, u'a') +
                QStringLiteral(
                    "</p><script>var player='https://www.youtube-nocookie.com/embed/x';document.title="
                    "location.origin+' '+document.getElementById('a').textContent.length</script>"),
            {}));
        QTRY_COMPARE_WITH_TIMEOUT(host.layer->property("loadedViews").toInt(), 1, kLoadMs);
        QQuickItem* view = findItem(host.root.get(), QStringLiteral("webFrameView"));
        QQuickItem* web = findItem(host.root.get(), QStringLiteral("webFrameEngine"));
        QVERIFY(view && web);
        QVERIFY(!view->property("contentFile").toUrl().isEmpty()); // le chemin lourd, pas loadHtml
        const QString expected = QStringLiteral("https://org.actcraft.actcraft %1").arg(3 * 1024 * 1024);
        QTRY_COMPARE_WITH_TIMEOUT(web->property("title").toString(), expected, 10000);
        QSignalSpy loaded(view, SIGNAL(pageLoaded()));
        host.frames->reload(0);
        QTRY_COMPARE_WITH_TIMEOUT(loaded.count(), 1, kLoadMs);
        QTRY_COMPARE_WITH_TIMEOUT(web->property("title").toString(), expected, 10000);
    }

    // S3 : un cookie posé dans le document A est absent du document B, gardes allumées. Le cookie vient de la
    // réponse du document principal : en sous-ressource d'un document sans base, Chromium le traiterait comme
    // SameSite=Lax et ne le stockerait pas. La couche ne donne pas d'adresse à la vue gardes allumées : deux
    // vues directes prouvent l'étanchéité des profils, la seconde partie prouve qu'un autre document a bien
    // une autre vue et un autre profil.
    void cookieStaysInItsDocument()
    {
        TestServer server;
        QVERIFY(server.listening());
        server.page(QStringLiteral("/doc"), "<title>DOC</title>");
        server.header(QStringLiteral("/doc"), "Set-Cookie: marque=oui");
        const QUrl address = server.url(QStringLiteral("/doc"));
        {
            DirectView a;
            QVERIFY(a.start(true));
            QSignalSpy loaded(a.root.get(), SIGNAL(pageLoaded()));
            a.root->setProperty("directUrl", address);
            QMetaObject::invokeMethod(a.root.get(), "load");
            QTRY_COMPARE_WITH_TIMEOUT(loaded.count(), 1, kLoadMs);
            QVERIFY(!server.lastRequest(QStringLiteral("/doc")).contains("marque=oui"));
            // Même vue, même profil : le cookie repart. Si le test expire ici (la même `url` réaffectée ne
            // relance pas le moteur), appeler "reload" du contrat à la place de "load" (web.reload()).
            QMetaObject::invokeMethod(a.root.get(), "load");
            QTRY_COMPARE_WITH_TIMEOUT(loaded.count(), 2, kLoadMs);
            QVERIFY2(server.lastRequest(QStringLiteral("/doc")).contains("marque=oui"), "cookie non stocké");
        }
        {
            DirectView b;
            QVERIFY(b.start(true));
            QSignalSpy loaded(b.root.get(), SIGNAL(pageLoaded()));
            b.root->setProperty("directUrl", address);
            QMetaObject::invokeMethod(b.root.get(), "load");
            QTRY_COMPARE_WITH_TIMEOUT(loaded.count(), 1, kLoadMs);
            QVERIFY(!server.lastRequest(QStringLiteral("/doc")).contains("marque=oui"));
        }
        WebFrameHost host;
        QVERIFY(host.start());
        host.frames->setGuardsEnabled(true);
        QVERIFY(host.frames->insertFrame(QStringLiteral("<p>A</p>"), {}));
        QTRY_COMPARE_WITH_TIMEOUT(host.layer->property("loadedViews").toInt(), 1, kLoadMs);
        QPointer<QQuickItem> viewA = findItem(host.root.get(), QStringLiteral("webFrameEngine"));
        QVERIFY(viewA);
        QPointer<QObject> profileA = viewA->property("profile").value<QObject*>();
        QVERIFY(profileA);
        // Profil éphémère : rien n'est écrit sur le disque, quelle que soit sa création (n° 33).
        QVERIFY(profileA->property("offTheRecord").toBool());
        host.document.newDocument(1024, 768);
        QVERIFY(host.frames->insertFrame(QStringLiteral("<p>B</p>"), {}));
        QTRY_VERIFY_WITH_TIMEOUT(viewA.isNull() && profileA.isNull(), 5000);
        QTRY_VERIFY_WITH_TIMEOUT(findItem(host.root.get(), QStringLiteral("webFrameEngine")), kLoadMs);
        QCOMPARE(
            findItem(host.root.get(), QStringLiteral("webFrameView"))->property("guardsEnabled").toBool(),
            true);
    }

    // E2 : la vue reste visible sous une boîte ouverte.
    void realViewStaysVisibleUnderAPopup()
    {
        WebFrameHost host;
        QVERIFY(host.start());
        QVERIFY(host.frames->insertFrame(QStringLiteral("<p>x</p>"), {}));
        QTRY_COMPARE_WITH_TIMEOUT(host.layer->property("loadedViews").toInt(), 1, kLoadMs);
        host.root->setProperty("inputBlocked", true);
        QVERIFY(findItem(host.root.get(), QStringLiteral("webFrameViewHost"))->isVisible());
    }

    // E8 : target=_blank s'ouvre dans la même vue.
    void popupOpensInTheSameView()
    {
        TestServer server;
        QVERIFY(server.listening());
        server.page(QStringLiteral("/popup"), "<title>SURGIE</title>");
        DirectView view;
        QVERIFY(view.start(false));
        view.load(QStringLiteral("<a id=l target=_blank href=\"%1\">x</a><script>l.click()</script>")
                      .arg(server.url(QStringLiteral("/popup")).toString()));
        QTRY_COMPARE_WITH_TIMEOUT(view.title(), QStringLiteral("SURGIE"), kLoadMs);
    }

    // E8 : permission refusée (géolocalisation, erreur 1 = PERMISSION_DENIED).
    void permissionIsDenied()
    {
        TestServer server;
        QVERIFY(server.listening());
        server.page(QStringLiteral("/geo"),
                    "<script>navigator.geolocation.getCurrentPosition(function(){document.title='OK'},"
                    "function(e){document.title='REFUS'+e.code})</script>");
        DirectView view;
        QVERIFY(view.start(false));
        view.root->setProperty("directUrl", server.url(QStringLiteral("/geo")));
        QMetaObject::invokeMethod(view.root.get(), "load");
        QTRY_COMPARE_WITH_TIMEOUT(view.title(), QStringLiteral("REFUS1"), kLoadMs);
    }

    // E8 : un téléchargement lancé par la page est ignoré ; la page reste. L'observable : un fichier écrit si
    // la demande est acceptée (témoin positif), absent sinon ; la demande a bien été faite dans les deux cas.
    void pageDownloadIsIgnored_data()
    {
        QTest::addColumn<bool>("acceptIt");
        QTest::newRow("ignore (le comportement de la vue)") << false;
        QTest::newRow("temoin : accepte") << true;
    }

    void pageDownloadIsIgnored()
    {
        QFETCH(bool, acceptIt);
        TestServer server;
        QVERIFY(server.listening());
        server.page(QStringLiteral("/f.bin"), "x");
        // Vraie réponse de téléchargement : depuis un document sans origine, un <a download> seul serait
        // suivi comme un lien (attribut ignoré hors de la même origine) et le test ne prouverait rien.
        server.header(QStringLiteral("/f.bin"), "Content-Disposition: attachment; filename=f.bin");
        QTemporaryDir downloads;
        QVERIFY(downloads.isValid());
        DirectView view;
        DownloadWatch watch;
        QVERIFY(watch.start(view.engine, downloads.path(), acceptIt));
        QVERIFY(view.start(false));
        QSignalSpy loaded(view.root.get(), SIGNAL(pageLoaded()));
        view.load(
            QStringLiteral(
                "<title>RESTE</title><a id=l download "
                "href=\"%1\">x</a><script>onload=function(){setTimeout(function(){l.click()},0)}</script>")
                .arg(server.url(QStringLiteral("/f.bin")).toString()));
        QTRY_COMPARE_WITH_TIMEOUT(loaded.count(), 1, kLoadMs);
        QTRY_VERIFY_WITH_TIMEOUT(server.requests() >= 1, 10000); // le téléchargement a bien été demandé
        QTRY_VERIFY_WITH_TIMEOUT(watch.requests() >= 1, 10000);
        const QString written = downloads.filePath(QStringLiteral("f.bin"));
        if (acceptIt) {
            QTRY_VERIFY_WITH_TIMEOUT(QFile::exists(written), 10000);
        } else {
            QTest::qWait(1000);
            QVERIFY(!QFile::exists(written));
            QVERIFY(QDir(downloads.path()).isEmpty());
        }
        QCOMPARE(view.title(), QStringLiteral("RESTE"));
    }

    // n° 11, 12 : chaque réglage du moteur suit l'interrupteur (jamais une valeur constante).
    void engineSettingsFollowTheGuards_data()
    {
        QTest::addColumn<bool>("guards");
        QTest::newRow("gardes coupees") << false;
        QTest::newRow("gardes allumees") << true;
    }

    void engineSettingsFollowTheGuards()
    {
        QFETCH(bool, guards);
        DirectView view;
        QVERIFY(view.start(guards));
        QObject* settings = view.web->property("settings").value<QObject*>();
        QVERIFY(settings);
        for (const char* name : {"javascriptEnabled", "localStorageEnabled", "localContentCanAccessFileUrls",
                                 "localContentCanAccessRemoteUrls"})
            QVERIFY2(settings->property(name).toBool() == !guards, name);
    }

    // Gardes coupées (état par défaut), vrai moteur : clic sur un lien, titre et historique rapportés au
    // modèle, précédent, suivant, recharger.
    void realNavigationReportsToTheModel()
    {
        TestServer server;
        QVERIFY(server.listening());
        server.page(
            QStringLiteral("/a"),
            "<title>PAGE A</title><style>body{margin:0}a{display:block;width:100%;height:100%}</style>"
            "<a id=l href=\"/b\">suite</a>");
        server.page(QStringLiteral("/b"), "<title>PAGE B</title><p>b</p>");
        WebFrameHost host;
        QVERIFY(host.start());
        QVERIFY(!host.frames->guardsEnabled());
        QVERIFY(host.frames->insertFrame(QString(), server.url(QStringLiteral("/a")).toString()));
        QTRY_COMPARE_WITH_TIMEOUT(host.layer->property("loadedViews").toInt(), 1, kLoadMs);
        QTRY_COMPARE_WITH_TIMEOUT(roleOf(host, WebFrameItems::TitleRole).toString(), QStringLiteral("PAGE A"),
                                  10000);
        QVERIFY(!roleOf(host, WebFrameItems::CanGoBackRole).toBool());
        QQuickItem* web = findItem(host.root.get(), QStringLiteral("webFrameEngine"));
        QVERIFY(web);
        // Vrai clic de souris : un clic scripté (sans geste de l'utilisateur) ne laisse pas d'entrée
        // d'historique.
        QTest::mouseClick(&host.window, Qt::LeftButton, {}, web->mapToScene(QPointF(30, 30)).toPoint());
        QTRY_COMPARE_WITH_TIMEOUT(roleOf(host, WebFrameItems::TitleRole).toString(), QStringLiteral("PAGE B"),
                                  10000);
        QTRY_VERIFY_WITH_TIMEOUT(roleOf(host, WebFrameItems::CanGoBackRole).toBool(), 10000);
        host.frames->goBack(0);
        QTRY_COMPARE_WITH_TIMEOUT(roleOf(host, WebFrameItems::TitleRole).toString(), QStringLiteral("PAGE A"),
                                  10000);
        QTRY_VERIFY_WITH_TIMEOUT(roleOf(host, WebFrameItems::CanGoForwardRole).toBool(), 10000);
        QVERIFY(!roleOf(host, WebFrameItems::CanGoBackRole).toBool());
        // « Suivant » est bien le suivant, pas un rechargement : la page B revient.
        host.frames->goForward(0);
        QTRY_COMPARE_WITH_TIMEOUT(roleOf(host, WebFrameItems::TitleRole).toString(), QStringLiteral("PAGE B"),
                                  10000);
        // « Recharger » est bien un rechargement : la page B est redemandée (réponse sans cache).
        server.reset();
        host.frames->reload(0);
        QTRY_VERIFY_WITH_TIMEOUT(server.paths().contains(QStringLiteral("/b")), 10000);
        QCOMPARE(roleOf(host, WebFrameItems::TitleRole).toString(), QStringLiteral("PAGE B"));
    }

    // n° 10, E12 : bascule à chaud des gardes sur le vrai moteur ; la vue est refaite avec le bon réglage.
    void guardsToggleRebuildsTheRealView()
    {
        WebFrameHost host;
        QVERIFY(host.start());
        QVERIFY(host.frames->insertFrame(
            QStringLiteral("<title>avant</title><script>document.title='JS'</script>"), {}));
        QTRY_COMPARE_WITH_TIMEOUT(host.layer->property("loadedViews").toInt(), 1, kLoadMs);
        QQuickItem* web = findItem(host.root.get(), QStringLiteral("webFrameEngine"));
        QVERIFY(web);
        QTRY_COMPARE_WITH_TIMEOUT(web->property("title").toString(), QStringLiteral("JS"), 10000);
        host.frames->setGuardsEnabled(true);
        QTRY_COMPARE_WITH_TIMEOUT(host.layer->property("loadedViews").toInt(), 2, kLoadMs);
        QQuickItem* guarded = findItem(host.root.get(), QStringLiteral("webFrameEngine"));
        QVERIFY(guarded);
        QVERIFY(
            findItem(host.root.get(), QStringLiteral("webFrameView"))->property("guardsEnabled").toBool());
        QVERIFY(profileOf(guarded)->property("offTheRecord").toBool());
        QVERIFY(!guarded->property("settings").value<QObject*>()->property("javascriptEnabled").toBool());
        QTest::qWait(300);
        QCOMPARE(guarded->property("title").toString(), QStringLiteral("avant"));
        host.frames->setGuardsEnabled(false);
        QTRY_COMPARE_WITH_TIMEOUT(host.layer->property("loadedViews").toInt(), 3, kLoadMs);
        QQuickItem* open = findItem(host.root.get(), QStringLiteral("webFrameEngine"));
        QVERIFY(open);
        QVERIFY(
            !findItem(host.root.get(), QStringLiteral("webFrameView"))->property("guardsEnabled").toBool());
        QTRY_COMPARE_WITH_TIMEOUT(open->property("title").toString(), QStringLiteral("JS"), 10000);
    }

    // n° 33 : deux vues vivantes en même temps, gardes allumées : un profil chacune, le cookie de A absent de
    // B. Gardes coupées : un profil commun, le cookie de A part avec la requête de B.
    void liveViewsShareOrIsolateTheirProfile_data()
    {
        QTest::addColumn<bool>("guards");
        QTest::newRow("gardes allumees") << true;
        QTest::newRow("gardes coupees") << false;
    }

    void liveViewsShareOrIsolateTheirProfile()
    {
        QFETCH(bool, guards);
        const QByteArray cookie = guards ? "marque=oui" : "commun=oui";
        TestServer server;
        QVERIFY(server.listening());
        server.page(QStringLiteral("/doc"), "<title>DOC</title>");
        server.header(QStringLiteral("/doc"), "Set-Cookie: " + cookie);
        const QUrl address = server.url(QStringLiteral("/doc"));
        DirectView a;
        DirectView b;
        QVERIFY(a.start(guards));
        QVERIFY(b.start(guards));
        QSignalSpy loadedA(a.root.get(), SIGNAL(pageLoaded()));
        QSignalSpy loadedB(b.root.get(), SIGNAL(pageLoaded()));
        a.root->setProperty("directUrl", address);
        b.root->setProperty("directUrl", address);
        QMetaObject::invokeMethod(a.root.get(), "load");
        QTRY_COMPARE_WITH_TIMEOUT(loadedA.count(), 1, kLoadMs);
        QMetaObject::invokeMethod(a.root.get(), "load"); // A, vivante, a reçu le cookie : il repart
        QTRY_COMPARE_WITH_TIMEOUT(loadedA.count(), 2, kLoadMs);
        QVERIFY2(server.lastRequest(QStringLiteral("/doc")).contains(cookie), "cookie de A non stocké");
        QMetaObject::invokeMethod(b.root.get(), "load");
        QTRY_COMPARE_WITH_TIMEOUT(loadedB.count(), 1, kLoadMs);
        const bool seenByB = server.lastRequest(QStringLiteral("/doc")).contains(cookie);
        QCOMPARE(seenByB, !guards);
        QCOMPARE(profileOf(a.web) == profileOf(b.web), !guards);
        if (guards) {
            QVERIFY(profileOf(a.web)->property("offTheRecord").toBool());
            QVERIFY(profileOf(b.web)->property("offTheRecord").toBool());
        }
    }

    // S1, E10 : du texte sélectionné, vue au focus : edit.copy le met au presse-papiers.
    void editCopyCopiesTheSelection()
    {
        QGuiApplication::clipboard()->clear();
        presentationharness::MainWindow main;
        QVERIFY(main.window());
        QQuickItem* layer = main.item(QStringLiteral("webFrameLayer"));
        QObject* frames = layer->findChild<QObject*>(QStringLiteral("appWebFrames"));
        bool inserted = false;
        QVERIFY(QMetaObject::invokeMethod(
            frames, "insertFrame", Q_RETURN_ARG(bool, inserted),
            Q_ARG(QString,
                  QStringLiteral("<body id=b>copie-moi<script>getSelection().selectAllChildren(b)</script>")),
            Q_ARG(QString, QString())));
        QTRY_COMPARE_WITH_TIMEOUT(layer->property("loadedViews").toInt(), 1, 60000);
        QQuickItem* web = main.item(QStringLiteral("webFrameEngine"));
        QVERIFY(web);
        main.window()->requestActivate();
        web->forceActiveFocus();
        QTRY_VERIFY(main.item(QStringLiteral("webFrameView"))->property("viewFocused").toBool());
        QVERIFY(main.run(QStringLiteral("edit.copy")));
        QTRY_COMPARE_WITH_TIMEOUT(QGuiApplication::clipboard()->text().trimmed(), QStringLiteral("copie-moi"),
                                  10000);
    }

    // Revue T20 : un clic sur la page, hors du cadre, reprend le focus au cadre. Sinon edit.copy (Ctrl+C)
    // resterait pris par la vue web et ne copierait plus les objets.
    void clickOnThePageTakesTheFocusBack()
    {
        presentationharness::MainWindow main;
        QVERIFY(main.window());
        QQuickItem* layer = main.item(QStringLiteral("webFrameLayer"));
        QObject* frames = layer->findChild<QObject*>(QStringLiteral("appWebFrames"));
        bool inserted = false;
        QVERIFY(QMetaObject::invokeMethod(frames, "insertFrame", Q_RETURN_ARG(bool, inserted),
                                          Q_ARG(QString, QStringLiteral("<p>x</p>")),
                                          Q_ARG(QString, QString())));
        QTRY_COMPARE_WITH_TIMEOUT(layer->property("loadedViews").toInt(), 1, 60000);
        QQuickItem* web = main.item(QStringLiteral("webFrameEngine"));
        QVERIFY(web);
        QVERIFY(main.waitReady());
        main.window()->requestActivate();
        web->forceActiveFocus();
        QTRY_VERIFY(main.item(QStringLiteral("webFrameView"))->property("viewFocused").toBool());
        QVERIFY(main.run(QStringLiteral("tool.select")));
        // Un point de la page hors du cadre : le coin opposé de la zone, hors de l'emprise de la vue.
        const QRectF cadre = web->mapRectToScene(web->boundingRect());
        const QRectF area = main.pageView()->mapRectToScene(main.pageView()->boundingRect());
        QPoint outside(int(area.left()) + 12, int(area.top()) + 12);
        if (cadre.adjusted(-8, -8, 8, 8).contains(outside))
            outside = QPoint(int(area.right()) - 12, int(area.bottom()) - 12);
        QVERIFY(!cadre.adjusted(-8, -8, 8, 8).contains(outside));
        QTest::mouseClick(main.window(), Qt::LeftButton, Qt::NoModifier, outside);
        QTRY_VERIFY(!main.item(QStringLiteral("webFrameView"))->property("viewFocused").toBool());
        QVariant taken;
        QVERIFY(QMetaObject::invokeMethod(layer, "copyFromFocusedView", Q_RETURN_ARG(QVariant, taken)));
        QVERIFY(!taken.toBool());
    }

    // Revue finale : le clic droit sur la page reprend lui aussi le focus à la vue web (pointerPressed part à
    // chaque appui sur la vue), sans quoi Échap puis Ctrl+C copieraient la sélection du cadre.
    void rightClickOnThePageTakesTheFocusBack()
    {
        presentationharness::MainWindow main;
        QVERIFY(main.window());
        QQuickItem* layer = main.item(QStringLiteral("webFrameLayer"));
        QObject* frames = layer->findChild<QObject*>(QStringLiteral("appWebFrames"));
        bool inserted = false;
        QVERIFY(QMetaObject::invokeMethod(frames, "insertFrame", Q_RETURN_ARG(bool, inserted),
                                          Q_ARG(QString, QStringLiteral("<p>x</p>")),
                                          Q_ARG(QString, QString())));
        QTRY_COMPARE_WITH_TIMEOUT(layer->property("loadedViews").toInt(), 1, 60000);
        QQuickItem* web = main.item(QStringLiteral("webFrameEngine"));
        QVERIFY(web);
        QVERIFY(main.waitReady());
        main.window()->requestActivate();
        web->forceActiveFocus();
        QTRY_VERIFY(main.item(QStringLiteral("webFrameView"))->property("viewFocused").toBool());
        QVERIFY(main.run(QStringLiteral("tool.select")));
        const QRectF cadre = web->mapRectToScene(web->boundingRect());
        const QRectF area = main.pageView()->mapRectToScene(main.pageView()->boundingRect());
        QPoint outside(int(area.left()) + 12, int(area.top()) + 12);
        if (cadre.adjusted(-8, -8, 8, 8).contains(outside))
            outside = QPoint(int(area.right()) - 12, int(area.bottom()) - 12);
        QVERIFY(!cadre.adjusted(-8, -8, 8, 8).contains(outside));
        QTest::mouseClick(main.window(), Qt::RightButton, Qt::NoModifier, outside);
        // Échap ferme le menu contextuel, qui rend le focus à celui qui l'avait : la vue web, si le clic
        // droit ne le lui a pas repris.
        QTest::keyClick(main.window(), Qt::Key_Escape);
        QTest::qWait(300);
        QVERIFY(!main.item(QStringLiteral("webFrameView"))->property("viewFocused").toBool());
        QVariant taken;
        QVERIFY(QMetaObject::invokeMethod(layer, "copyFromFocusedView", Q_RETURN_ARG(QVariant, taken)));
        QVERIFY(!taken.toBool());
    }

    // Revue T23 : un appui gauche sur une couverture déclarée (rideau, bande du ruban) ne reprend pas le
    // focus à la vue web : il n'appartient pas à la page, et Ctrl+C doit continuer de copier la sélection
    // web.
    void leftClickOnACoverKeepsTheFocus()
    {
        presentationharness::MainWindow main;
        QVERIFY(main.window());
        QQuickItem* layer = main.item(QStringLiteral("webFrameLayer"));
        QObject* frames = layer->findChild<QObject*>(QStringLiteral("appWebFrames"));
        bool inserted = false;
        QVERIFY(QMetaObject::invokeMethod(frames, "insertFrame", Q_RETURN_ARG(bool, inserted),
                                          Q_ARG(QString, QStringLiteral("<p>x</p>")),
                                          Q_ARG(QString, QString())));
        QTRY_COMPARE_WITH_TIMEOUT(layer->property("loadedViews").toInt(), 1, 60000);
        QQuickItem* web = main.item(QStringLiteral("webFrameEngine"));
        QVERIFY(web);
        QVERIFY(main.waitReady());
        main.window()->requestActivate();
        web->forceActiveFocus();
        QTRY_VERIFY(main.item(QStringLiteral("webFrameView"))->property("viewFocused").toBool());
        QVERIFY(main.run(QStringLiteral("tool.select")));
        const QRectF cadre = web->mapRectToScene(web->boundingRect());
        const QRectF area = main.pageView()->mapRectToScene(main.pageView()->boundingRect());
        QPoint outside(int(area.left()) + 12, int(area.top()) + 12);
        if (cadre.adjusted(-8, -8, 8, 8).contains(outside))
            outside = QPoint(int(area.right()) - 12, int(area.bottom()) - 12);
        QVERIFY(!cadre.adjusted(-8, -8, 8, 8).contains(outside));
        // Couverture déclarée sans prise de souris : l'appui arrive à la vue, qui doit le reconnaître
        // couvert.
        QQuickItem cover;
        cover.setParentItem(main.window()->contentItem());
        cover.setPosition(QPointF(outside) - QPointF(10, 10));
        cover.setSize(QSizeF(20, 20));
        main.pageView()->addInputCover(&cover);
        QTest::mouseClick(main.window(), Qt::LeftButton, Qt::NoModifier, outside);
        QTest::qWait(300);
        QVERIFY(main.item(QStringLiteral("webFrameView"))->property("viewFocused").toBool());
        main.pageView()->removeInputCover(&cover);
    }

    // Revue T20 : gardes allumées, déposer une adresse ou un fichier sur la vue ne la fait pas naviguer.
    void dropNavigationFollowsTheGuards_data()
    {
        QTest::addColumn<bool>("guards");
        QTest::newRow("gardes") << true;
        QTest::newRow("sans gardes") << false;
    }
    void dropNavigationFollowsTheGuards()
    {
        QFETCH(bool, guards);
        DirectView view;
        QVERIFY(view.start(guards));
        auto* settings = qvariant_cast<QObject*>(view.web->property("settings"));
        QVERIFY(settings);
        QCOMPARE(settings->property("navigateOnDropEnabled").toBool(), !guards);
    }

    // E10 : dans un champ de saisie, le moteur garde ses touches (ShortcutOverride) : Ctrl+V n'atteint pas la
    // page.
    void inputFieldKeepsPaste()
    {
        QGuiApplication::clipboard()->setText(QStringLiteral("xyz"));
        presentationharness::MainWindow main;
        QVERIFY(main.window());
        QQuickItem* layer = main.item(QStringLiteral("webFrameLayer"));
        QObject* frames = layer->findChild<QObject*>(QStringLiteral("appWebFrames"));
        bool inserted = false;
        QVERIFY(QMetaObject::invokeMethod(
            frames, "insertFrame", Q_RETURN_ARG(bool, inserted),
            Q_ARG(QString, QStringLiteral("<input id=i autofocus oninput=\"document.title='VAL:'+value\">")),
            Q_ARG(QString, QString())));
        QTRY_COMPARE_WITH_TIMEOUT(layer->property("loadedViews").toInt(), 1, 60000);
        QQuickItem* web = main.item(QStringLiteral("webFrameEngine"));
        QVERIFY(web);
        QVERIFY(main.waitReady());
        const int objects = main.pageObjectCount();
        main.window()->requestActivate();
        web->forceActiveFocus();
        QTRY_VERIFY(main.item(QStringLiteral("webFrameView"))->property("viewFocused").toBool());
        QTest::keyClick(main.window(), Qt::Key_V, Qt::ControlModifier);
        QTRY_COMPARE_WITH_TIMEOUT(web->property("title").toString(), QStringLiteral("VAL:xyz"), 10000);
        QCOMPARE(main.pageObjectCount(), objects);
    }
};

int main(int argc, char** argv)
{
    actcraft::ui::prepareWebFrameEngine();
    QGuiApplication app(argc, argv);
    TstUiWebEngine test;
    return QTest::qExec(&test, argc, argv);
}

#include "tst_ui_webengine.moc"
