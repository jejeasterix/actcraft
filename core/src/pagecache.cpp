#include <actcraft/core/pagecache.h>

namespace actcraft::core {

namespace {
// Contrat de Change (undo.h) : zone vide = toute la page.
CacheArea changedArea(const RectF& area) noexcept
{
    if (area.width > 0 && area.height > 0) {
        return area;
    }
    return std::nullopt;
}
} // namespace

std::optional<std::size_t> applyToCache(const Changes& changes, std::size_t shownPage, PageCache& cache)
{
    std::size_t shown = shownPage;
    for (const Change& c : changes) {
        if (c.kind == Change::Kind::PageInserted) {
            if (c.page <= shown) {
                ++shown; // page insérée avant la page affichée (ou à sa place) : elle recule d'un rang
            }
            continue;
        }
        if (c.kind == Change::Kind::PageRemoved) {
            if (c.page == shown) {
                return std::nullopt;
            }
            if (c.page < shown) {
                --shown;
            }
            continue;
        }
        if (c.page != shown) {
            continue;
        }
        switch (c.kind) {
        case Change::Kind::ObjectInserted:
            cache.insert(c.object);
            break;
        case Change::Kind::ObjectRemoved:
            cache.remove(c.object, c.area);
            break;
        case Change::Kind::ObjectChanged:
            cache.invalidate(c.object, changedArea(c.area));
            break;
        case Change::Kind::PageChanged:
            cache.invalidate(0, std::nullopt);
            break;
        case Change::Kind::PageSettingsChanged: // présentation de la page : rien à redessiner (I3)
        case Change::Kind::PageInserted:
        case Change::Kind::PageRemoved:
            break;
        }
    }
    return shown;
}

} // namespace actcraft::core
