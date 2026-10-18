#include "startup.h"

#include <actcraft/formats/urlio.h>

#include <QMetaObject>
#include <QQmlApplicationEngine>
#include <QQuickGraphicsConfiguration>
#include <QQuickWindow>

StartupOptions parseStartupOptions(const QStringList& args)
{
    StartupOptions options;
    options.cacheEnabled = !args.contains(QStringLiteral("--sans-cache"));
    const qsizetype at = args.indexOf(QStringLiteral("--ouvrir"));
    if (at >= 0 && at + 1 < args.size()) {
        options.openUrl = actcraft::formats::urlFromArgument(args.at(at + 1));
    }
    return options;
}

QString openFileArgument(const QStringList& args)
{
    const qsizetype explicitIndex = args.indexOf(QStringLiteral("--ouvrir"));
    if (explicitIndex >= 0 && explicitIndex + 1 < args.size()) {
        return args.at(explicitIndex + 1);
    }
    static const QStringList withValue{QStringLiteral("--rendu"), QStringLiteral("--quitter-apres"),
                                       QStringLiteral("--ouvrir")};
    for (qsizetype i = 1; i < args.size(); ++i) {
        const QString& a = args.at(i);
        if (withValue.contains(a)) {
            ++i; // sa valeur n'est pas un fichier
            continue;
        }
        if (!a.startsWith(QLatin1String("--"))) {
            return a;
        }
    }
    return QString();
}

QQuickWindow* loadMainWindow(QQmlApplicationEngine& engine, const StartupOptions& options)
{
    engine.setInitialProperties({{QStringLiteral("cacheEnabled"), options.cacheEnabled},
                                 {QStringLiteral("generatedPage"), options.generatedPage}});
    engine.loadFromModule("ActCraft.Ui", "Main");
    if (engine.rootObjects().isEmpty()) {
        return nullptr;
    }
    auto* window = qobject_cast<QQuickWindow*>(engine.rootObjects().constFirst());
    if (window) {
        // H10-9 (L10-T8) : pas de tampon de profondeur pour le contenu 2D. Les couches, dont la texture du
        // cache, perdent leur profondeur-stencil multi-échantillonnée. Main.qml montre la fenêtre au
        // chargement ; elle n'est exposée (scène initialisée) qu'au retour à la boucle d'événements.
        QQuickGraphicsConfiguration config = window->graphicsConfiguration();
        config.setDepthBufferFor2D(false);
        window->setGraphicsConfiguration(config);
    }
    if (window && options.openUrl.isValid() && !options.openUrl.isEmpty()) {
        QVariant result;
        QMetaObject::invokeMethod(window, "runCommand", Q_RETURN_ARG(QVariant, result),
                                  Q_ARG(QVariant, QVariant(QStringLiteral("file.open"))),
                                  Q_ARG(QVariant, QVariant(options.openUrl)));
    }
    return window;
}
