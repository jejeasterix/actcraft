#include "renderbackend.h"
#include "startup.h"

#include <actcraft/formats/urlio.h>
#include <actcraft/ui/language.h>
#include <actcraft/ui/preferences.h>
#include <actcraft/ui/viewtransform.h>
#include <actcraft/ui/webframeengine.h>

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QTimer>
#include <QtQml/qqmlextensionplugin.h>

#include <cstdio>
#include <cstring>

#ifdef Q_OS_ANDROID
#include <sys/system_properties.h>
#endif

Q_IMPORT_QML_PLUGIN(ActCraft_UiPlugin)

// Échantillonnage de la fenêtre et du cache (H10-9) : à changer ensemble.
static_assert(kMsaaSamples == actcraft::ui::kCacheMsaaSamples);

int main(int argc, char* argv[])
{
    // `--version` (S1) : avant toute fenêtre, sur la sortie standard. Pas de QCommandLineParser::showVersion
    // : sous Windows, une application sans console l'afficherait dans une boîte de message.
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--version") == 0) {
            std::printf("ActCraft %s\n", ACTCRAFT_VERSION);
            std::fflush(stdout);
            return 0;
        }
    }

    actcraft::ui::prepareWebFrameEngine(); // avant l'application (cadre web, DA2)
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

    // Langue (D6-11) : traducteur installé avant le chargement du QML. Le Preferences de la fenêtre est une
    // instance QML ; celui-ci, temporaire, lit les mêmes réglages (QSettings par défaut).
    {
        actcraft::ui::Preferences preferences;
        actcraft::ui::Language::install(app, preferences);
    }

    QQmlApplicationEngine engine;
    QObject::connect(
        &engine, &QQmlApplicationEngine::objectCreationFailed, &app, [] { QCoreApplication::exit(1); },
        Qt::QueuedConnection);
    // Fichier nu (« Ouvrir avec », association de fichiers) ou `--ouvrir <fichier>`.
    StartupOptions options = parseStartupOptions(args);
    if (options.openUrl.isEmpty()) {
        const QString file = openFileArgument(args);
        if (!file.isEmpty()) {
            options.openUrl = actcraft::formats::urlFromArgument(file);
        }
    }
    options.showDashboard = true; // l'application seule ouvre l'accueil au démarrage (L6-T23)
#ifdef Q_OS_ANDROID
    // Qt ignore applicationArguments hors débogage (docs/verifications/2026-11-12-lot11-restes-pc.md,
    // L11-T13-PC) : l'essai de l'émulateur passe par une propriété debug.*, que seul adb shell peut poser
    // (OB68).
    char probe[PROP_VALUE_MAX] = {};
    if (__system_property_get("debug.actcraft.essai", probe) > 0 && std::strcmp(probe, "html-lourd") == 0) {
        options.heavyHtmlProbe = true;
    }
#endif
    if (!loadMainWindow(engine, options)) {
        return 1;
    }

    // `--quitter-apres <ms>` : fermeture automatique, pour les essais de démarrage et le banc.
    const qsizetype quitIndex = args.indexOf(QStringLiteral("--quitter-apres"));
    if (quitIndex >= 0 && quitIndex + 1 < args.size()) {
        const int delay = args.at(quitIndex + 1).toInt();
        if (options.webFrameProbe.isEmpty()) {
            QTimer::singleShot(delay, &app, &QCoreApplication::quit);
        } else {
            // Le cadre posé par l'essai rend le document modifié : `quit` passerait par la demande
            // d'enregistrement de Main.qml (onClosing), qui attendrait une réponse sans fin.
            QTimer::singleShot(delay, &app, [] { QCoreApplication::exit(0); });
        }
    }

    return QGuiApplication::exec();
}
