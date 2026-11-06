#include <actcraft/core/interactivity.h>

namespace actcraft::core {

void setTravel(Restrictions& r, Travel t) noexcept
{
    r.travel = t;
    r.movable = t != Travel::Fixed;
}

void setTurning(Restrictions& r, Turning t) noexcept
{
    r.turning = t;
    r.rotatable = t != Turning::Fixed;
}

void normalize(Restrictions& r) noexcept
{
    if (!r.movable) {
        r.travel = Travel::Fixed;
    } else if (r.travel == Travel::Fixed) {
        r.movable = false;
    }
    if (!r.rotatable) {
        r.turning = Turning::Fixed;
    } else if (r.turning == Turning::Fixed) {
        r.rotatable = false;
    }
}

// Liaison structurée : un champ ajouté à Restrictions casse la compilation ici au lieu d'échapper au défaut.
bool isDefault(const Restrictions& r) noexcept
{
    const auto& [movable, resizable, rotatable, travel, track, turning, turnStep, pivot, pivotPoint,
                 pivotObject, blocksOthers, magnet, magnetMark, magnetPoint] = r;
    static_cast<void>(movable); // champs du lot 2 : hors de ce défaut
    static_cast<void>(resizable);
    static_cast<void>(rotatable);
    const Restrictions d;
    return travel == d.travel && track == d.track && turning == d.turning && turnStep == d.turnStep &&
           pivot == d.pivot && pivotPoint.x == d.pivotPoint.x && pivotPoint.y == d.pivotPoint.y &&
           pivotObject == d.pivotObject && blocksOthers == d.blocksOthers && magnet == d.magnet &&
           magnetMark == d.magnetMark && magnetPoint.x == d.magnetPoint.x && magnetPoint.y == d.magnetPoint.y;
}

bool isDefault(const ContainerRule& c) noexcept
{
    const auto& [accepts, object, keywords, fit, tolerance, bounceBack, chime, chimeSound] = c;
    const ContainerRule d;
    return accepts == d.accepts && object == d.object && keywords.empty() && fit == d.fit &&
           tolerance == d.tolerance && bounceBack == d.bounceBack && chime == d.chime && chimeSound.empty();
}

bool isSupported(const Action& a) noexcept
{
    return !a.catalogId.empty();
}

} // namespace actcraft::core
