#include <actcraft/core/lift.h>

// Talon L0-S : rempli par L3-T4.

namespace actcraft::core {

std::vector<LiftedObject> takeObjects(Page&, const std::vector<ObjectId>&)
{
    return {};
}

void restoreObjects(Page&, const std::vector<LiftedObject>&) {}

Changes liftChanges(std::size_t, const std::vector<LiftedObject>&)
{
    return {};
}

Changes changesAfterLift(std::size_t, const std::vector<LiftedObject>&, const Changes&, const Page&)
{
    return {};
}

} // namespace actcraft::core
