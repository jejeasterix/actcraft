// Talon L11 : rempli par L11-T4.
#pragma once

#include <actcraft/input/routing.h>

#include <cstdint>
#include <memory>
#include <string_view>

namespace actcraft::input {

enum class TouchMode : std::uint8_t
{
    Ignore, // le doigt ne fait rien
    Pen,    // le doigt écrit avec le stylo de l'utilisateur
    Select, // le doigt sélectionne
    Full,   // gestion complète : outil courant, gestes à deux doigts (décision 25)
};

std::string_view touchModeId(TouchMode mode) noexcept;              // "ignore", "pen", "select", "full"
bool touchModeFromId(std::string_view id, TouchMode& out) noexcept; // faux pour un identifiant inconnu

class IgnoreTouchPolicy final : public RoutingPolicy
{
public:
    Decision decide(const Sample& press, const RouterState& state) const override;
};
class PenTouchPolicy final : public RoutingPolicy
{
public:
    Decision decide(const Sample& press, const RouterState& state) const override;
};
class SelectTouchPolicy final : public RoutingPolicy
{
public:
    Decision decide(const Sample& press, const RouterState& state) const override;
};

std::unique_ptr<RoutingPolicy> makeTouchPolicy(TouchMode mode);

} // namespace actcraft::input
