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

// Talon L0-S : rempli par L3-T5.
// Politique minimale du socle (L0-S7) : stylet et souris → outil ; doigt pendant un contact de stylet ou de
// souris routé vers l'outil → rien (paume) ; sinon → outil. Ni geste ni M4 : L3-T5.
Decision FullTouchPolicy::decide(const Sample& press, const RouterState& state) const
{
    Decision decision;
    decision.user = kFirstUser;
    decision.route = press.device == DeviceKind::Touch && state.stylusDown ? Route::Ignore : Route::Tool;
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
            for (auto& [pointer, held] : m_contacts) {
                if (pointer != kStylusOrMouse && held.route == Route::Tool) {
                    held.route = Route::Gesture;
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
