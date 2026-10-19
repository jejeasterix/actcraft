#include "tooladapters.h"

#include <actcraft/ui/commands.h>
#include <actcraft/ui/documentcontroller.h>
#include <actcraft/ui/pageview.h>

#include <QVariantList>

#include <utility>

namespace actcraft::ui {

namespace {

QVariantList pointList(const std::vector<core::PointF>& points)
{
    QVariantList out;
    out.reserve(static_cast<qsizetype>(points.size()));
    for (const core::PointF& point : points) {
        out.push_back(QVariantMap{{QStringLiteral("x"), point.x}, {QStringLiteral("y"), point.y}});
    }
    return out;
}

} // namespace

DocumentEditorAdapter::DocumentEditorAdapter(DocumentController& controller) : m_controller(controller) {}

const core::Page& DocumentEditorAdapter::page() const
{
    return m_controller.document().page(pageIndex());
}

std::size_t DocumentEditorAdapter::pageIndex() const
{
    return static_cast<std::size_t>(m_controller.currentIndex());
}

void DocumentEditorAdapter::execute(std::unique_ptr<core::UndoCommand> command)
{
    m_controller.execute(std::move(command));
}

std::vector<core::PageObject> DocumentEditorAdapter::lift(const std::vector<core::ObjectId>& ids)
{
    return m_controller.liftObjects(ids);
}

void DocumentEditorAdapter::drop(std::unique_ptr<core::UndoCommand> command)
{
    m_controller.dropLifted(std::move(command));
}

bool DocumentEditorAdapter::isLifting() const
{
    return m_controller.isLifting();
}

LiftSurfaceAdapter::LiftSurfaceAdapter(render::LiftLayerItem& layer) : m_layer(layer) {}

void LiftSurfaceAdapter::show(std::vector<core::PageObject> objects)
{
    m_layer.setObjects(std::move(objects));
}

void LiftSurfaceAdapter::setTransform(const core::Transform& extra)
{
    m_layer.setExtraTransform(extra);
}

void LiftSurfaceAdapter::clear()
{
    m_layer.clear();
}

SelectionSurfaceAdapter::SelectionSurfaceAdapter(Commands* commands, Show show)
    : m_commands(commands), m_show(std::move(show))
{}

void SelectionSurfaceAdapter::show(const tools::SelectionView& view)
{
    QVariantList bar;
    for (const tools::BarSpot& spot : view.bar) {
        bar.push_back(QVariantMap{
            {QStringLiteral("command"), spot.command},
            {QStringLiteral("icon"), m_commands ? m_commands->iconUrl(spot.command) : QString()},
            {QStringLiteral("x"), spot.center.x},
            {QStringLiteral("y"), spot.center.y},
        });
    }
    QVariant band;
    if (view.rubberBand) {
        band = QVariantMap{{QStringLiteral("x"), view.rubberBand->x},
                           {QStringLiteral("y"), view.rubberBand->y},
                           {QStringLiteral("width"), view.rubberBand->width},
                           {QStringLiteral("height"), view.rubberBand->height}};
    }
    const bool handles = view.visible && view.handlesVisible;
    m_show(QVariantMap{{QStringLiteral("visible"), view.visible},
                       {QStringLiteral("corners"), pointList(view.corners)},
                       {QStringLiteral("handles"), pointList(view.handles)},
                       {QStringLiteral("bar"), bar},
                       {QStringLiteral("rubberBand"), band},
                       {QStringLiteral("handlesVisible"), handles}},
           handles);
}

ViewInfoAdapter::ViewInfoAdapter(PageView& view, Commands* commands, TrashArea area, TrashHover hover)
    : m_view(view), m_commands(commands), m_area(std::move(area)), m_hover(std::move(hover))
{}

double ViewInfoAdapter::zoom() const
{
    return m_view.zoom();
}

bool ViewInfoAdapter::overTrash(core::PointF pagePoint) const
{
    const QRectF area = m_area();
    return !area.isEmpty() && area.contains(m_view.pageToScene(QPointF(pagePoint.x, pagePoint.y)));
}

void ViewInfoAdapter::setTrashHover(bool hover)
{
    m_hover(hover);
}

QStringList ViewInfoAdapter::objectBarCommands() const
{
    QStringList ids;
    if (m_commands) {
        const QVariantList bar = m_commands->objectBar();
        for (const QVariant& entry : bar) {
            ids.push_back(entry.toMap().value(QStringLiteral("id")).toString());
        }
    }
    return ids;
}

CommandSinkAdapter::CommandSinkAdapter(Commands* commands) : m_commands(commands) {}

bool CommandSinkAdapter::trigger(const QString& id, const QVariant& argument)
{
    return m_commands && m_commands->trigger(id, argument);
}

ToolAdapters::ToolAdapters(DocumentController& controller, PageView& pageView, render::InkItem& inkItem,
                           render::LiftLayerItem& layer, Commands* commandList, tools::UserState& user,
                           SelectionSurfaceAdapter::Show showSelection, ViewInfoAdapter::TrashArea trashArea,
                           ViewInfoAdapter::TrashHover trashHover)
    : controllerRef(&controller), viewRef(&pageView), inkRef(&inkItem), layerRef(&layer),
      commandsRef(commandList), document(controller), ink(inkItem), strokes(controller, pageView),
      lift(layer), selection(commandList, std::move(showSelection)),
      view(pageView, commandList, std::move(trashArea), std::move(trashHover)), commands(commandList),
      context{document, ink, strokes, lift, selection, view, commands, user}
{}

bool ToolAdapters::matches(const DocumentController* controller, const PageView* pageView,
                           const render::InkItem* inkItem, const render::LiftLayerItem* layer,
                           const Commands* commandList) const noexcept
{
    return controllerRef == controller && viewRef == pageView && inkRef == inkItem && layerRef == layer &&
           commandsRef == commandList;
}

} // namespace actcraft::ui
