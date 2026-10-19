#include <actcraft/input/routing.h>

#include <utility>

namespace actcraft::input {

namespace {

// Stylet et souris partagent ce contact ; un doigt a l'identifiant de son point + 1 (qtevents.cpp).
constexpr std::uint64_t kStylusOrMouse = 0;

// État recalculé des décisions retenues (appui, relâché, oubli : jamais à chaque mouvement). Une décision
// Cover n'y compte jamais (D-S1) ; stylusDown n'est tenu que par une décision Tool.
RouterState stateOf(const std::unordered_map<std::uint64_t, Decision>& contacts)
{
    RouterState state;
    for (const auto& [pointer, decision] : contacts) {
        if (pointer == kStylusOrMouse) {
            state.stylusDown = state.stylusDown || decision.route == Route::Tool;
        } else if (decision.route == Route::Tool) {
            ++state.toolTouches;
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
        decision.route = Route::Tool; // premier doigt
    } else if (state.toolTouches == 1 && state.gestureTouches == 0) {
        // Deuxième doigt : le geste commence, le premier doigt y passe ; son trait est abandonné (H3-17).
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

Decision PointerRouter::route(const Sample& sample)
{
    if (sample.phase == Phase::Press) {
        // Un appui redécide : la décision retenue du même pointeur (relâché jamais vu) est remplacée.
        m_contacts.erase(sample.pointer);
        m_state = stateOf(m_contacts);
        if (m_coverTest && m_coverTest(sample)) {
            // Route de la politique (D-S1) : retenue pour tout le contact, sans effet sur RouterState.
            Decision cover;
            cover.route = Route::Cover;
            cover.user = kFirstUser;
            m_contacts[sample.pointer] = cover;
            return cover;
        }
        Decision decision = m_policy ? m_policy->decide(sample, m_state) : Decision{};
        if (decision.endToolContacts) {
            // M4 (b) : la décision vit dans la politique, pas dans la vue (modes du toucher du lot 11). Un
            // deuxième doigt fait passer le premier au geste ; un stylet ou une souris qui prend la main le
            // fait passer à Ignore : il n'écrit plus et ne reste pas compté comme un doigt du geste (un doigt
            // suivant ferait sinon un pincement avec lui).
            const Route moved = sample.device == DeviceKind::Touch ? Route::Gesture : Route::Ignore;
            for (auto& [pointer, held] : m_contacts) {
                if (pointer != kStylusOrMouse && held.route == Route::Tool) {
                    held.route = moved;
                }
            }
        }
        m_contacts[sample.pointer] = decision;
        m_state = stateOf(m_contacts);
        return decision;
    }
    const auto found = m_contacts.find(sample.pointer);
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
    if (m_contacts.erase(pointer) > 0) {
        m_state = stateOf(m_contacts);
    }
}

} // namespace actcraft::input
