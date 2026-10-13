#include "startup.h"

#include <QQmlApplicationEngine>
#include <QQuickWindow>

StartupOptions parseStartupOptions(const QStringList& args)
{
    StartupOptions options;
    options.cacheEnabled = !args.contains(QStringLiteral("--sans-cache"));
    return options;
}

QQuickWindow* loadMainWindow(QQmlApplicationEngine& engine, const StartupOptions& options)
{
    engine.setInitialProperties({{QStringLiteral("cacheEnabled"), options.cacheEnabled}});
    engine.loadFromModule("ActCraft.Ui", "Main");
    if (engine.rootObjects().isEmpty()) {
        return nullptr;
    }
    return qobject_cast<QQuickWindow*>(engine.rootObjects().constFirst());
}
