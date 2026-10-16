#include <actcraft/core/pagecache.h>

namespace actcraft::core {

void applyToCache(const Changes& changes, std::size_t shownPage, PageCache& cache)
{
    for (const Change& c : changes) {
        if (c.page != shownPage) {
            continue;
        }
        switch (c.kind) {
        case Change::Kind::ObjectInserted:
            cache.insert(c.object);
            break;
        case Change::Kind::ObjectRemoved:
            cache.remove(c.object);
            break;
        case Change::Kind::ObjectChanged:
            cache.invalidate(c.area);
            break;
        case Change::Kind::PageChanged:
            cache.invalidate(RectF{});
            break;
        case Change::Kind::PageInserted:
        case Change::Kind::PageRemoved:
            break;
        }
    }
}

} // namespace actcraft::core
