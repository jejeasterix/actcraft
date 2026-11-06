#pragma once

namespace actcraft::tools {
// Réglages du mode conception lus par les outils (D8-9) ; posée par ui::ToolHost.
class InteractionPolicy
{
public:
    virtual ~InteractionPolicy() = default;
    virtual bool actionsEnabled() const = 0;      // faux : un clic sur un objet d'action le sélectionne
    virtual bool restrictionsEnabled() const = 0; // faux : limiteurs, rotation et conteneurs ignorés
    virtual bool blockersEnabled() const = 0;     // faux : objets bloquants ignorés
};
} // namespace actcraft::tools
