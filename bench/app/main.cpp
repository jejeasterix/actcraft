#include "renderbackend.h"
#include "startup.h"

#include <actcraft/bench/options.h>
#include <actcraft/bench/result.h>
#include <actcraft/bench/runner.h>
#include <actcraft/bench/verdict.h>

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQuickWindow>
#include <QTimer>
#include <QtQml/qqmlextensionplugin.h>

#include <cstdio>

Q_IMPORT_QML_PLUGIN(ActCraft_UiPlugin)

namespace {
void printLine(const QString& line)
{
    std::printf("%s\n", line.toUtf8().constData());
    std::fflush(stdout);
}
} // namespace

// Banc de fluidité (DA1) : charge la vraie interface, rejoue tracé, zoom et défilement, écrit le JSON.
// Dernière ligne et code de sortie : BUDGET TENU (0), BUDGET DEPASSE (1), MESURE INVALIDE (2).
int main(int argc, char* argv[])
{
    QGuiApplication app(argc, argv);
    QGuiApplication::setApplicationName(QStringLiteral("ActCraft-Banc"));
    const QStringList args = QGuiApplication::arguments();

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

    // Déclaré AVANT le moteur QML, donc détruit APRÈS la fenêtre : Qt Quick garde les périphériques
    // synthétiques de l'injecteur, et le collecteur reçoit des appels directs du fil de rendu.
    actcraft::bench::Runner runner(options);
    QQmlApplicationEngine engine;
    StartupOptions startup;
    startup.cacheEnabled = options.cacheEnabled;
    QQuickWindow* window = loadMainWindow(engine, startup);
    if (!window) {
        printLine(QStringLiteral("MESURE INVALIDE"));
        return 2;
    }

    int code = static_cast<int>(actcraft::bench::ExitCode::Invalid);
    QTimer::singleShot(0, &app, [&] {
        actcraft::bench::BenchResult result = runner.run(window);
        const QString defaultDir = QCoreApplication::applicationDirPath() + QStringLiteral("/resultats-banc");
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
