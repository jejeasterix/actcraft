// Le vrai moteur (cadrage n° 30) : lancé hors écran sur Mac, saut nommé sous Windows hors écran (Reste PC 4).
#include "startup.h"
#include "support/presentationharness.h"
#include "support/testserver.h"
#include "support/webframehost.h"

#include <actcraft/ui/webframeengine.h>

#include <QGuiApplication>
#include <QtQml/qqmlextensionplugin.h>
#include <QtTest>

Q_IMPORT_QML_PLUGIN(ActCraft_UiPlugin)

using presentationharness::findItem;

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
};

int main(int argc, char** argv)
{
    actcraft::ui::prepareWebFrameEngine();
    QGuiApplication app(argc, argv);
    TstUiWebEngine test;
    return QTest::qExec(&test, argc, argv);
}

#include "tst_ui_webengine.moc"
