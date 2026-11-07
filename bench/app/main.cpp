#include "renderbackend.h"
#include "startup.h"

#include <actcraft/bench/options.h>
#include <actcraft/bench/result.h>
#include <actcraft/bench/runner.h>
#include <actcraft/bench/simulatedrecorders.h>
#include <actcraft/bench/testroots.h>
#include <actcraft/bench/verdict.h>
#include <actcraft/ui/webframeitem.h> // ACTCRAFT_WEBFRAME

#include <QDir>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQuickWindow>
#include <QSettings>
#include <QStandardPaths>
#include <QTimer>
#include <QtQml/qqmlextensionplugin.h>
#if ACTCRAFT_WEBFRAME
#include <QtWebView/QtWebView>
#endif

#include <cstdio>
#ifdef Q_OS_ANDROID
#include <QJniObject>
#endif

Q_IMPORT_QML_PLUGIN(ActCraft_UiPlugin)

namespace {
void printLine(const QString& line)
{
    std::printf("%s\n", line.toUtf8().constData());
    std::fflush(stdout);
}

#ifdef Q_OS_ANDROID
// Dossier propre externe de l'application (getExternalFilesDir) : lisible par `adb pull` sans `run-as`, qu'un
// APK release refuse (A21). Repli sur le chemin canonique si le système ne répond pas.
QString androidExternalFilesDir()
{
    const QJniObject context = QNativeInterface::QAndroidApplication::context();
    if (context.isValid()) {
        const QJniObject dir = context.callObjectMethod(
            "getExternalFilesDir", "(Ljava/lang/String;)Ljava/io/File;", static_cast<jstring>(nullptr));
        if (dir.isValid()) {
            return dir.callObjectMethod<jstring>("getAbsolutePath").toString();
        }
    }
    return QStringLiteral("/sdcard/Android/data/io.github.jejeasterix.actcraft.banc/files");
}
#endif
} // namespace

// Banc de fluidité (DA1) : charge la vraie interface, rejoue tracé, zoom et défilement, écrit le JSON.
// Dernière ligne et code de sortie : BUDGET TENU (0), BUDGET DEPASSE (1), MESURE INVALIDE (2).
int main(int argc, char* argv[])
{
#if ACTCRAFT_WEBFRAME
    QtWebView::initialize(); // M10 : avant l'application (cadre web, L8-T30)
#endif
    QGuiApplication app(argc, argv);
#ifdef Q_OS_ANDROID
    actcraft::bench::useSimulatedRecorders(); // aucune permission : ni micro ni fenêtre de permission
#else
    if (QGuiApplication::platformName() == QLatin1String("offscreen"))
        actcraft::bench::useSimulatedRecorders(); // hors écran : ni micro ni fenêtre de permission
#endif
    QGuiApplication::setApplicationName(QStringLiteral("ActCraft-Banc"));
    const QStringList args = actcraft::bench::expandSingleStringArguments(QGuiApplication::arguments());

    RenderBackend backend = RenderBackend::Auto;
    QString error;
    actcraft::bench::BenchOptions options;
    if (!parseRenderBackend(args, backend, error) ||
        !actcraft::bench::parseBenchOptions(args, options, error)) {
        std::fprintf(stderr, "%s\n", qPrintable(error));
        printLine(QStringLiteral("MESURE INVALIDE"));
        return 2;
    }
    applyRenderBackend(backend);

    // Dossier des résultats par défaut, calculé avant l'interface (amendement 3, n5) : il porte aussi les
    // réglages et les profils d'essai du banc, vidés à chaque lancement et posés avant tout objet (N1).
#ifdef Q_OS_ANDROID
    const QString defaultDir =
        actcraft::bench::resultsDirFor(actcraft::bench::Platform::Android, androidExternalFilesDir());
#else
    const QString defaultDir = actcraft::bench::resultsDirFor(actcraft::bench::Platform::Desktop,
                                                              QCoreApplication::applicationDirPath());
#endif
    const QString settingsDir = QDir(defaultDir).filePath(QStringLiteral("reglages"));
    const QString profilesDir = QDir(defaultDir).filePath(QStringLiteral("profils"));
    QDir(settingsDir).removeRecursively();
    QDir(profilesDir).removeRecursively();
    QStandardPaths::setTestModeEnabled(true);
    actcraft::bench::useIsolatedResourceRoots(defaultDir); // jamais le Documents/ActCraft/Ressources du poste
    QGuiApplication::setOrganizationName(QStringLiteral("ActCraft"));
    QSettings::setDefaultFormat(QSettings::IniFormat);
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, settingsDir);

    // Déclaré AVANT le moteur QML, donc détruit APRÈS la fenêtre : Qt Quick garde les périphériques
    // synthétiques de l'injecteur, et le collecteur reçoit des appels directs du fil de rendu.
    actcraft::bench::Runner runner(options);
    QQmlApplicationEngine engine;
    StartupOptions startup;
    startup.cacheEnabled = options.cacheEnabled;
    startup.generatedPage = true; // le banc mesure la page de 300 objets (DA7)
    startup.profileRoot = profilesDir;
    QQuickWindow* window = loadMainWindow(engine, startup);
    if (!window) {
        printLine(QStringLiteral("MESURE INVALIDE"));
        return 2;
    }

    int code = static_cast<int>(actcraft::bench::ExitCode::Invalid);
    QTimer::singleShot(0, &app, [&] {
        actcraft::bench::BenchResult result = runner.run(window);
        const QString path =
            actcraft::bench::resolveOutputPath(options, defaultDir, actcraft::bench::resultFileName(result));
        QString writeError;
        if (!actcraft::bench::writeResult(result, path, writeError)) {
            result.invalidReasons << QStringLiteral("error: %1").arg(writeError);
            actcraft::bench::applyVerdict(result);
        }
        for (const QString& line : actcraft::bench::summaryLines(result, path)) {
            printLine(line);
        }
        code = static_cast<int>(actcraft::bench::exitCodeFor(result));
        QCoreApplication::exit(code);
    });
    QGuiApplication::exec();
    return code;
}
