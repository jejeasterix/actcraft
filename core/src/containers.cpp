#include <actcraft/core/containers.h>

namespace actcraft::core {

bool isInFront(const Page&, ObjectId, ObjectId)
{
    return false;
}

bool accepts(const PageObject&, const PageObject&)
{
    return false;
}

bool fits(const PageObject&, const PageObject&)
{
    return false;
}

DropResult resolveDrop(const Page&, const std::vector<ObjectId>&)
{
    return {};
}

std::vector<ObjectId> containedIn(const Page&, ObjectId)
{
    return {};
}

} // namespace actcraft::core
