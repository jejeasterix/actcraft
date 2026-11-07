#include <actcraft/input/touchmodes.h>

#include <array>
#include <utility>

namespace actcraft::input {

namespace {

// Identifiants du réglage du poste `workstation/touch-mode` (D11-3).
constexpr std::array<std::pair<TouchMode, std::string_view>, 4> kTouchModeIds{{
    {TouchMode::Ignore, "ignore"},
    {TouchMode::Pen, "pen"},
    {TouchMode::Select, "select"},
    {TouchMode::Full, "full"},
}};

// Stylet, souris, autre : l'outil courant, dans tous les modes (D11-4). Comme en mode full (M4 b), un doigt
// qui écrit cède la main : le routeur le passe à Ignore et la vue termine son trait.
Decision pointerDecision(const RouterState& state)
{
    Decision decision;
    decision.route = Route::Tool;
    decision.user = kFirstUser;
    decision.endToolContacts = state.toolTouches > 0;
    return decision;
}

// Modes pen et select : un seul doigt à la fois, avec l'outil imposé, jamais de geste (D11-3).
Decision imposedToolDecision(const Sample& press, const RouterState& state, FingerTool tool)
{
    if (press.device != DeviceKind::Touch) {
        return pointerDecision(state);
    }
    Decision decision;
    decision.user = kFirstUser;
    if (state.stylusDown || state.toolTouches > 0 || state.gestureTouches > 0) {
        decision.route = Route::Ignore; // paume, ou doigt suivant : aucun geste hors du mode full
        return decision;
    }
    decision.route = Route::Tool;
    decision.fingerTool = tool;
    return decision;
}

} // namespace

std::string_view touchModeId(TouchMode mode) noexcept
{
    for (const auto& [entry, id] : kTouchModeIds) {
        if (entry == mode) {
            return id;
        }
    }
    return {};
}

bool touchModeFromId(std::string_view id, TouchMode& out) noexcept
{
    for (const auto& [mode, entry] : kTouchModeIds) {
        if (entry == id) {
            out = mode;
            return true;
        }
    }
    return false;
}

Decision IgnoreTouchPolicy::decide(const Sample& press, const RouterState& state) const
{
    if (press.device != DeviceKind::Touch) {
        return pointerDecision(state);
    }
    Decision decision;
    decision.route = Route::Ignore;
    decision.user = kFirstUser;
    return decision;
}

Decision PenTouchPolicy::decide(const Sample& press, const RouterState& state) const
{
    return imposedToolDecision(press, state, FingerTool::Pen);
}

Decision SelectTouchPolicy::decide(const Sample& press, const RouterState& state) const
{
    return imposedToolDecision(press, state, FingerTool::Select);
}

std::unique_ptr<RoutingPolicy> makeTouchPolicy(TouchMode mode)
{
    switch (mode) {
    case TouchMode::Ignore:
        return std::make_unique<IgnoreTouchPolicy>();
    case TouchMode::Pen:
        return std::make_unique<PenTouchPolicy>();
    case TouchMode::Select:
        return std::make_unique<SelectTouchPolicy>();
    case TouchMode::Full:
        break;
    }
    return std::make_unique<FullTouchPolicy>();
}

} // namespace actcraft::input
