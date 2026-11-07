#pragma once

#include <actcraft/core/containers.h>
#include <actcraft/core/lift.h>
#include <actcraft/tools/interactionpolicy.h>
#include <actcraft/tools/tool.h>
#include <actcraft/tools/toolcontext.h>

#include <cstddef>
#include <cstdint>
#include <functional>
#include <optional>
#include <vector>

namespace actcraft::tools {

// Sélection (F-210, F-520 à F-528, F-541, F-050).
class SelectTool final : public Tool
{
public:
    explicit SelectTool(ToolContext& context);
    QString id() const override; // « select »
    void activate() override;
    void deactivate() override; // garde la sélection, masque le cadre
    void onSample(const input::Sample& sample) override;
    bool isBusy() const noexcept override;
    void cancel() override;
    // Touches Maj et Ctrl : un échantillon n'en porte pas. L'appelant transmet chaque changement par onKey
    // (appui : Qt::Key_Shift ou Qt::Key_Control avec le modificateur ; relâché : les mêmes avec les
    // modificateurs restants) ; toute touche transmise remplace les modificateurs connus (L3-T8).
    bool onKey(int key, int modifiers) override; // Suppr : supprimer ; Échap : abandon du geste
    void selectAll();
    void clearSelection();
    void deleteSelection(); // sans sélection : rien (F-541)
    void enlarge();         // × kEnlargeStep autour du centre (F-523)
    void reduce();          // ÷ kEnlargeStep
    void setTranslucency(std::uint8_t value);
    void refresh(); // la page a changé (annuler, page suivante) : sélection filtrée, cadre recalculé
    // Mode points de forme (F-527, `object.shape-points`, L5-T16) : la forme sélectionnée seule montre ses
    // points (SelectionView::shapePoints) ; glisser un point le déplace en une étape. Échap au repos quitte
    // le mode.
    void setShapePointsMode(bool on);
    bool shapePointsMode() const noexcept;
    // Lot 8 (L8-T16, D8-9) : réglages du mode conception, posés par ui::ToolHost ; nul : tout actif. Non
    // possédée, lue à chaque geste.
    void setInteractionPolicy(const InteractionPolicy* policy) noexcept;
    // Dépôt accepté par un conteneur (F-577) : le conteneur gagnant et son son de confirmation (joué par
    // L8-T25). Appelé une fois, après le dépôt.
    using ContainerAccepted = std::function<void(core::ObjectId container, bool chime)>;
    void setContainerAccepted(ContainerAccepted callback);
    // Mesure (tests, banc) : pages d'essai construites depuis la création de l'outil. Au plus une par geste,
    // au relâché d'un glisser soumis à une règle ; aucune pendant les mouvements (fluidité).
    std::size_t probePagesBuilt() const noexcept;

private:
    enum class State : std::uint8_t
    {
        Idle,
        Pressed,    // appui posé, seuil de glissement pas franchi
        RubberBand, // rectangle élastique en cours
        Dragging,   // objets soulevés, couche soulevée déplacée
        Resizing,   // objets soulevés, poignée glissée (F-520)
        Rotating,   // objets soulevés, bouton de rotation glissé (F-522)
        SmartPoint, // forme soulevée, point intelligent glissé (F-242)
        ShapePoint, // forme soulevée, point de forme glissé (F-527, L5-T16)
    };
    enum class Grab : std::uint8_t
    {
        None,
        Band,       // appui dans le vide (ou avec Maj/Ctrl) : rectangle
        Drag,       // appui sur un objet ou dans le cadre : déplacement
        FreeMove,   // bouton « déplacer librement » : déplace même un objet verrouillé ou limité
        Handle,     // poignée : redimensionner
        Rotate,     // bouton de rotation
        Command,    // autre bouton de la barre : un clic le déclenche
        SmartPoint, // point intelligent d'une forme seule
        ShapePoint, // point de forme d'une forme seule, en mode points de forme
    };

    void press(const input::Sample& sample);
    void move(const input::Sample& sample);
    void release(const input::Sample& sample);
    void beginDrag(bool force);
    // À l'appui sur un objet à chemin hors de son trait : le glisser commence, l'objet saute sur son trait.
    void jumpOntoTrackAtPress();
    void beginTransform(State kind);
    void beginSmartPoint();
    void updateSmartPoint(core::PointF point);
    void endSmartPoint(bool cancelled);
    // Mode points de forme : points de la forme sélectionnée seule (page), vides hors du mode.
    std::vector<core::PointF> shownShapePoints(const std::vector<core::ObjectId>& selection) const;
    void beginShapePoint();
    void updateShapePoint(core::PointF point);
    void endShapePoint(bool cancelled);
    // Soulève ids (m_lifted d'abord), montre les copies et passe à state.
    void liftForGesture(std::vector<core::ObjectId> ids, State state);
    // Agrandir et Réduire : la sélection redimensionnable, × factor autour du centre du cadre.
    void scaleSelection(double factor);
    // Non constante : la rotation à pas retient le dernier angle permis (allowedTurn, L8-T15).
    core::Transform gestureTransform(core::PointF point);
    bool restrictionsOn() const noexcept;
    bool blockersOn() const noexcept;
    // Revue L8-T16 : hors mode conception, une sélection de plusieurs objets dont l'un a une rotation réglée
    // (bloquée, à sens, à pas, à pivot) ne tourne pas ; le bouton de rotation quitte la barre.
    bool turnRefused(const core::Page& page, const std::vector<core::ObjectId>& selection) const;
    QStringList barCommands(const core::Page& page, const std::vector<core::ObjectId>& selection) const;
    // Règles du glisser (L8-T16), prises avant le soulèvement : limiteurs, boîtes des bloquants, rangs des
    // soulevés s'il faudra une page d'essai. Sans règle, rien n'est copié.
    void prepareRules(const core::Page& page, const std::vector<core::ObjectId>& ids);
    // Déplacement permis (limiteurs puis objets bloquants) pour le déplacement voulu, depuis le départ.
    core::PointF allowedDelta(core::PointF wanted) const;
    // Verdict du dépôt sur la page d'essai, construite ici (resolveDrop) ; accrochage ajouté à `delta` s'il
    // passe les limiteurs et les bloquants et ne change pas le verdict.
    core::DropResult settle(core::Transform& delta);
    void updateDrag(core::PointF point);
    void endDrag(core::PointF point, bool cancelled);
    void showBand(core::PointF point);
    void showFrame();
    void publish(const SelectionView& view);
    void reset() noexcept;

    ToolContext& m_context;
    State m_state = State::Idle;
    Grab m_grab = Grab::None;
    std::uint64_t m_pointer = 0;
    core::PointF m_start;
    bool m_jumpedAtPress = false;          // le glisser a commencé à l'appui (saut sur le trait)
    core::ObjectId m_pendingSelect = 0;    // objet saisi non sélectionné : sélectionné si ce n'est qu'un clic
    bool m_dragSelection = false;          // le glissement déplace la sélection (le cadre la suit)
    bool m_additive = false;               // rectangle avec Maj/Ctrl : ajoute
    std::vector<core::ObjectId> m_base;    // sélection avant un rectangle additif
    std::vector<core::ObjectId> m_grabbed; // objets visés par le glissement
    std::vector<core::ObjectId> m_lifted;  // objets effectivement soulevés
    std::optional<core::PageObject> m_firstLifted; // copie du premier objet soulevé (ancrage de l'accrochage)
    SelectionView m_frame;                         // cadre de la sélection au soulèvement
    HandleKind m_handle = HandleKind::TopLeft;     // poignée saisie
    QString m_command;                             // bouton de barre saisi
    std::size_t m_smartPoint = 0;                  // point intelligent saisi
    std::vector<double> m_smartParameters;         // paramètres du dernier aperçu
    bool m_smartMoved = false;                     // l'aperçu a changé au moins une fois
    bool m_trashHover = false;
    bool m_shapePointsMode = false;             // object.shape-points (F-527)
    std::size_t m_shapePoint = 0;               // point de forme saisi
    std::optional<core::PointF> m_shapePointTo; // position du dernier aperçu
    std::vector<core::ObjectId> m_copied;       // soulevés à dragCopy : laissés en place, copiés au dépôt
    std::vector<core::PageObject>
        m_copySources; // leur fermeture (closureForCopy), prise avant le soulèvement
    int m_modifiers = 0;
    const InteractionPolicy* m_policy = nullptr;
    ContainerAccepted m_containerAccepted;
    // Règles du glisser (L8-T16), prises avant le soulèvement ; rien n'y est recalculé pendant les
    // mouvements.
    core::Page m_travelPage;                 // soulevés limités, leurs chemins et membres (closureForCopy)
    std::vector<std::size_t> m_limited;      // index dans m_travelPage des soulevés à déplacement limité
    std::vector<core::RectF> m_movingBoxes;  // boîtes de départ des soulevés (s'il y a un bloquant)
    std::vector<core::RectF> m_blockerBoxes; // boîtes des objets bloquants non soulevés
    bool m_settle = false;                   // conteneur ou aimant : page d'essai au relâché
    std::vector<std::size_t> m_liftRanks;    // rangs d'origine des soulevés (m_settle)
    std::vector<core::LiftedObject> m_liftedAll; // copies des soulevés à leur rang, pour la page d'essai
    bool m_force = false;                        // « déplacer librement » : limiteurs de déplacement ignorés
    bool m_turning = false;                      // rotation à règle (pas, sens, pivot) d'un objet seul
    double m_turnBase = 0;                       // angle de l'objet au départ (degrés)
    double m_turnFrom = 0;                       // dernier angle permis (allowedTurn)
    core::PointF m_pivot;                        // pivot de la rotation à règle (page)
    std::size_t m_probePagesBuilt = 0;           // probePagesBuilt()
};

} // namespace actcraft::tools
