#include <actcraft/core/connectors.h>
#include <actcraft/core/edits.h>
#include <actcraft/core/hittest.h>
#include <actcraft/core/shapegeometry.h>
#include <actcraft/tools/connectortool.h>

#include <cmath>
#include <memory>
#include <utility>
#include <vector>

namespace actcraft::tools {

namespace {

constexpr double kHoverTolerance = 2.0; // pixels de page, en plus de l'accrochage du cœur

double zoomOf(const ToolContext& context)
{
    const double zoom = context.view.zoom();
    return zoom > 0 ? zoom : 1.0;
}

} // namespace

ConnectorTool::ConnectorTool(ToolContext& context) : m_context(context) {}

QString ConnectorTool::id() const
{
    return QStringLiteral("connector");
}

bool ConnectorTool::isBusy() const noexcept
{
    return m_active;
}

void ConnectorTool::setStyle(std::uint8_t style)
{
    m_style = style;
}

// Points d'attache de l'objet sous le pointeur (ou de celui dont un point est à portée d'accrochage).
void ConnectorTool::showAttachPoints(core::PointF point)
{
    const core::Page& page = m_context.document.page();
    std::optional<core::ObjectId> id = core::objectAt(page, point, kHoverTolerance / zoomOf(m_context));
    if (!id) {
        if (const std::optional<core::Attachment> near = core::snapConnectorEnd(page, point)) {
            id = near->object;
        }
    }
    std::vector<core::PointF> points;
    if (id) {
        const std::size_t i = core::indexOf(page, *id);
        if (i < page.objects.size() && page.objects[i].common.id == *id &&
            !std::get_if<core::Connector>(&page.objects[i])) {
            points = core::attachPoints(page.objects[i]);
        }
    }
    if (points.empty() && !m_shownPoints) {
        return;
    }
    SelectionView view;
    view.visible = false;
    view.smartPoints = std::move(points);
    m_shownPoints = !view.smartPoints.empty();
    m_context.selection.show(view);
}

void ConnectorTool::clearAttachPoints()
{
    if (m_shownPoints) {
        m_shownPoints = false;
        m_context.selection.show(SelectionView{});
    }
}

bool ConnectorTool::dragged(core::PointF point) const
{
    if (!m_active) {
        return false;
    }
    return std::hypot(point.x - m_rawPress.x, point.y - m_rawPress.y) >=
           core::kShapeMinDragScreen / zoomOf(m_context);
}

// Connecteur de la presse jusqu'à `point` : chaque bout s'accroche par la règle du cœur.
core::PageObject ConnectorTool::currentConnector(core::PointF point) const
{
    core::PointF end = point;
    core::ObjectId objectB = 0;
    if (const std::optional<core::Attachment> snap =
            core::snapConnectorEnd(m_context.document.page(), point)) {
        end = snap->point;
        objectB = snap->object;
    }
    return core::makeConnector(m_start, m_objectA, end, objectB, m_style);
}

void ConnectorTool::onSample(const input::Sample& sample)
{
    const core::PointF point{sample.x, sample.y};
    switch (sample.phase) {
    case input::Phase::Press: {
        clearAttachPoints();
        m_active = true;
        m_rawPress = point;
        m_start = point;
        m_objectA = 0;
        if (const std::optional<core::Attachment> snap =
                core::snapConnectorEnd(m_context.document.page(), point)) {
            m_start = snap->point;
            m_objectA = snap->object;
        }
        break;
    }
    case input::Phase::Move:
        if (!m_active) {
            showAttachPoints(point);
            break;
        }
        if (dragged(point)) {
            std::vector<core::PageObject> shown;
            shown.push_back(currentConnector(point));
            m_context.lift.show(std::move(shown));
            showAttachPoints(point);
        }
        break;
    case input::Phase::Release: {
        if (!m_active) {
            break;
        }
        const bool moved = dragged(point);
        core::PageObject connector = currentConnector(point);
        m_active = false;
        m_objectA = 0;
        m_context.lift.clear();
        clearAttachPoints();
        if (moved) {
            m_context.document.execute(std::make_unique<core::AddObjectCommand>(
                m_context.document.pageIndex(), std::move(connector)));
        }
        break;
    }
    }
}

void ConnectorTool::cancel()
{
    m_active = false;
    m_objectA = 0;
    m_context.lift.clear();
    clearAttachPoints();
}

} // namespace actcraft::tools
