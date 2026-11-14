#include "startup.h"

#include <actcraft/core/document.h>
#include <actcraft/formats/nativefile.h>
#include <actcraft/ui/documentcontroller.h>
#include <actcraft/ui/webframeitem.h>

#include <QFile>
#include <QFileInfo>
#include <QQmlApplicationEngine>
#include <QQuickWindow>
#include <QSettings>
#include <QTemporaryDir>
#include <QTest>
#include <QtQml/qqmlextensionplugin.h>

Q_IMPORT_QML_PLUGIN(ActCraft_UiPlugin)

using namespace actcraft;

// Fichier ouvert au lancement (S4, décision 26).
class TstAppOpen : public QObject
{
    Q_OBJECT
    QTemporaryDir m_settingsDir;

private slots:
    void initTestCase()
    {
        // Main.qml porte Preferences et ProfileController : réglages INI d'essai, jamais ceux du poste.
        QVERIFY(m_settingsDir.isValid());
        QSettings::setDefaultFormat(QSettings::IniFormat);
        QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, m_settingsDir.path());
    }

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

    void parsesWebFrameProbe()
    {
        const StartupOptions options =
            parseStartupOptions({QStringLiteral("actcraft"), QStringLiteral("--essai-cadre-web"),
                                 QStringLiteral("/tmp/page.html")});
        QCOMPARE(options.webFrameProbe, QStringLiteral("/tmp/page.html"));
        QVERIFY(parseStartupOptions({QStringLiteral("actcraft")}).webFrameProbe.isEmpty());
    }

    void parsesHeavyHtmlProbe()
    {
        QVERIFY(parseStartupOptions({QStringLiteral("actcraft"), QStringLiteral("--essai-html-lourd")})
                    .heavyHtmlProbe);
        QVERIFY(!parseStartupOptions({QStringLiteral("actcraft")}).heavyHtmlProbe);
        QVERIFY(
            openFileArgument({QStringLiteral("actcraft"), QStringLiteral("--essai-html-lourd")}).isEmpty());
        QVERIFY(!parseStartupOptions({QStringLiteral("actcraft"), QStringLiteral("--essai-html-lourd")})
                     .heavyHtmlGuards);
        const StartupOptions guarded =
            parseStartupOptions({QStringLiteral("actcraft"), QStringLiteral("--essai-html-lourd-gardes")});
        QVERIFY(guarded.heavyHtmlProbe);
        QVERIFY(guarded.heavyHtmlGuards);
        QVERIFY(openFileArgument({QStringLiteral("actcraft"), QStringLiteral("--essai-html-lourd-gardes")})
                    .isEmpty());
    }

    // OB68 : le HTML de l'essai dépasse la limite des adresses data: (2 Mo) même sans encodage, et porte son
    // repère.
    void heavyHtmlProbePageExceedsTheAddressLimit()
    {
        const QString page = heavyHtmlProbePage();
        QVERIFY(page.toUtf8().size() > 3000000);
        QVERIFY(page.contains(QStringLiteral("<title>ACTCRAFT HTML LOURD</title>")));
        QVERIFY(page.contains(QStringLiteral("background:#00c853")));
    }

    // OB75 : gardes allumées, la page de l'essai passe par la réécriture du modèle (finishContent) ; elle y
    // reste au-delà de la limite de loadHtml (fichier écrit, ce qui donne la base about:blank sous Android)
    // et verte.
    void heavyHtmlProbePageStaysHeavyUnderGuards()
    {
        ui::DocumentController document;
        ui::WebFrameItems frames;
        frames.setGuardsEnabled(true);
        document.newDocument(1024, 768);
        frames.setDocument(&document);
        QVERIFY(frames.insertFrame(heavyHtmlProbePage(), {}));
        const auto role = [&](int r) { return frames.data(frames.index(0, 0), r); };
        QTRY_VERIFY_WITH_TIMEOUT(!role(ui::WebFrameItems::LoadingRole).toBool() &&
                                     !role(ui::WebFrameItems::ContentRole).toString().isEmpty(),
                                 60000);
        const QString shown = role(ui::WebFrameItems::ContentRole).toString();
        QVERIFY2(shown.size() > 3000000, qPrintable(QString::number(shown.size())));
        QVERIFY(shown.contains(QStringLiteral("background:#00c853")));
        const QUrl file = role(ui::WebFrameItems::ContentFileRole).toUrl();
        QVERIFY2(file.isLocalFile() && QFileInfo::exists(file.toLocalFile()), qPrintable(file.toString()));
    }

    void startupOpensTheFile()
    {
        QTemporaryDir dir;
        const QString path = dir.filePath(QStringLiteral("trois.actcraft"));
        QFile file(path);
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.write(formats::encodeActcraft(core::Document(std::vector<core::Page>(3)), {}));
        file.close();
        QTemporaryDir profiles; // profils d'essai (amendement 3, N1)
        StartupOptions options;
        options.openUrl = QUrl::fromLocalFile(path);
        options.profileRoot = profiles.path();
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
