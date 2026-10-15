#pragma once

#include <actcraft/bench/injector.h>
#include <actcraft/bench/options.h>
#include <actcraft/bench/recorder.h>
#include <actcraft/bench/result.h>

#include <QImage>
#include <QList>
#include <QString>
#include <QStringList>

#include <cstdint>
#include <functional>
#include <memory>
#include <vector>

class QQuickWindow;

namespace actcraft::render {
class FixedPageItem;
}
namespace actcraft::ui {
class PageView;
}

namespace actcraft::bench {

// Écrit les captures (une par étape de captureSteps()) dans `dir`, créé si besoin. Chaque échec est ajouté à
// `errors` sous la forme « error: capture … ».
void saveCaptures(const QString& dir, const QString& graphicsApi, const QList<QImage>& images,
                  QStringList& errors);

// Vue de la démonstration (DA7) : 100 %, centrée sur les points lus par ses contrôles de contenu, quelle que
// soit la position laissée par les scénarios. Ne déplace que la vue, jamais l'interface.
void showDemoTargets(ui::PageView& view);

// Joue un passage complet du banc sur la fenêtre déjà chargée ; ne quitte pas l'application.
// DURÉE DE VIE : le Runner doit être détruit APRÈS la fenêtre. Qt Quick garde les périphériques synthétiques
// de l'injecteur, et le collecteur reçoit des appels directs du fil de rendu (bench/app/main.cpp le déclare
// avant le moteur QML).
class Runner
{
public:
    explicit Runner(BenchOptions options);
    ~Runner();
    Runner(const Runner&) = delete;
    Runner& operator=(const Runner&) = delete;

    // Une seule fois par Runner.
    BenchResult run(QQuickWindow* window);

private:
    bool waitFor(const std::function<bool()>& condition, int timeoutMs);
    void waitMs(int ms);
    bool placeWindow(BenchResult& result);
    // Chauffe en rendu continu ; retourne la médiane des intervalles (0 si non mesurable).
    std::int64_t warmUp(int ms);
    bool setLevel(const ScenarioPlan& plan);
    // Toute injection passe par ici (scénarios, et le trait de démonstration) : la numérotation de la chaîne
    // d'entrée repart de 0, donc numéro de séquence = indice de l'événement dans `inputs`.
    void startInputs(std::vector<PlannedInput> inputs, std::int64_t t0Ns);
    // Séquence de démonstration non mesurée : captures en mémoire, contrôles de contenu, PNG si demandés.
    void runDemo(BenchResult& result);
    ScenarioResult play(const ScenarioPlan& plan, std::int64_t refreshPeriodNs, QStringList& invalidReasons);

    QQuickWindow* m_window = nullptr;
    BenchOptions m_options;
    ui::PageView* m_view = nullptr;
    render::FixedPageItem* m_fixed = nullptr;
    std::unique_ptr<FrameRecorder> m_recorder;
    std::unique_ptr<Injector> m_injector;
};

} // namespace actcraft::bench
