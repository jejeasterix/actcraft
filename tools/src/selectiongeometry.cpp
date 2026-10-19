#include <actcraft/core/edits.h>
#include <actcraft/core/hittest.h>
#include <actcraft/tools/selectiongeometry.h>

#include <algorithm>
#include <optional>

namespace actcraft::tools {

// Cadre (coins et visibilité) ; poignées et barre d'objet : L3-T9.
SelectionView layoutSelection(const core::Page& page, const std::vector<core::ObjectId>& ids, double,
                              const QStringList&)
{
    std::optional<core::RectF> united;
    for (const core::ObjectId id : ids) {
        const std::size_t index = core::indexOf(page, id);
        if (index == page.objects.size()) {
            continue;
        }
        const core::RectF bounds = core::boundsOnPage(page.objects[index]);
        united = united ? core::unitedRect(*united, bounds) : bounds;
    }
    SelectionView out;
    if (!united) {
        return out;
    }
    out.visible = true;
    out.corners = {{united->x, united->y},
                   {united->x + united->width, united->y},
                   {united->x + united->width, united->y + united->height},
                   {united->x, united->y + united->height}};
    return out;
}

SelectionHit hitSelection(const SelectionView& view, core::PointF point, double)
{
    SelectionHit hit;
    if (!view.visible || view.corners.size() != 4) {
        return hit;
    }
    const core::RectF frame =
        core::normalizedRect(view.corners[0], view.corners[2]); // cadre non pivoté (L3-T8)
    if (frame.contains(point)) {
        hit.kind = SelectionHitKind::Inside;
    }
    return hit;
}

} // namespace actcraft::tools
