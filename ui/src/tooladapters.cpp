#include "tooladapters.h"

#include "mathtoolscontroller.h"

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

void stackAbove(QQuickItem& leader, QQuickItem& follower)
{
    follower.setParentItem(leader.parentItem());
    follower.setParent(&leader);
    follower.stackAfter(&leader);
    QQuickItem* const from = &leader;
    QQuickItem* const to = &follower;
    const auto follow = [from, to] {
        to->setPosition(from->position());
        to->setSize(from->size());
    };
    follow();
    QObject::connect(from, &QQuickItem::xChanged, to, follow);
    QObject::connect(from, &QQuickItem::yChanged, to, follow);
    QObject::connect(from, &QQuickItem::widthChanged, to, follow);
    QObject::connect(from, &QQuickItem::heightChanged, to, follow);
}

DocumentEditorAdapter::DocumentEditorAdapter(DocumentController& controller, int owner)
    : m_controller(controller), m_alive(&controller), m_owner(owner)
{
    m_observer = controller.addChangeObserver(
        [this](const core::Changes&, DocumentController::ChangeOrigin) { ++m_revision; });
}

DocumentEditorAdapter::~DocumentEditorAdapter()
{
    if (m_alive && m_observer != 0) {
        m_alive->removeChangeObserver(m_observer);
    }
}

QByteArray DocumentEditorAdapter::resourceData(const std::string& key) const
{
    const core::Resource* resource = m_controller.document().resource(key);
    return resource ? resource->data : QByteArray();
}

std::uint64_t DocumentEditorAdapter::revision() const
{
    return m_revision;
}

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
    return m_controller.liftObjects(ids, m_owner);
}

// L5-T17 : le connecteur lié suit l'objet déposé, en une étape. Le suivi se décide dans le contrôleur
// (DocumentController::run, M2 de L5-C18), sur la page où les objets soulevés sont revenus.
void DocumentEditorAdapter::drop(std::unique_ptr<core::UndoCommand> command)
{
    m_controller.dropLifted(std::move(command), m_owner);
}

bool DocumentEditorAdapter::isLifting() const
{
    return m_controller.isLifting(m_owner);
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

QVariantMap selectionFrameMap(const tools::SelectionView& view, Commands* commands)
{
    QVariantList bar;
    for (const tools::BarSpot& spot : view.bar) {
        bar.push_back(QVariantMap{
            {QStringLiteral("command"), spot.command},
            {QStringLiteral("icon"), commands ? commands->iconUrl(spot.command) : QString()},
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
    return QVariantMap{{QStringLiteral("visible"), view.visible},
                       {QStringLiteral("corners"), pointList(view.corners)},
                       {QStringLiteral("handles"), pointList(view.handles)},
                       {QStringLiteral("smartPoints"), pointList(view.smartPoints)},
                       {QStringLiteral("shapePoints"), pointList(view.shapePoints)},
                       {QStringLiteral("bar"), bar},
                       {QStringLiteral("rubberBand"), band},
                       {QStringLiteral("handlesVisible"), view.visible && view.handlesVisible}};
}

void SelectionSurfaceAdapter::show(const tools::SelectionView& view)
{
    const QVariantMap frame = selectionFrameMap(view, m_commands);
    m_show(frame, frame.value(QStringLiteral("handlesVisible")).toBool());
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

std::optional<core::PointF> ViewInfoAdapter::rotationPivot() const
{
    // F-284 : l'origine XY montrée, posée sur la vue par MathToolsController (kRotationPivotProperty).
    const QVariant pivot = m_view.property(MathToolsController::kRotationPivotProperty);
    if (!pivot.isValid()) {
        return std::nullopt;
    }
    return core::PointF{pivot.toPointF().x(), pivot.toPointF().y()};
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

CommandSinkAdapter::CommandSinkAdapter(Commands* commands, input::UserId user)
    : m_commands(commands), m_user(user)
{}

bool CommandSinkAdapter::trigger(const QString& id, const QVariant& argument)
{
    return m_commands && m_commands->trigger(id, argument, int{m_user});
}

ToolAdapters::ToolAdapters(DocumentController& controller, PageView& pageView, render::InkItem& inkItem,
                           render::LiftLayerItem& layer, Commands* commandList, tools::UserState& user,
                           SelectionSurfaceAdapter::Show showSelection, ViewInfoAdapter::TrashArea trashArea,
                           ViewInfoAdapter::TrashHover trashHover)
    : controllerRef(&controller), viewRef(&pageView), inkRef(&inkItem), layerRef(&layer),
      commandsRef(commandList), document(controller, user.id()), ink(inkItem), strokes(controller, pageView),
      lift(layer), selection(commandList, std::move(showSelection)),
      view(pageView, commandList, std::move(trashArea), std::move(trashHover)),
      commands(commandList, user.id()),
      context{document, ink, strokes, lift, selection, view, commands, user, false, false}
{}

bool ToolAdapters::matches(const DocumentController* controller, const PageView* pageView,
                           const render::InkItem* inkItem, const render::LiftLayerItem* layer,
                           const Commands* commandList) const noexcept
{
    return controllerRef == controller && viewRef == pageView && inkRef == inkItem && layerRef == layer &&
           commandsRef == commandList;
}

} // namespace actcraft::ui
