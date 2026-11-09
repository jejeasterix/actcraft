#include "renderbackend.h"
#include "startup.h"

#include <actcraft/bench/options.h>
#include <actcraft/bench/result.h>
#include <actcraft/bench/runner.h>
#include <actcraft/bench/simulatedrecorders.h>
#include <actcraft/bench/testroots.h>
#include <actcraft/bench/verdict.h>
#include <actcraft/ui/webframeengine.h>

#include <QDir>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQuickWindow>
#include <QSettings>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QTimer>
#include <QtQml/qqmlextensionplugin.h>

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
#ifdef Q_OS_ANDROID
    qInfo().noquote() << line; // stdout n'atteint pas logcat : scripts/banc-android.ps1 y lit le verdict
#endif
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
    actcraft::ui::prepareWebFrameEngine(); // avant l'application (cadre web, DA2)
    QGuiApplication app(argc, argv);
#ifdef Q_OS_ANDROID
    actcraft::bench::useSimulatedRecorders(); // aucune permission : ni micro ni fenêtre de permission
#else
    if (QGuiApplication::platformName() == QLatin1String("offscreen"))
        actcraft::bench::useSimulatedRecorders(); // hors écran : ni micro ni fenêtre de permission
#endif
    QGuiApplication::setApplicationName(QStringLiteral("ActCraft-Banc"));
    // Avant le relevé REGLAGES, qui montre ainsi le vrai chemin du fichier de réglages.
    QGuiApplication::setOrganizationName(QStringLiteral("ActCraft"));
    // L7-T38 : réglages propres à la passe, dans un dossier temporaire, comme bench/captures/main.cpp :
    // l'effet de page choisi par « effets-page » (QSettings, TransitionController) ne survit pas à une passe
    // interrompue (Ctrl+C) et ne touche ni les passes suivantes ni de vrais réglages. QSettings natif
    // (CFPreferences, registre) ignorerait QStandardPaths::setTestModeEnabled. Déclaré avant le moteur QML,
    // détruit après lui.
    const QTemporaryDir settingsDir;
    if (!settingsDir.isValid()) {
        std::fprintf(stderr, "dossier de réglages temporaire impossible : %s\n",
                     qPrintable(settingsDir.errorString()));
        printLine(QStringLiteral("MESURE INVALIDE"));
        return 2;
    }
    QSettings::setDefaultFormat(QSettings::IniFormat);
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, settingsDir.path());
    // Relevé avant toute mesure, sans rien écrire (tst_bench_reglages_isoles, sur une sortie refusée).
    std::fprintf(stderr, "REGLAGES %s\n", qPrintable(QSettings().fileName()));
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
    // profils d'essai du banc, vidés à chaque lancement et posés avant tout objet (N1). Les réglages sont
    // déjà dans le dossier temporaire de la passe (L7-T38, plus haut).
#ifdef Q_OS_ANDROID
    const QString defaultDir =
        actcraft::bench::resultsDirFor(actcraft::bench::Platform::Android, androidExternalFilesDir());
#else
    const QString defaultDir = actcraft::bench::resultsDirFor(actcraft::bench::Platform::Desktop,
                                                              QCoreApplication::applicationDirPath());
#endif
    const QString profilesDir = QDir(defaultDir).filePath(QStringLiteral("profils"));
    QDir(profilesDir).removeRecursively();
    QStandardPaths::setTestModeEnabled(true);
    actcraft::bench::useIsolatedResourceRoots(defaultDir); // jamais le Documents/ActCraft/Ressources du poste

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
