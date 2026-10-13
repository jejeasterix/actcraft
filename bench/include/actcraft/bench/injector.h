#pragma once

#include <actcraft/bench/scenarios.h>

#include <QObject>
#include <QPointingDevice>
#include <QTimer>

#include <functional>
#include <memory>
#include <vector>

class QWindow;
class QEvent;

namespace actcraft::bench {

// Entrées synthétiques postées à la fenêtre (DA6), avec rattrapage : à chaque déclenchement, tous les
// échantillons dont l'instant prévu est passé partent d'un coup, chacun gardant son instant prévu. Un
// minuteur périodique à un événement par réveil garderait le retard d'un fil principal bloqué et ne le
// rattraperait jamais (décision 23). Ni injection native ni souris pilotée. Les événements postés pointent
// vers les périphériques de l'injecteur : il doit vivre jusqu'à leur traitement.
class Injector : public QObject
{
    Q_OBJECT

public:
    using PageToScene = std::function<QPointF(QPointF)>;

    Injector(QWindow* target, PageToScene pageToScene, QObject* parent = nullptr);
    ~Injector() override;

    const QPointingDevice* stylus() const noexcept;
    const QPointingDevice* wheel() const noexcept;

    // Instants prévus : t0Ns + offsetNs. autoTick : minuteur précis ; sinon tick() à la main (tests).
    void start(std::vector<PlannedInput> inputs, std::int64_t t0Ns, bool autoTick = true);
    // Poste tous les événements dont l'instant prévu est passé ; retourne leur nombre.
    std::size_t tick(std::int64_t nowNs);
    // Abandon : arrête le minuteur, plus rien n'est posté (tick() compris) jusqu'au prochain start() ;
    // planFinished n'est pas émis et finished() reste faux si des échantillons n'ont pas été postés.
    void stop();
    bool finished() const noexcept;
    // Tests : précision et échéance du réveil.
    const QTimer& wakeTimer() const noexcept;

    const std::vector<PlannedInput>& inputs() const noexcept;
    const std::vector<std::int64_t>& plannedNs() const noexcept; // absolus
    const std::vector<std::int64_t>& postedNs() const noexcept;  // absolus ; −1 si pas encore posté

signals:
    void planFinished();

private:
    void scheduleNext();
    QEvent* makeEvent(const PlannedInput& input) const;

    QWindow* m_target;
    PageToScene m_pageToScene;
    std::unique_ptr<QPointingDevice> m_stylus;
    std::unique_ptr<QPointingDevice> m_wheel;
    QTimer m_timer;
    std::vector<PlannedInput> m_inputs;
    std::vector<std::int64_t> m_planned;
    std::vector<std::int64_t> m_posted;
    std::size_t m_next = 0;
    bool m_autoTick = true;
    bool m_finishedSignalled = false;
    bool m_stopped = false;
};

} // namespace actcraft::bench
