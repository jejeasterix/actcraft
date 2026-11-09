// Le vrai moteur (cadrage n° 30) : lancé hors écran sur Mac, saut nommé sous Windows hors écran (Reste PC 4).
#include "startup.h"
#include "support/presentationharness.h"
#include "support/testserver.h"
#include "support/webframehost.h"

#include <actcraft/ui/webframeengine.h>

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
        const QImage image = QImage::fromData(resources.begin()->second.data, "PNG").convertToFormat(
            QImage::Format_RGB32);
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

    // E8 : un téléchargement lancé par la page est ignoré ; la page reste.
    void pageDownloadIsIgnored()
    {
        TestServer server;
        QVERIFY(server.listening());
        server.page(QStringLiteral("/f.bin"), "x");
        // Vraie réponse de téléchargement : depuis un document sans origine, un <a download> seul serait
        // suivi comme un lien (attribut ignoré hors de la même origine) et le test ne prouverait rien.
        server.header(QStringLiteral("/f.bin"), "Content-Disposition: attachment; filename=f.bin");
        DirectView view;
        QVERIFY(view.start(false));
        QSignalSpy loaded(view.root.get(), SIGNAL(pageLoaded()));
        view.load(
            QStringLiteral(
                "<title>RESTE</title><a id=l download "
                "href=\"%1\">x</a><script>onload=function(){setTimeout(function(){l.click()},0)}</script>")
                .arg(server.url(QStringLiteral("/f.bin")).toString()));
        QTRY_COMPARE_WITH_TIMEOUT(loaded.count(), 1, kLoadMs);
        QTRY_VERIFY_WITH_TIMEOUT(server.requests() >= 1, 10000); // le téléchargement a bien été demandé
        QTest::qWait(500);
        QCOMPARE(view.title(), QStringLiteral("RESTE"));
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
