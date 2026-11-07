#include <actcraft/input/twouserpolicy.h>

#include <QPointF>

namespace actcraft::input {

TwoUserPolicy::TwoUserPolicy(QRectF secondUserArea) : m_area(secondUserArea) {}

void TwoUserPolicy::setSecondUserArea(QRectF area)
{
    m_area = area;
}

UserId TwoUserPolicy::stylusUser(std::uint64_t deviceId) const noexcept
{
    if (deviceId == 0) {
        return kFirstUser; // appareil sans identifiant : l'utilisateur 0, sans être retenu
    }
    if (m_firstStylus == 0) {
        m_firstStylus = deviceId;
    }
    return deviceId == m_firstStylus ? kFirstUser : kSecondUser;
}

Decision TwoUserPolicy::decide(const Sample& press, const RouterState& state) const
{
    Decision decision; // fingerTool reste Current
    if (press.device != DeviceKind::Touch) {
        decision.route = Route::Tool;
        decision.user = press.device == DeviceKind::Stylus ? stylusUser(press.deviceId) : kFirstUser;
        // M4 b par utilisateur (L11-C1) : la paume de cet utilisateur, posée avant, cesse d'écrire.
        decision.endToolContacts = state.toolTouchesByUser[decision.user] > 0;
        decision.endUserContactsOnly = true;
        return decision;
    }
    decision.user = m_area.contains(QPointF(press.x, press.y)) ? kSecondUser : kFirstUser;
    const bool palm = state.stylusDownByUser[decision.user];
    const bool extraFinger = state.toolTouchesByUser[decision.user] > 0;
    decision.route = palm || extraFinger ? Route::Ignore : Route::Tool;
    return decision;
}

std::uint64_t TwoUserPolicy::firstStylusDevice() const noexcept
{
    return m_firstStylus;
}

} // namespace actcraft::input
