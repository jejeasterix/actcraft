#include "startup.h"

#include <actcraft/core/document.h>
#include <actcraft/formats/nativefile.h>
#include <actcraft/ui/documentcontroller.h>

#include <QFile>
#include <QQmlApplicationEngine>
#include <QQuickWindow>
#include <QTemporaryDir>
#include <QTest>
#include <QtQml/qqmlextensionplugin.h>

Q_IMPORT_QML_PLUGIN(ActCraft_UiPlugin)

using namespace actcraft;

// Fichier ouvert au lancement (S4, décision 26).
class TstAppOpen : public QObject
{
    Q_OBJECT

private slots:
    void firstNonOptionArgumentIsTheFile()
    {
        QCOMPARE(openFileArgument({QStringLiteral("actcraft.exe"), QStringLiteral("C:/a/b.flipchart")}),
                 QStringLiteral("C:/a/b.flipchart"));
        QCOMPARE(openFileArgument({QStringLiteral("actcraft.exe"), QStringLiteral("--rendu"),
                                   QStringLiteral("d3d11"), QStringLiteral("x.actcraft")}),
                 QStringLiteral("x.actcraft"));
        QCOMPARE(openFileArgument({QStringLiteral("actcraft.exe"), QStringLiteral("--quitter-apres"),
                                   QStringLiteral("5000")}),
                 QString());
        QCOMPARE(
            openFileArgument({QStringLiteral("x"), QStringLiteral("--ouvrir"), QStringLiteral("y.actcraft")}),
            QStringLiteral("y.actcraft"));
        QVERIFY(parseStartupOptions(
                    {QStringLiteral("x"), QStringLiteral("--ouvrir"), QStringLiteral("y.actcraft")})
                    .openUrl.isLocalFile());
    }

    void startupOpensTheFile()
    {
        QTemporaryDir dir;
        const QString path = dir.filePath(QStringLiteral("trois.actcraft"));
        QFile file(path);
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.write(formats::encodeActcraft(core::Document(std::vector<core::Page>(3)), {}));
        file.close();
        StartupOptions options;
        options.openUrl = QUrl::fromLocalFile(path);
        QQmlApplicationEngine engine;
        QQuickWindow* window = loadMainWindow(engine, options);
        QVERIFY(window);
        auto* doc = window->findChild<ui::DocumentController*>(QStringLiteral("documentController"));
        QVERIFY(doc);
        QTRY_COMPARE_WITH_TIMEOUT(doc->pageCount(), 3, 10000);
        QCOMPARE(doc->title(), QStringLiteral("trois"));
    }
};

QTEST_MAIN(TstAppOpen)
#include "tst_app_open.moc"
