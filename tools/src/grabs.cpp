#include <actcraft/tools/grabs.h>

namespace actcraft::tools {

bool Grabs::hold(core::ObjectId object, input::UserId user)
{
    const auto [entry, inserted] = m_held.try_emplace(object, user);
    return inserted || entry->second == user;
}

void Grabs::release(input::UserId user)
{
    std::erase_if(m_held, [user](const auto& entry) { return entry.second == user; });
}

std::optional<input::UserId> Grabs::holder(core::ObjectId object) const
{
    const auto found = m_held.find(object);
    if (found == m_held.end()) {
        return std::nullopt;
    }
    return found->second;
}

} // namespace actcraft::tools
