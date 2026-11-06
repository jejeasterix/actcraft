#pragma once

#include <actcraft/core/model.h>

// Interactivité (lot 8) : les modes de déplacement et de rotation tiennent d'accord les booléens du lot 2
// (`movable`, `rotatable`), qui restent la source du « bloqué » lue par les outils et par formats/.
namespace actcraft::core {

void setTravel(Restrictions& r, Travel t) noexcept;   // movable = t != Fixed
void setTurning(Restrictions& r, Turning t) noexcept; // rotatable = t != Fixed
void normalize(Restrictions& r) noexcept;             // booléen faux → mode Fixed ; mode Fixed → booléen faux
bool isDefault(const Restrictions& r) noexcept;       // ne regarde que les champs du lot 8
bool isDefault(const ContainerRule& c) noexcept;
bool isSupported(const Action& a) noexcept; // catalogId non vide

} // namespace actcraft::core
