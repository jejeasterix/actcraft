#include <actcraft/core/edits.h>
#include <actcraft/core/gridsnap.h>
#include <actcraft/core/shapegeometry.h>
#include <actcraft/tools/shapetool.h>
#include <actcraft/tools/userstate.h>

#include <cmath>
#include <memory>
#include <utility>

namespace actcraft::tools {

ShapeTool::ShapeTool(ToolContext& context) : m_context(context) {}

QString ShapeTool::id() const
{
    return QStringLiteral("shape");
}

// Les formes sans boîte (ligne et courbe brisées, main levée) ont le geste de L4-T31 : rien ici.
bool ShapeTool::boxGesture() const
{
    const std::string preset = m_context.user.shapeSettings().preset.toStdString();
    return core::shapeGesture(preset) == core::ShapeGesture::Box;
}

// Écart en pixels d'écran (donc brut, avant accrochage) : en deçà du seuil, le tracé est un clic (F-241).
bool ShapeTool::dragged() const
{
    if (!m_press) {
        return false;
    }
    const double zoom = m_context.view.zoom();
    const double distance = std::hypot(m_current.x - m_press->x, m_current.y - m_press->y);
    return distance >= core::kShapeMinDragScreen / (zoom > 0 ? zoom : 1.0);
}

core::PointF ShapeTool::snapped(core::PointF point) const
{
    const core::Page& page = m_context.document.page();
    return core::nearestGridPoint(page.grid, point, page.width, page.height).value_or(point);
}

std::optional<core::PageObject> ShapeTool::currentObject() const
{
    if (!m_press) {
        return std::nullopt;
    }
    const ShapeSettings settings = m_context.user.shapeSettings();
    const InkSettings outline = m_context.user.settings(QStringLiteral("shape"));
    core::ShapeStyle style;
    style.outline = outline.color;
    style.width = outline.width;
    style.filled = settings.filled;
    style.fill = settings.fill;

    const std::string id = settings.preset.toStdString();
    const core::ShapePreset* preset = core::findShapePreset(id);
    const core::PointF from = snapped(*m_press);
    const core::PointF to = snapped(m_current);
    if (preset && preset->family == core::ShapeFamily::Line) {
        const auto [a, b] = core::dragLine(from, to, *preset, settings.fromCenter, false);
        return core::makeLineObject(id, a, b, style);
    }
    const bool square = preset && preset->square;
    return core::makeShapeObject(id, core::dragBox(from, to, settings.fromCenter, square), style);
}

void ShapeTool::showPreview()
{
    if (!dragged()) {
        m_context.lift.clear();
        return;
    }
    if (std::optional<core::PageObject> object = currentObject()) {
        std::vector<core::PageObject> shown;
        shown.push_back(std::move(*object));
        m_context.lift.show(std::move(shown));
    }
}

void ShapeTool::onSample(const input::Sample& sample)
{
    const core::PointF point{sample.x, sample.y};
    switch (sample.phase) {
    case input::Phase::Press:
        if (!boxGesture()) {
            return;
        }
        m_press = point;
        m_current = point;
        return;
    case input::Phase::Move:
        if (!m_press) {
            return;
        }
        m_current = point;
        showPreview();
        return;
    case input::Phase::Release: {
        if (!m_press) {
            return;
        }
        m_current = point;
        const bool place = dragged();
        std::optional<core::PageObject> object;
        if (place) {
            object = currentObject();
        }
        m_press.reset();
        m_context.lift.clear();
        if (object) {
            m_context.document.execute(
                std::make_unique<core::AddObjectCommand>(m_context.document.pageIndex(), std::move(*object)));
        }
        return;
    }
    }
}

bool ShapeTool::isBusy() const noexcept
{
    return m_press.has_value();
}

void ShapeTool::cancel()
{
    m_press.reset();
    m_context.lift.clear();
}

bool ShapeTool::onKey(int, int)
{
    return false;
}

} // namespace actcraft::tools
