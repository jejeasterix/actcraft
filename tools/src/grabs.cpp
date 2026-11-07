// Talon L11 : rempli par L11-T8.
#include <actcraft/tools/grabs.h>

namespace actcraft::tools {

// Talon L11 : rempli par L11-T8.
bool Grabs::hold(core::ObjectId, input::UserId)
{
    return false;
}

// Talon L11 : rempli par L11-T8.
void Grabs::release(input::UserId) {}

// Talon L11 : rempli par L11-T8.
std::optional<input::UserId> Grabs::holder(core::ObjectId) const
{
    return std::nullopt;
}

} // namespace actcraft::tools
