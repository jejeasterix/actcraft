#include <actcraft/core/edits.h>
#include <actcraft/core/hittest.h>
#include <actcraft/tools/selectiongeometry.h>

#include <algorithm>
#include <cmath>
#include <optional>

namespace actcraft::tools {

namespace {

constexpr double kBarSpacingScreen = 4.0; // écart entre deux boutons de la barre (F-521)

core::PointF middle(core::PointF a, core::PointF b) noexcept
{
    return {(a.x + b.x) / 2, (a.y + b.y) / 2};
}

// Barre d'objet : au-dessus du cadre, sous lui si elle sortirait du haut de la page, à droite si le cadre est
// plus haut que large (colonne). Tailles en pixels d'écran, converties en pixels de page.
std::vector<BarSpot> layoutBar(const std::vector<core::PointF>& corners, const QStringList& commands,
                               double zoom)
{
    std::vector<BarSpot> bar;
    if (commands.isEmpty()) {
        return bar;
    }
    double minX = corners[0].x;
    double maxX = minX;
    double minY = corners[0].y;
    double maxY = minY;
    for (const core::PointF& corner : corners) {
        minX = std::min(minX, corner.x);
        maxX = std::max(maxX, corner.x);
        minY = std::min(minY, corner.y);
        maxY = std::max(maxY, corner.y);
    }
    const double button = kBarButtonScreen / zoom;
    const double spacing = kBarSpacingScreen / zoom;
    const double gap = kBarGapScreen / zoom;
    const double count = static_cast<double>(commands.size());
    const double total = count * button + (count - 1) * spacing;
    const double centerX = (minX + maxX) / 2;
    const double centerY = (minY + maxY) / 2;
    const bool column = (maxY - minY) > (maxX - minX);
    double fixed = 0; // y de la rangée, ou x de la colonne
    if (column) {
        fixed = maxX + gap + button / 2;
    } else {
        fixed = minY - gap - button / 2;
        if (fixed - button / 2 < 0) {
            fixed = maxY + gap + button / 2;
        }
    }
    const double first = (column ? centerY : centerX) - total / 2 + button / 2;
    for (qsizetype i = 0; i < commands.size(); ++i) {
        const double along = first + static_cast<double>(i) * (button + spacing);
        bar.push_back({commands[i], column ? core::PointF{fixed, along} : core::PointF{along, fixed}});
    }
    return bar;
}

// Point dans le quadrilatère convexe (bords compris) ; aire nulle : jamais.
bool insideQuad(const std::vector<core::PointF>& quad, core::PointF p) noexcept
{
    double area = 0;
    bool positive = false;
    bool negative = false;
    for (std::size_t i = 0; i < 4; ++i) {
        const core::PointF a = quad[i];
        const core::PointF b = quad[(i + 1) % 4];
        area += a.x * b.y - b.x * a.y;
        const double cross = (b.x - a.x) * (p.y - a.y) - (b.y - a.y) * (p.x - a.x);
        positive = positive || cross > 0;
        negative = negative || cross < 0;
    }
    return std::abs(area) > 1e-12 && !(positive && negative);
}

} // namespace

SelectionView layoutSelection(const core::Page& page, const std::vector<core::ObjectId>& ids, double zoom,
                              const QStringList& barCommands)
{
    if (!(zoom > 0)) {
        zoom = 1;
    }
    std::optional<core::RectF> united;
    const core::PageObject* single = nullptr;
    int count = 0;
    bool resizable = true;
    bool rotatable = true;
    for (const core::ObjectId id : ids) {
        const std::size_t index = core::indexOf(page, id);
        if (index == page.objects.size()) {
            continue;
        }
        const core::PageObject& object = page.objects[index];
        const core::RectF bounds = core::boundsOnPage(object);
        united = united ? core::unitedRect(*united, bounds) : bounds;
        single = &object;
        ++count;
        resizable = resizable && !object.common.locked && object.common.restrictions.resizable;
        rotatable = rotatable && !object.common.locked && object.common.restrictions.rotatable;
    }
    SelectionView out;
    if (!united) {
        return out;
    }
    out.visible = true;
    if (count == 1 && !single->common.transform.isIdentity()) {
        // Un seul objet : son cadre suit sa transformation (F-520). Emprise locale = emprise de la copie
        // sans transformation.
        core::PageObject plain = *single;
        plain.common.transform = core::Transform{};
        const core::RectF local = core::boundsOnPage(plain);
        const core::Transform& t = single->common.transform;
        out.corners = {t.map({local.x, local.y}), t.map({local.x + local.width, local.y}),
                       t.map({local.x + local.width, local.y + local.height}),
                       t.map({local.x, local.y + local.height})};
    } else {
        out.corners = {{united->x, united->y},
                       {united->x + united->width, united->y},
                       {united->x + united->width, united->y + united->height},
                       {united->x, united->y + united->height}};
    }
    if (resizable) {
        const std::vector<core::PointF>& c = out.corners;
        out.handles = {c[0], middle(c[0], c[1]), c[1], middle(c[1], c[2]),
                       c[2], middle(c[2], c[3]), c[3], middle(c[3], c[0])};
    }
    QStringList commands = barCommands;
    if (!rotatable) {
        commands.removeAll(QStringLiteral("object.rotate"));
    }
    out.bar = layoutBar(out.corners, commands, zoom);
    return out;
}

SelectionHit hitSelection(const SelectionView& view, core::PointF point, double zoom)
{
    SelectionHit hit;
    if (!view.visible || view.corners.size() != 4) {
        return hit;
    }
    if (!(zoom > 0)) {
        zoom = 1;
    }
    const double half = kBarButtonScreen / zoom / 2;
    for (const BarSpot& spot : view.bar) {
        if (std::abs(point.x - spot.center.x) <= half && std::abs(point.y - spot.center.y) <= half) {
            hit.kind = SelectionHitKind::BarButton;
            hit.command = spot.command;
            return hit;
        }
    }
    if (view.handlesVisible && view.handles.size() == 8) {
        const double radius = kHandleRadiusScreen / zoom;
        for (std::size_t i = 0; i < view.handles.size(); ++i) {
            if (std::hypot(point.x - view.handles[i].x, point.y - view.handles[i].y) <= radius) {
                hit.kind = SelectionHitKind::Handle;
                hit.handle = static_cast<HandleKind>(i);
                return hit;
            }
        }
    }
    if (insideQuad(view.corners, point)) {
        hit.kind = SelectionHitKind::Inside;
    }
    return hit;
}

} // namespace actcraft::tools
