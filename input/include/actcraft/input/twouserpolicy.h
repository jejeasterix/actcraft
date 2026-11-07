#pragma once

#include <actcraft/input/routing.h>

#include <QRectF>

#include <cstdint>

namespace actcraft::input {

// Mode à deux utilisateurs (D11-2, DA-A1, F-303). Utilisateur : souris (et autre appareil) → 0 ; stylet → 0
// pour le premier appareil vu (Sample::deviceId), 1 pour tout autre, 0 sans identifiant ; doigt → 1 dans
// secondUserArea, sinon 0. Route : stylet et souris → outil ; doigt → outil, sauf pendant un contact de
// stylet ou de souris du même utilisateur (paume) ou si ce même utilisateur a déjà un doigt sur l'outil
// (doigt de plus) : rien. Jamais Gesture (gestes suspendus), jamais endToolContacts : le routeur couperait
// aussi les doigts de l'autre utilisateur. Lit RouterState::stylusDownByUser et toolTouchesByUser.
class TwoUserPolicy final : public RoutingPolicy
{
public:
    // Aire de l'utilisateur 1 (moitié de la deuxième palette), comparée aux coordonnées des échantillons
    // routés (la vue route en coordonnées de page) : l'appelant la tient à jour quand la vue change.
    explicit TwoUserPolicy(QRectF secondUserArea);
    void setSecondUserArea(QRectF area);
    Decision decide(const Sample& press, const RouterState& state) const override;
    // Premier appareil de stylet vu (l'utilisateur 0) ; 0 : aucun ; réinitialisé à la construction.
    std::uint64_t firstStylusDevice() const noexcept;

private:
    UserId stylusUser(std::uint64_t deviceId) const noexcept; // retient le premier appareil vu

    QRectF m_area;
    mutable std::uint64_t m_firstStylus = 0; // 0 : aucun vu
};

} // namespace actcraft::input
