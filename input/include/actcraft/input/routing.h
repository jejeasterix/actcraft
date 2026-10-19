#pragma once

#include <actcraft/input/sample.h>

#include <cstdint>
#include <functional>
#include <memory>
#include <unordered_map>

namespace actcraft::input {

using UserId = std::uint8_t;
inline constexpr UserId kFirstUser = 0;

enum class Route : std::uint8_t
{
    Tool,    // l'outil courant de l'utilisateur reçoit les échantillons du contact
    Gesture, // le geste à deux doigts de la vue
    Ignore,  // rien (paume, troisième doigt)
    Cover,   // une surcouche déclarée couvre le point d'appui : la vue ne prend rien (socle, D-S1)
};

struct RouterState
{
    bool stylusDown = false; // un contact de stylet ou de souris est en cours
    int toolTouches = 0;     // doigts en cours routés vers l'outil
    int gestureTouches = 0;  // doigts en cours routés vers le geste
};

struct Decision
{
    Route route = Route::Ignore;
    UserId user = kFirstUser;
    bool endToolContacts =
        false; // les doigts en cours vers l'outil passent au geste : la vue termine leur trait
};

// Politique de routage (DA-A1), remplaçable : décide à l'appui ; la décision vaut jusqu'au relâché.
class RoutingPolicy
{
public:
    virtual ~RoutingPolicy() = default;
    virtual Decision decide(const Sample& press, const RouterState& state) const = 0;
};

// Gestion complète du toucher (décision 25, N2-A5) : stylet et souris → outil ; doigt pendant un contact de
// stylet ou de souris → rien (paume) ; premier doigt → outil ; deuxième doigt → geste, et le premier y passe
// (endToolContacts) ; au-delà de deux doigts → rien. Un seul utilisateur (kFirstUser) jusqu'au lot 11.
class FullTouchPolicy final : public RoutingPolicy
{
public:
    Decision decide(const Sample& press, const RouterState& state) const override;
};

class PointerRouter
{
public:
    PointerRouter();                                       // FullTouchPolicy
    void setPolicy(std::unique_ptr<RoutingPolicy> policy); // les contacts en cours gardent leur décision
    // À l'appui, une décision retenue pour le même pointeur est remplacée ; la politique décide et la
    // décision est retenue pour `sample.pointer` (stylet et souris : 0) ; ensuite, la décision retenue ; au
    // relâché, elle est rendue puis oubliée. Mouvement d'un contact inconnu : Ignore. endToolContacts : les
    // décisions retenues des doigts routés vers l'outil passent à Gesture.
    Decision route(const Sample& sample);
    int gestureTouches() const noexcept;
    void reset(); // tout contact oublié (perte de saisie)
    // Consulté à l'appui, avant la politique : vrai → Route::Cover rendue, sans effet sur RouterState.
    void setCoverTest(std::function<bool(const Sample&)> test);
    // Contact terminé ou non pris par la vue (deuxième doigt, annulation, appui couvert) : sa décision est
    // oubliée.
    void release(std::uint64_t pointer);

private:
    std::unique_ptr<RoutingPolicy> m_policy;
    std::unordered_map<std::uint64_t, Decision> m_contacts;
    RouterState m_state;
    std::function<bool(const Sample&)> m_coverTest;
};

} // namespace actcraft::input
