// Talon L11 : rempli par L11-T7.
#pragma once

#include <actcraft/input/routing.h>

#include <QRectF>

#include <cstdint>

namespace actcraft::input {

class TwoUserPolicy final : public RoutingPolicy
{
public:
    explicit TwoUserPolicy(QRectF secondUserArea);
    void setSecondUserArea(QRectF area);
    Decision decide(const Sample& press, const RouterState& state) const override;
    // Appareils de stylet vus, dans l'ordre (le premier est l'utilisateur 0) ; réinitialisé à la
    // construction.
    std::uint64_t firstStylusDevice() const noexcept;

private:
    QRectF m_area;
    mutable std::uint64_t m_firstStylus = 0; // 0 : aucun vu
};

} // namespace actcraft::input
