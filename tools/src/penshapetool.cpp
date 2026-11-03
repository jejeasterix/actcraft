#include <actcraft/core/edits.h>
#include <actcraft/core/hittest.h>
#include <actcraft/core/shapegeometry.h>
#include <actcraft/tools/penshapetool.h>

#include <Qt>

#include <cmath>
#include <memory>
#include <utility>

namespace actcraft::tools {

namespace {

constexpr std::int64_t kDoubleClickNs = 400'000'000;

double zoomOf(const ToolContext& context)
{
    const double zoom = context.view.zoom();
    return zoom > 0 ? zoom : 1.0;
}

} // namespace

PenShapeTool::PenShapeTool(ToolContext& context) : m_context(context) {}

QString PenShapeTool::id() const
{
    return QStringLiteral("pen-shape");
}

bool PenShapeTool::isBusy() const noexcept
{
    return m_active;
}

void PenShapeTool::setShape(core::PenShape shape)
{
    if (shape == m_shape)
        return; // même tracé (choisir une couleur, une épaisseur) : la polyligne en cours reste
    reset();
    m_shape = shape;
}

void PenShapeTool::setPen(const core::Outline& pen, bool highlighter)
{
    m_pen = pen;
    m_highlighter = highlighter;
}

bool PenShapeTool::dragged(core::PointF point) const
{
    return std::hypot(point.x - m_press.x, point.y - m_press.y) >=
           core::kShapeMinDragScreen / zoomOf(m_context);
}

// Aperçu par la couche soulevée : aucun redessin du cache, aucune commande.
void PenShapeTool::showPreview(const std::vector<core::PointF>& points)
{
    std::vector<core::PageObject> shown;
    shown.push_back(core::penShapeStroke(points, m_pen, m_highlighter));
    m_context.lift.show(std::move(shown));
}

void PenShapeTool::commit(const std::vector<core::PointF>& points)
{
    m_context.document.execute(std::make_unique<core::AddObjectCommand>(
        m_context.document.pageIndex(), core::penShapeStroke(points, m_pen, m_highlighter)));
}

void PenShapeTool::reset()
{
    const bool wasActive = m_active;
    m_active = false;
    m_clicks.clear();
    if (wasActive) {
        m_context.lift.clear();
    }
}

void PenShapeTool::onSample(const input::Sample& sample)
{
    const core::PointF point{sample.x, sample.y};
    if (m_shape == core::PenShape::Polyline) {
        switch (sample.phase) {
        case input::Phase::Press: {
            const bool doubleClick = !m_clicks.empty() &&
                                     sample.timestampNs - m_lastClickNs <= kDoubleClickNs &&
                                     std::hypot(point.x - m_press.x, point.y - m_press.y) <=
                                         core::kHitToleranceScreen / zoomOf(m_context);
            if (doubleClick) {
                std::vector<core::PointF> points = m_clicks;
                const bool enough = points.size() >= 2;
                reset();
                if (enough) {
                    commit(points);
                }
                return;
            }
            m_active = true;
            m_press = point;
            m_lastClickNs = sample.timestampNs;
            m_clicks.push_back(point);
            break;
        }
        case input::Phase::Move:
            if (m_active) {
                std::vector<core::PointF> points = m_clicks;
                points.push_back(point);
                showPreview(points);
            }
            break;
        case input::Phase::Release:
            break;
        }
        return;
    }
    switch (sample.phase) {
    case input::Phase::Press:
        m_active = true;
        m_press = point;
        break;
    case input::Phase::Move:
        if (m_active && dragged(point)) {
            showPreview(core::penShapePoints(m_shape, m_press, point));
        }
        break;
    case input::Phase::Release: {
        if (!m_active) {
            break;
        }
        const bool moved = dragged(point);
        reset();
        if (moved) {
            commit(core::penShapePoints(m_shape, m_press, point));
        }
        break;
    }
    }
}

bool PenShapeTool::onKey(int key, int modifiers)
{
    static_cast<void>(modifiers);
    if (!m_active) {
        return false;
    }
    if ((key == Qt::Key_Return || key == Qt::Key_Enter) && m_shape == core::PenShape::Polyline) {
        std::vector<core::PointF> points = m_clicks;
        reset();
        if (points.size() >= 2) {
            commit(points);
        }
        return true;
    }
    if (key == Qt::Key_Escape) {
        reset();
        return true;
    }
    return false;
}

// Abandon sans étape : un geste de forme interrompu ne laisse rien.
void PenShapeTool::cancel()
{
    reset();
}

} // namespace actcraft::tools
