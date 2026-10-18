#include <actcraft/input/routing.h>

#include <utility>

// Talon L0-S : rempli par L3-T5.

namespace actcraft::input {

Decision FullTouchPolicy::decide(const Sample&, const RouterState&) const
{
    return {};
}

// Talon L0-S : rempli par L0-S7.

PointerRouter::PointerRouter() : m_policy(std::make_unique<FullTouchPolicy>())
{
    static_cast<void>(m_state);
}

void PointerRouter::setPolicy(std::unique_ptr<RoutingPolicy> policy)
{
    m_policy = std::move(policy);
}

Decision PointerRouter::route(const Sample&)
{
    return {};
}

int PointerRouter::gestureTouches() const noexcept
{
    return 0;
}

void PointerRouter::reset() {}

void PointerRouter::setCoverTest(std::function<bool(const Sample&)> test)
{
    m_coverTest = std::move(test);
}

void PointerRouter::release(std::uint64_t) {}

} // namespace actcraft::input
