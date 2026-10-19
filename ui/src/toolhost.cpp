#include <actcraft/ui/toolhost.h>

#include <memory>

// Réel au socle (L0-S6, D-S10) : user(). Le reste : Talon L0-S : rempli par L3-T14.
namespace actcraft::ui {

ToolHost::ToolHost(QObject* parent) : QObject(parent) {}

ToolHost::~ToolHost() = default;

Commands* ToolHost::commands() const noexcept
{
    return m_commands.data();
}

void ToolHost::setCommands(Commands* commands)
{
    m_commands = commands;
}

DocumentController* ToolHost::documentController() const noexcept
{
    return m_controller.data();
}

void ToolHost::setDocumentController(DocumentController* controller)
{
    m_controller = controller;
}

PageView* ToolHost::view() const noexcept
{
    return m_view.data();
}

void ToolHost::setView(PageView* view)
{
    m_view = view;
}

render::LiftLayerItem* ToolHost::liftLayer() const noexcept
{
    return m_liftLayer.data();
}

void ToolHost::setLiftLayer(render::LiftLayerItem* layer)
{
    m_liftLayer = layer;
}

QString ToolHost::currentTool() const
{
    return {};
}

QColor ToolHost::color() const
{
    return {};
}

qreal ToolHost::width() const
{
    return 0;
}

int ToolHost::selectionCount() const
{
    return 0;
}

bool ToolHost::handlesVisible() const
{
    return false;
}

QVariantMap ToolHost::selectionFrame() const
{
    return {};
}

QRectF ToolHost::trashArea() const noexcept
{
    return m_trashArea;
}

void ToolHost::setTrashArea(const QRectF& area)
{
    m_trashArea = area;
}

bool ToolHost::trashHover() const noexcept
{
    return m_trashHover;
}

QString ToolHost::settingsFile() const
{
    return m_settingsFile;
}

void ToolHost::setSettingsFile(const QString& path)
{
    m_settingsFile = path;
}

void ToolHost::activate(const QString&) {}

void ToolHost::setColor(const QColor&) {}

void ToolHost::setWidth(qreal) {}

void ToolHost::selectAll() {}

void ToolHost::clearSelection() {}

void ToolHost::deleteSelection() {}

void ToolHost::enlargeSelection() {}

void ToolHost::reduceSelection() {}

void ToolHost::setSelectionTranslucency(int) {}

void ToolHost::cancelGesture() {}

tools::Tool* ToolHost::activeTool(input::UserId) const
{
    return nullptr;
}

tools::UserState& ToolHost::user(input::UserId)
{
    if (!m_user) {
        m_user = std::make_unique<tools::UserState>(input::kFirstUser);
    }
    return *m_user;
}

} // namespace actcraft::ui
