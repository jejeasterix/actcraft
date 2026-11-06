#include <actcraft/core/broadcast.h>

namespace actcraft::core {

std::optional<Restriction> restrictionFor(std::string_view) noexcept
{
    return std::nullopt;
}

bool forbids(const BroadcastRules&, Restriction) noexcept
{
    return false;
}

Document published(const Document& source, const BroadcastRules&)
{
    return source;
}

} // namespace actcraft::core
