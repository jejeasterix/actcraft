#include "renderbackend.h"
#include "startup.h"

#include <actcraft/bench/simulatedrecorders.h>
#include <actcraft/bench/testroots.h>
#include <actcraft/bench/uiscenario.h>
#include <actcraft/ui/language.h>
#include <actcraft/ui/webframeitem.h> // ACTCRAFT_WEBFRAME

#include <QDir>
#include <QFile>
#include <QGuiApplication>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QQmlApplicationEngine>
#include <QQmlNetworkAccessManagerFactory>
#include <QQuickWindow>
#include <QScreen>
#include <QSettings>
#include <QStandardPaths>
#include <QSysInfo>
#include <QTemporaryDir>
#include <QTest>
#include <QThread>
#include <QTimer>
#include <QtQml/qqmlextensionplugin.h>
#if ACTCRAFT_WEBFRAME
#include <QtWebView/QtWebView>
#endif

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

// --retard-images (L7-T40) : injection de lenteur, jamais une synchronisation. Le lecteur d'images de Qt
// Quick charge hors du fil principal ce qui n'est ni un fichier ni une ressource, URL « data: » comprises (le
// pointillé des séparations de zones), par le gestionnaire réseau que lui fabrique le moteur. Chaque requête
// attend ici, dans le fil du lecteur : une étape qui lit des pixels sans attendre la fin des chargements
// échoue à coup sûr.
class SlowNetworkAccessManager : public QNetworkAccessManager
{
public:
    SlowNetworkAccessManager(unsigned long delayMs, QObject* parent)
        : QNetworkAccessManager(parent), m_delayMs(delayMs)
    {}

protected:
    QNetworkReply* createRequest(Operation op, const QNetworkRequest& request,
                                 QIODevice* outgoingData) override
    {
        QThread::msleep(m_delayMs);
        return QNetworkAccessManager::createRequest(op, request, outgoingData);
    }

private:
    unsigned long m_delayMs;
};

class SlowNetworkFactory : public QQmlNetworkAccessManagerFactory
{
public:
    explicit SlowNetworkFactory(unsigned long delayMs) : m_delayMs(delayMs) {}
    QNetworkAccessManager* create(QObject* parent) override
    {
        return new SlowNetworkAccessManager(m_delayMs, parent);
    }

private:
    unsigned long m_delayMs;
};

} // namespace

// Captures et scénarios d'interface (DA8). Dernière ligne et code : SCENARIO OK (0), SCENARIO EN ECHEC (1),
// SCENARIO INVALIDE (2 : arguments, fichier, interface).
//   actcraft-captures --scenario <fichier.json> [--rendu auto|d3d11|logiciel] [--sortie <dossier>]
//                     [--taille LxH|plein-ecran] [--lire objet.propriété=valeur]… [--traduction <fichier.qm>]
//                     [--retard-images <ms>]
int main(int argc, char* argv[])
{
    // DA8 : dialogues Qt Quick non natifs forcés (un dialogue natif ne reçoit pas d'événements postés).
    QCoreApplication::setAttribute(Qt::AA_DontUseNativeDialogs);
#if ACTCRAFT_WEBFRAME
    QtWebView::initialize(); // M10 : avant l'application (cadre web, L8-T30)
#endif
    QGuiApplication app(argc, argv);
    if (QGuiApplication::platformName() == QLatin1String("offscreen"))
        actcraft::bench::useSimulatedRecorders(); // hors écran : ni micro ni fenêtre de permission
    QGuiApplication::setApplicationName(QStringLiteral("ActCraft-Captures"));
    // Réglages propres à la passe, dans un dossier temporaire, comme MainWindow des tests
    // (tests/support/presentationharness.h) : une passe ne lit jamais ce qu'une autre a écrit (effets.json
    // choisit le fondu global), ni les réglages réels de l'utilisateur. QStandardPaths::setTestModeEnabled ne
    // suffirait pas : le format natif de QSettings (CFPreferences sous macOS, registre sous Windows)
    // l'ignore. Déclaré avant le moteur QML, donc détruit après lui.
    const QTemporaryDir settingsDir;
    if (!settingsDir.isValid()) {
        return invalid(
            QStringLiteral("dossier de réglages temporaire impossible : %1").arg(settingsDir.errorString()));
    }
    QSettings::setDefaultFormat(QSettings::IniFormat);
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, settingsDir.path());
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
    const QByteArray content = file.readAll();
    // Un scénario peut déclarer "platforms": ["windows"] ; ailleurs il ne se joue pas (code 77, « Skipped »).
    const QJsonArray platforms =
        QJsonDocument::fromJson(content).object().value(QStringLiteral("platforms")).toArray();
    if (!platforms.isEmpty() && !platforms.contains(QJsonValue(QSysInfo::productType()))) {
        printLine(QStringLiteral("SCENARIO HORS PLATEFORME"));
        return 77;
    }
    UiScenario scenario;
    if (!parseUiScenario(content, scenario, error)) {
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

    // Profils d'essai propres à ce lancement (L6-T5, amendement 3, N1), posés avant tout objet : jamais ceux
    // du poste ; les réglages sont déjà dans le dossier temporaire de la passe (L7-T41, plus haut).
    // `<sortie>` est propre au scénario et au rendu ; l'étape `restart` ne vide rien.
    const QDir outputDir(output);
    const QString profilesDir = outputDir.filePath(QStringLiteral("profils"));
    // Copie de reprise (L11-T13) : vidée au lancement, gardée à l'étape `restart` (la reprise s'y relit).
    const QString recoveryDir = outputDir.filePath(QStringLiteral("reprise"));
    QDir(profilesDir).removeRecursively();
    QDir(recoveryDir).removeRecursively();
    QStandardPaths::setTestModeEnabled(true);
    // Racines « Mes ressources » et partagées d'essai sous `<sortie>`, vidées à chaque lancement (le mode
    // test ne déplace pas DocumentsLocation : jamais le vrai Documents/ActCraft/Ressources du poste).
    actcraft::bench::useIsolatedResourceRoots(output);
    QGuiApplication::setOrganizationName(QStringLiteral("ActCraft"));
    StartupOptions startup = parseStartupOptions(args); // --sans-cache compris
    startup.profileRoot = profilesDir;
    startup.recoveryDir = recoveryDir;

    // Pseudo-langue de contrôle (L11-T19) : traduction installée AVANT le chargement de l'interface.
    const QString translation = valueAfter(args, QStringLiteral("--traduction"));
    if (!translation.isEmpty() && !actcraft::ui::Language::installFile(app, translation)) {
        return invalid(QStringLiteral("--traduction : fichier illisible : %1").arg(translation));
    }

    std::unique_ptr<SlowNetworkFactory> slowImages; // avant le moteur, donc détruit après lui
    const QString delay = valueAfter(args, QStringLiteral("--retard-images"));
    if (args.contains(QStringLiteral("--retard-images")) && delay.isEmpty()) {
        return invalid(QStringLiteral("--retard-images attend un délai de 1 à 60000 ms"));
    }
    if (!delay.isEmpty()) {
        bool okDelay = false;
        const int delayMs = delay.toInt(&okDelay);
        if (!okDelay || delayMs < 1 || delayMs > 60000) {
            return invalid(QStringLiteral("--retard-images attend un délai de 1 à 60000 ms"));
        }
        slowImages = std::make_unique<SlowNetworkFactory>(static_cast<unsigned long>(delayMs));
    }

    // Déclaré AVANT le moteur QML, donc détruit APRÈS la fenêtre : Qt Quick garde le stylet synthétique.
    std::unique_ptr<UiScenarioPlayer> player;
    auto engine = std::make_unique<QQmlApplicationEngine>();
    // Charge l'interface sur les dossiers d'essai ci-dessus et l'affiche ; sert au démarrage et à l'étape
    // `restart` (mêmes dossiers, jamais ceux du poste).
    const auto launch = [&]() -> QQuickWindow* {
        if (slowImages) {
            engine->setNetworkAccessManagerFactory(slowImages.get());
        }
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
        // Les étapes sautées (skipOn) ne passent pas inaperçues dans le verdict final.
        const qsizetype skipped = report.filter(QStringLiteral("SAUTE étape"), Qt::CaseSensitive).size();
        QString verdict = QStringLiteral("SCENARIO OK");
        if (skipped > 0) {
            verdict += QStringLiteral(" (%1 étape(s) sautée(s))").arg(skipped);
        }
        printLine(ok ? verdict : QStringLiteral("SCENARIO EN ECHEC"));
        code = ok ? 0 : 1;
        QCoreApplication::exit(code);
    });
    QGuiApplication::exec();
    return code;
}
