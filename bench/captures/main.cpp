#include "renderbackend.h"
#include "startup.h"

#include <actcraft/bench/uiscenario.h>

#include <QDir>
#include <QFile>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQuickWindow>
#include <QScreen>
#include <QSettings>
#include <QStandardPaths>
#include <QTest>
#include <QTimer>
#include <QtQml/qqmlextensionplugin.h>

#include <cstdio>
#include <memory>
#include <vector>

Q_IMPORT_QML_PLUGIN(ActCraft_UiPlugin)

using namespace actcraft::bench;

namespace {

void printLine(const QString& line)
{
    std::printf("%s\n", line.toUtf8().constData());
    std::fflush(stdout);
}

int invalid(const QString& why)
{
    std::fprintf(stderr, "%s\n", why.toUtf8().constData()); // UTF-8 : accents intacts
    printLine(QStringLiteral("SCENARIO INVALIDE"));
    return 2;
}

QString valueAfter(const QStringList& args, const QString& name)
{
    const qsizetype index = args.indexOf(name);
    return index >= 0 && index + 1 < args.size() ? args.at(index + 1) : QString();
}

} // namespace

// Captures et scénarios d'interface (DA8). Dernière ligne et code : SCENARIO OK (0), SCENARIO EN ECHEC (1),
// SCENARIO INVALIDE (2 : arguments, fichier, interface).
//   actcraft-captures --scenario <fichier.json> [--rendu auto|d3d11|logiciel] [--sortie <dossier>]
//                     [--taille LxH|plein-ecran] [--lire objet.propriété=valeur]…
int main(int argc, char* argv[])
{
    // DA8 : dialogues Qt Quick non natifs forcés (un dialogue natif ne reçoit pas d'événements postés).
    QCoreApplication::setAttribute(Qt::AA_DontUseNativeDialogs);
    QGuiApplication app(argc, argv);
    QGuiApplication::setApplicationName(QStringLiteral("ActCraft-Captures"));
    const QStringList args = QGuiApplication::arguments();

    RenderBackend backend = RenderBackend::Auto;
    QString error;
    if (!parseRenderBackend(args, backend, error)) {
        return invalid(error);
    }
    applyRenderBackend(backend);

    const QString scenarioPath = valueAfter(args, QStringLiteral("--scenario"));
    QString output = valueAfter(args, QStringLiteral("--sortie"));
    if (output.isEmpty()) {
        output = QDir::current().filePath(QStringLiteral("captures"));
    }
    QFile file(scenarioPath);
    if (scenarioPath.isEmpty() || !file.open(QIODevice::ReadOnly)) {
        return invalid(QStringLiteral("scénario introuvable : %1").arg(scenarioPath));
    }
    UiScenario scenario;
    if (!parseUiScenario(file.readAll(), scenario, error)) {
        return invalid(QStringLiteral("%1 : %2").arg(scenarioPath, error));
    }
    std::vector<UiStep> reads;
    for (qsizetype i = 0; i + 1 < args.size(); ++i) {
        if (args.at(i) == QLatin1String("--lire")) {
            UiStep read;
            if (!parseReadSpec(args.at(i + 1), read)) {
                return invalid(
                    QStringLiteral("--lire attend « objet.propriété=valeur » : %1").arg(args.at(i + 1)));
            }
            reads.push_back(read);
        }
    }
    const QString size = valueAfter(args, QStringLiteral("--taille"));
    const bool fullScreen = size == QLatin1String("plein-ecran");
    QSize windowSize(1280, 800);
    if (!size.isEmpty() && !fullScreen) {
        const QStringList parts = size.split(QLatin1Char('x'));
        bool okWidth = false;
        bool okHeight = false;
        windowSize = QSize(parts.value(0).toInt(&okWidth), parts.value(1).toInt(&okHeight));
        if (parts.size() != 2 || !okWidth || !okHeight || windowSize.isEmpty()) {
            return invalid(QStringLiteral("--taille attend LxH ou plein-ecran"));
        }
    }

    // Réglages et profils d'essai propres à ce lancement (L6-T5, amendement 3, N1), posés avant tout objet :
    // jamais ceux du poste. `<sortie>` est propre au scénario et au rendu ; l'étape `restart` ne vide rien.
    const QDir outputDir(output);
    const QString settingsDir = outputDir.filePath(QStringLiteral("reglages"));
    const QString profilesDir = outputDir.filePath(QStringLiteral("profils"));
    QDir(settingsDir).removeRecursively();
    QDir(profilesDir).removeRecursively();
    QStandardPaths::setTestModeEnabled(true);
    // Racine « Mes ressources » d'essai (mode test : jamais le dossier réel du poste) vidée à chaque
    // lancement, pour que le scénario `ressources` se rejoue sans nettoyage à la main.
    QDir(QDir(QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation))
             .filePath(QStringLiteral("ActCraft/Ressources")))
        .removeRecursively();
    QGuiApplication::setOrganizationName(QStringLiteral("ActCraft"));
    QSettings::setDefaultFormat(QSettings::IniFormat);
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, settingsDir);
    StartupOptions startup = parseStartupOptions(args); // --sans-cache compris
    startup.profileRoot = profilesDir;

    // Déclaré AVANT le moteur QML, donc détruit APRÈS la fenêtre : Qt Quick garde le stylet synthétique.
    std::unique_ptr<UiScenarioPlayer> player;
    auto engine = std::make_unique<QQmlApplicationEngine>();
    // Charge l'interface sur les dossiers d'essai ci-dessus et l'affiche ; sert au démarrage et à l'étape
    // `restart` (mêmes dossiers, jamais ceux du poste).
    const auto launch = [&]() -> QQuickWindow* {
        QQuickWindow* shown = loadMainWindow(*engine, startup);
        if (!shown) {
            return nullptr;
        }
        QScreen* screen = QGuiApplication::primaryScreen();
        shown->setScreen(screen); // écran principal (4 moniteurs sur le poste de Jérôme)
        if (fullScreen) {
            shown->setGeometry(screen->geometry());
            shown->showFullScreen();
        } else {
            shown->resize(windowSize);
            shown->setPosition(screen->availableGeometry().center() -
                               QPoint(windowSize.width() / 2, windowSize.height() / 2));
            shown->showNormal();
        }
        return shown;
    };
    QQuickWindow* window = launch();
    if (!window) {
        return invalid(QStringLiteral("interface non chargée"));
    }
    player = std::make_unique<UiScenarioPlayer>(window);
    player->setLineSink(printLine); // une ligne par étape, au fil de l'eau
    player->setRestartHandler([&]() -> QQuickWindow* {
        engine.reset(); // détruit l'ancien moteur et sa fenêtre avant d'en créer un autre
        engine = std::make_unique<QQmlApplicationEngine>();
        QQuickWindow* fresh = launch();
        if (!fresh || !QTest::qWaitForWindowExposed(fresh, 10000)) {
            return nullptr;
        }
        window = fresh;
        return fresh;
    });

    int code = 2;
    QTimer::singleShot(0, &app, [&] {
        QStringList report;
        if (!QTest::qWaitForWindowExposed(window, 10000)) {
            printLine(QStringLiteral("FENETRE non exposée"));
            printLine(QStringLiteral("SCENARIO INVALIDE"));
            code = 2;
            QCoreApplication::exit(code);
            return;
        }
        bool ok = player->play(scenario, output, report);
        for (const UiStep& read : reads) {
            ok = player->expect(read, report) && ok;
        }
        printLine(ok ? QStringLiteral("SCENARIO OK") : QStringLiteral("SCENARIO EN ECHEC"));
        code = ok ? 0 : 1;
        QCoreApplication::exit(code);
    });
    QGuiApplication::exec();
    return code;
}
