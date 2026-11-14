#include "startup.h"

#include <actcraft/formats/urlio.h>
#include <actcraft/ui/documentcontroller.h>

#include <QMetaObject>
#include <QQmlApplicationEngine>
#include <QQuickItem>
#include <QQuickWindow>
#include <QTimer>

#include <functional>

StartupOptions parseStartupOptions(const QStringList& args)
{
    StartupOptions options;
    options.cacheEnabled = !args.contains(QStringLiteral("--sans-cache"));
    options.logFingerprint = args.contains(QStringLiteral("--journal-empreinte"));
    const qsizetype at = args.indexOf(QStringLiteral("--ouvrir"));
    if (at >= 0 && at + 1 < args.size()) {
        options.openUrl = actcraft::formats::urlFromArgument(args.at(at + 1));
    }
    const qsizetype probe = args.indexOf(QStringLiteral("--essai-cadre-web"));
    if (probe >= 0 && probe + 1 < args.size()) {
        options.webFrameProbe = args.at(probe + 1);
    }
    options.heavyHtmlProbe = args.contains(QStringLiteral("--essai-html-lourd"));
    return options;
}

QString openFileArgument(const QStringList& args)
{
    const qsizetype explicitIndex = args.indexOf(QStringLiteral("--ouvrir"));
    if (explicitIndex >= 0 && explicitIndex + 1 < args.size()) {
        return args.at(explicitIndex + 1);
    }
    static const QStringList withValue{QStringLiteral("--rendu"), QStringLiteral("--quitter-apres"),
                                       QStringLiteral("--ouvrir"), QStringLiteral("--essai-cadre-web")};
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

QString heavyHtmlProbePage()
{
    return QStringLiteral("<!DOCTYPE html><html><head><title>ACTCRAFT HTML LOURD</title></head>"
                          "<body style=\"margin:0;background:#00c853\"><!--") +
           QString(3000000, u'x') + QStringLiteral("--></body></html>");
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
    if (window && (!options.webFrameProbe.isEmpty() || options.heavyHtmlProbe)) {
        const QString address = options.webFrameProbe;
        const QString html = options.heavyHtmlProbe ? heavyHtmlProbePage() : QString();
        QTimer::singleShot(1000, window, [window, address, html] {
            const std::function<QQuickItem*(QQuickItem*)> find = [&](QQuickItem* item) -> QQuickItem* {
                if (item->objectName() == QLatin1String("webFrameLayer"))
                    return item;
                for (QQuickItem* child : item->childItems()) {
                    if (QQuickItem* found = find(child))
                        return found;
                }
                return nullptr;
            };
            QQuickItem* layer = find(window->contentItem());
            QObject* frames = layer ? layer->findChild<QObject*>(QStringLiteral("appWebFrames")) : nullptr;
            bool ok = false;
            if (frames)
                QMetaObject::invokeMethod(
                    frames, "insertFrame", Q_RETURN_ARG(bool, ok), Q_ARG(QString, html),
                    Q_ARG(QString, address.isEmpty() ? QString() : QUrl::fromUserInput(address).toString()));
            qInfo("ACTCRAFT ESSAI CADRE WEB %s", ok ? "POSE" : "ECHEC");
        });
    }
    return window;
}
