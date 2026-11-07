#include <actcraft/input/routing.h>

#include <algorithm>
#include <utility>

namespace actcraft::input {

namespace {

// Stylet et souris partagent ce contact ; un doigt a l'identifiant de son point + 1 (qtevents.cpp).
constexpr std::uint64_t kStylusOrMouse = 0;
// Stylet d'un appareil connu (Sample::deviceId, L11-T7) : sa propre clé, marquée du bit haut, distincte de
// la souris (0) et des doigts (petits entiers) ; deux stylets posés ensemble ne se remplacent pas.
constexpr std::uint64_t kStylusKey = std::uint64_t{1} << 63;

std::uint64_t contactKey(const Sample& sample) noexcept
{
    if (sample.device == DeviceKind::Stylus && sample.deviceId != 0) {
        return kStylusKey | sample.deviceId;
    }
    return sample.pointer;
}

bool isStylusOrMouse(std::uint64_t key) noexcept
{
    return key == kStylusOrMouse || (key & kStylusKey) != 0;
}

int userIndex(UserId user) noexcept
{
    return user == kFirstUser ? 0 : 1;
}

// État recalculé des décisions retenues (appui, relâché, oubli : jamais à chaque mouvement). Une décision
// Cover n'y compte jamais (D-S1) ; stylusDown n'est tenu que par une décision Tool.
RouterState stateOf(const std::unordered_map<std::uint64_t, Decision>& contacts)
{
    RouterState state;
    for (const auto& [key, decision] : contacts) {
        if (isStylusOrMouse(key)) {
            if (decision.route == Route::Tool) {
                state.stylusDown = true;
                state.stylusDownByUser[userIndex(decision.user)] = true;
            }
        } else if (decision.route == Route::Tool) {
            ++state.toolTouches;
            ++state.toolTouchesByUser[userIndex(decision.user)];
        } else if (decision.route == Route::Gesture) {
            ++state.gestureTouches;
        }
    }
    return state;
}

} // namespace

// Gestion complète du toucher (L3-T5, D12). Seule cette politique dit où va un contact : la vue n'y compte
// pas les doigts (le lot 11 change la politique, pas la vue).
Decision FullTouchPolicy::decide(const Sample& press, const RouterState& state) const
{
    Decision decision;
    decision.user = kFirstUser;
    if (press.device != DeviceKind::Touch) {
        // Stylet, souris, autre : l'outil. Un doigt qui écrit cède la main (M4 b) : le routeur le passe à
        // Ignore, la vue termine son trait.
        decision.route = Route::Tool;
        decision.endToolContacts = state.toolTouches > 0;
        // Pendant un pincement, les doigts du geste restent à Gesture et le stylet va à l'outil (H3-19).
        return decision;
    }
    if (state.stylusDown) {
        decision.route = Route::Ignore; // paume pendant un contact du stylet ou de la souris
    } else if (state.toolTouches + state.gestureTouches == 0) {
        // Premier doigt : l'outil ; posé avec un autre dans le même événement, le geste, sans point versé à
        // l'outil (H3-16).
        decision.route = state.pressesInEvent >= 2 ? Route::Gesture : Route::Tool;
    } else if (state.toolTouches == 1 && state.gestureTouches == 0) {
        // Deuxième doigt : le geste commence, le premier doigt y passe (ou passe à
        // Ignore si ce doigt arrive avec un autre, H3-16) ; son trait est abandonné (H3-17).
        decision.route = Route::Gesture;
        decision.endToolContacts = true;
    } else if (state.gestureTouches == 1) {
        decision.route = Route::Gesture; // un doigt resté du geste en retrouve un second
    } else {
        decision.route = Route::Ignore; // au-delà de deux doigts
    }
    return decision;
}

PointerRouter::PointerRouter() : m_policy(std::make_unique<FullTouchPolicy>()) {}

void PointerRouter::setPolicy(std::unique_ptr<RoutingPolicy> policy)
{
    m_policy = std::move(policy);
}

Decision PointerRouter::decidePress(const Sample& sample, bool covered, int pressesInEvent)
{
    // Un appui redécide : la décision retenue du même pointeur (relâché jamais vu) est remplacée.
    const std::uint64_t key = contactKey(sample);
    m_contacts.erase(key);
    m_state = stateOf(m_contacts);
    if (covered) {
        // Route de la politique (D-S1) : retenue pour tout le contact, sans effet sur RouterState.
        Decision cover;
        cover.route = Route::Cover;
        cover.user = kFirstUser;
        m_contacts[key] = cover;
        return cover;
    }
    RouterState state = m_state;
    state.pressesInEvent = pressesInEvent;
    Decision decision = m_policy ? m_policy->decide(sample, state) : Decision{};
    if (decision.endToolContacts) {
        // M4 (b) : la décision vit dans la politique, pas dans la vue (modes du toucher du lot 11). Un
        // deuxième doigt fait passer le premier au geste ; un stylet ou une souris qui prend la main le
        // fait passer à Ignore : il n'écrit plus et ne reste pas compté comme un doigt du geste (un doigt
        // suivant ferait sinon un pincement avec lui).
        // Doigts posés ensemble (H3-16) : ils font le geste à eux seuls, le doigt qui écrivait passe à
        // Ignore.
        const bool loneFinger = sample.device == DeviceKind::Touch && pressesInEvent < 2;
        const Route moved = loneFinger ? Route::Gesture : Route::Ignore;
        for (auto& [heldKey, held] : m_contacts) {
            if (!isStylusOrMouse(heldKey) && held.route == Route::Tool) {
                held.route = moved;
            }
        }
    }
    m_contacts[key] = decision;
    m_state = stateOf(m_contacts);
    return decision;
}

Decision PointerRouter::route(const Sample& sample)
{
    if (sample.phase == Phase::Press) {
        return decidePress(sample, m_coverTest && m_coverTest(sample), 1);
    }
    const auto found = m_contacts.find(contactKey(sample));
    if (found == m_contacts.end()) {
        return Decision{}; // contact inconnu (appui non vu, ou oublié) : Ignore
    }
    const Decision decision = found->second;
    if (sample.phase == Phase::Release) {
        m_contacts.erase(found);
        m_state = stateOf(m_contacts);
    }
    return decision;
}

std::vector<Decision> PointerRouter::routePresses(std::span<const Sample> presses)
{
    // Premier passage : le test de couverture, une fois par appui ; un doigt couvert ne compte pas parmi les
    // appuis de l'événement (il n'atteint pas la page). Second passage : chaque appui décidé dans l'ordre,
    // la politique voyant le nombre de doigts de l'événement.
    std::vector<Decision> decisions(presses.size());
    int fingers = 0;
    for (std::size_t i = 0; i < presses.size(); ++i) {
        const Sample& sample = presses[i];
        if (sample.phase != Phase::Press) {
            continue;
        }
        if (m_coverTest && m_coverTest(sample)) {
            decisions[i].route = Route::Cover;
        } else if (sample.device == DeviceKind::Touch) {
            ++fingers;
        }
    }
    const int pressesInEvent = std::max(fingers, 1);
    for (std::size_t i = 0; i < presses.size(); ++i) {
        const Sample& sample = presses[i];
        decisions[i] = sample.phase == Phase::Press
                           ? decidePress(sample, decisions[i].route == Route::Cover, pressesInEvent)
                           : route(sample);
    }
    return decisions;
}

int PointerRouter::gestureTouches() const noexcept
{
    return m_state.gestureTouches;
}

void PointerRouter::reset()
{
    m_contacts.clear();
    m_state = RouterState{};
}

void PointerRouter::setCoverTest(std::function<bool(const Sample&)> test)
{
    m_coverTest = std::move(test);
}

void PointerRouter::release(std::uint64_t pointer)
{
    // La vue relâche par Sample::pointer : 0 oublie la souris et tout stylet (clés par appareil comprises).
    const auto erased =
        pointer == kStylusOrMouse
            ? std::erase_if(m_contacts, [](const auto& entry) { return isStylusOrMouse(entry.first); })
            : m_contacts.erase(pointer);
    if (erased > 0) {
        m_state = stateOf(m_contacts);
    }
}

} // namespace actcraft::input
