#pragma once

#include <actcraft/bench/scenarios.h>

#include <QObject>
#include <QPointF>
#include <QPointingDevice>
#include <QTimer>

#include <cstdint>
#include <functional>
#include <memory>
#include <vector>

class QWindow;
class QEvent;

namespace actcraft::bench {

// Repère des points d'un geste direct : page (convertie en scène par `pageToScene`) ou fenêtre (tels quels).
enum class PointSpace : std::uint8_t
{
    Page,
    Window,
};

// Temps d'un geste direct : appui, déplacements, relâché.
enum class GesturePhase : std::uint8_t
{
    Press,
    Move,
    Release,
};

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

    // Gestes directs (L0-S2) : l'injecteur calcule la géométrie (points de scène) et le minutage (une image
    // entre deux pas d'un pincement, maintien du pointeur) et remet chaque temps à un récepteur ; il ne poste
    // aucun événement tactile ou souris (la livraison à une fenêtre est l'affaire du joueur de scénarios, qui
    // passe par l'API publique de Qt Test, hors du banc livré). Le récepteur d'un doigt reçoit un point par
    // doigt, dans l'ordre des identifiants. Faux, sans rien émettre, si un argument est invalide.
    using FingerSink = std::function<void(GesturePhase, const std::vector<QPointF>&)>;
    using PointerSink = std::function<void(GesturePhase, QPointF)>;
    // Point converti en scène selon son repère.
    QPointF scenePoint(QPointF point, PointSpace space) const;
    // Un doigt : appui au premier point, 16 déplacements par segment, relâché au dernier.
    bool touchPath(const std::vector<QPointF>& points, const FingerSink& sink,
                   PointSpace space = PointSpace::Page);
    // Deux doigts de part et d'autre du centre, alignés horizontalement ; écart (pixels de scène) de `from` à
    // `to` en `steps` pas, un par image. Appui à `from`, relâché à `to`.
    bool pinch(QPointF center, double from, double to, int steps, const FingerSink& sink,
               PointSpace space = PointSpace::Page);
    // Souris : appui au premier point, 16 déplacements par segment, maintien `holdMs` au dernier point,
    // relâché.
    bool pointerPath(const std::vector<QPointF>& points, const PointerSink& sink, int holdMs,
                     PointSpace space = PointSpace::Page);

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
