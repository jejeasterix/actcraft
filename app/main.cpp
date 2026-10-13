#include "renderbackend.h"
#include "startup.h"

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QTimer>
#include <QtQml/qqmlextensionplugin.h>

#include <cstdio>

Q_IMPORT_QML_PLUGIN(ActCraft_UiPlugin)

int main(int argc, char* argv[])
{
    QGuiApplication app(argc, argv);
    QGuiApplication::setApplicationName(QStringLiteral("ActCraft"));
    QGuiApplication::setOrganizationName(QStringLiteral("ActCraft"));

    const QStringList args = QGuiApplication::arguments();

    RenderBackend backend = RenderBackend::Auto;
    QString error;
    if (!parseRenderBackend(args, backend, error)) {
        std::fprintf(stderr, "%s\n", qPrintable(error));
        return 2;
    }
    applyRenderBackend(backend);

    QQmlApplicationEngine engine;
    QObject::connect(
        &engine, &QQmlApplicationEngine::objectCreationFailed, &app, [] { QCoreApplication::exit(1); },
        Qt::QueuedConnection);
    if (!loadMainWindow(engine, parseStartupOptions(args))) {
        return 1;
    }

    // `--quitter-apres <ms>` : fermeture automatique, pour les essais de démarrage et le banc.
    const qsizetype quitIndex = args.indexOf(QStringLiteral("--quitter-apres"));
    if (quitIndex >= 0 && quitIndex + 1 < args.size()) {
        QTimer::singleShot(args.at(quitIndex + 1).toInt(), &app, &QCoreApplication::quit);
    }

    return QGuiApplication::exec();
}
