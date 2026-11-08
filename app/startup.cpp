#include "startup.h"

#include <actcraft/formats/urlio.h>
#include <actcraft/ui/documentcontroller.h>

#include <QMetaObject>
#include <QQmlApplicationEngine>
#include <QQuickWindow>

StartupOptions parseStartupOptions(const QStringList& args)
{
    StartupOptions options;
    options.cacheEnabled = !args.contains(QStringLiteral("--sans-cache"));
    options.logFingerprint = args.contains(QStringLiteral("--journal-empreinte"));
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
                                 {QStringLiteral("generatedPage"), options.generatedPage},
                                 {QStringLiteral("profileRoot"), options.profileRoot},
                                 {QStringLiteral("recoveryDir"), options.recoveryDir},
                                 {QStringLiteral("showDashboard"), options.showDashboard}});
    engine.loadFromModule("ActCraft.Ui", "Main");
    if (engine.rootObjects().isEmpty()) {
        return nullptr;
    }
    auto* window = qobject_cast<QQuickWindow*>(engine.rootObjects().constFirst());
    if (window && options.logFingerprint) {
        // Essai sur l'émulateur (L11-T13) : l'empreinte du document, relevée avant et après une interruption.
        auto* document =
            window->findChild<actcraft::ui::DocumentController*>(QStringLiteral("documentController"));
        if (document) {
            const auto log = [document] {
                qInfo("ACTCRAFT EMPREINTE %s", qPrintable(document->fingerprintText()));
            };
            QObject::connect(document, &actcraft::ui::DocumentController::documentChanged, document, log);
            log();
        }
    }
    if (window && options.openUrl.isValid() && !options.openUrl.isEmpty()) {
        QVariant result;
        QMetaObject::invokeMethod(window, "runCommand", Q_RETURN_ARG(QVariant, result),
                                  Q_ARG(QVariant, QVariant(QStringLiteral("file.open"))),
                                  Q_ARG(QVariant, QVariant(options.openUrl)));
    }
    return window;
}
