#pragma once

#include <actcraft/input/sample.h>

#include <cstdint>
#include <functional>
#include <memory>
#include <span>
#include <unordered_map>
#include <vector>

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

// Outil qui reçoit un contact routé vers l'outil (D11-4).
enum class FingerTool : std::uint8_t
{
    Current, // l'outil courant de l'utilisateur (stylet, souris, doigt en mode full)
    Pen,     // le stylo de l'utilisateur, avec ses réglages, quel que soit l'outil courant
    Select,  // le sélecteur, quel que soit l'outil courant
};

struct RouterState
{
    bool stylusDown = false; // un contact de stylet ou de souris est en cours
    int toolTouches = 0;     // doigts en cours routés vers l'outil
    int gestureTouches = 0;  // doigts en cours routés vers le geste
    int pressesInEvent = 1;  // appuis de doigt non couverts du même événement (H3-16), posé par routePresses
};

struct Decision
{
    Route route = Route::Ignore;
    UserId user = kFirstUser;
    // Appui d'un doigt seul dans son événement : ces doigts passent à Gesture ; appui d'un stylet, de la
    // souris ou de doigts posés ensemble (pressesInEvent ≥ 2) : à Ignore (M4 b).
    bool endToolContacts =
        false; // les doigts en cours vers l'outil passent à Gesture ou à Ignore : la vue termine leur trait
    FingerTool fingerTool = FingerTool::Current; // outil imposé au contact (modes pen et select, D11-4)
};

// Politique de routage (DA-A1), remplaçable : décide à l'appui ; la décision vaut jusqu'au relâché.
class RoutingPolicy
{
public:
    virtual ~RoutingPolicy() = default;
    virtual Decision decide(const Sample& press, const RouterState& state) const = 0;
};

// Gestion complète du toucher (décision 25, N2-A5) : stylet et souris → outil ; doigt pendant un contact de
// stylet ou de souris → rien (paume, testée avant toute autre règle) ; premier doigt → outil ; deux doigts
// posés dans le même événement (pressesInEvent ≥ 2, H3-16) → tous deux au geste, sans point versé à l'outil ;
// deuxième doigt → geste, et le doigt qui écrit y passe (endToolContacts), ou passe à rien si les doigts du
// geste sont posés ensemble ; au-delà de deux doigts → rien. Un seul utilisateur (kFirstUser) jusqu'au
// lot 11.
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
    // décisions retenues des doigts routés vers l'outil passent à Gesture si l'appui vient d'un doigt
    // seul dans son événement, à Ignore sinon (stylet, souris, doigts posés ensemble ; M4 b).
    Decision route(const Sample& sample);
    // Appuis d'un même événement (H3-16) : décidés ensemble, dans l'ordre, chacun comme par route(), la
    // politique voyant `pressesInEvent` = nombre d'appuis de doigt de l'événement que la surcouche ne couvre
    // pas. En mode full, deux doigts posés ensemble vont tous deux au geste ; un doigt qui écrivait déjà
    // passe alors à Ignore (endToolContacts : son trait se termine) et ne rejoint pas le geste. Un
    // échantillon qui n'est pas un appui suit route().
    std::vector<Decision> routePresses(std::span<const Sample> presses);
    int gestureTouches() const noexcept;
    void reset(); // tout contact oublié (perte de saisie)
    // Consulté à l'appui, avant la politique : vrai → Route::Cover rendue, sans effet sur RouterState.
    void setCoverTest(std::function<bool(const Sample&)> test);
    // Contact terminé ou non pris par la vue (deuxième doigt, annulation, appui couvert) : sa décision est
    // oubliée.
    void release(std::uint64_t pointer);

private:
    // Appui : la décision retenue du même pointeur remplacée, Cover si `covered`, sinon la politique, qui
    // voit `pressesInEvent`.
    Decision decidePress(const Sample& sample, bool covered, int pressesInEvent);

    std::unique_ptr<RoutingPolicy> m_policy;
    std::unordered_map<std::uint64_t, Decision> m_contacts;
    RouterState m_state;
    std::function<bool(const Sample&)> m_coverTest;
};

} // namespace actcraft::input
