#pragma once

#include <actcraft/tools/tool.h>
#include <actcraft/tools/toolcontext.h>

#include <cstdint>
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

private:
    enum class State : std::uint8_t
    {
        Idle,
        Pressed,    // appui posé, seuil de glissement pas franchi
        RubberBand, // rectangle élastique en cours
        Dragging,   // objets soulevés, couche soulevée déplacée
        Resizing,   // objets soulevés, poignée glissée (F-520)
        Rotating,   // objets soulevés, bouton de rotation glissé (F-522)
    };
    enum class Grab : std::uint8_t
    {
        None,
        Band,     // appui dans le vide (ou avec Maj/Ctrl) : rectangle
        Drag,     // appui sur un objet ou dans le cadre : déplacement
        FreeMove, // bouton « déplacer librement » : déplace même un objet verrouillé ou limité
        Handle,   // poignée : redimensionner
        Rotate,   // bouton de rotation
        Command,  // autre bouton de la barre : un clic le déclenche
    };

    void press(const input::Sample& sample);
    void move(const input::Sample& sample);
    void release(const input::Sample& sample);
    void beginDrag(bool force);
    void beginTransform(State kind);
    // Soulève ids (m_lifted d'abord), montre les copies et passe à state.
    void liftForGesture(std::vector<core::ObjectId> ids, State state);
    // Agrandir et Réduire : la sélection redimensionnable, × factor autour du centre du cadre.
    void scaleSelection(double factor);
    core::Transform gestureTransform(core::PointF point) const;
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
    core::ObjectId m_pendingSelect = 0;    // objet saisi non sélectionné : sélectionné si ce n'est qu'un clic
    bool m_dragSelection = false;          // le glissement déplace la sélection (le cadre la suit)
    bool m_additive = false;               // rectangle avec Maj/Ctrl : ajoute
    std::vector<core::ObjectId> m_base;    // sélection avant un rectangle additif
    std::vector<core::ObjectId> m_grabbed; // objets visés par le glissement
    std::vector<core::ObjectId> m_lifted;  // objets effectivement soulevés
    SelectionView m_frame;                 // cadre de la sélection au soulèvement
    HandleKind m_handle = HandleKind::TopLeft; // poignée saisie
    QString m_command;                         // bouton de barre saisi
    bool m_trashHover = false;
    int m_modifiers = 0;
};

} // namespace actcraft::tools
