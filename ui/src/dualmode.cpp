#include <actcraft/input/touchmodes.h>
#include <actcraft/input/twouserpolicy.h>
#include <actcraft/ui/dualmode.h>

#include <memory>
#include <string>

namespace actcraft::ui {

namespace {

const QString kToggle = QStringLiteral("users.dual");

} // namespace

DualMode::DualMode(QObject* parent) : QObject(parent) {}

DualMode::~DualMode()
{
    if (m_commands) {
        m_commands->setNativeHandler(kToggle, {}); // le gestionnaire tient `this`
    }
}

Commands* DualMode::commands() const noexcept
{
    return m_commands;
}

void DualMode::setCommands(Commands* commands)
{
    if (m_commands == commands) {
        return;
    }
    if (m_commands) {
        m_commands->setNativeHandler(kToggle, {});
    }
    m_commands = commands;
    if (m_commands) {
        m_commands->setNativeHandler(kToggle, [this](const QVariant&) { toggle(); });
        m_commands->setChecked(kToggle, m_active);
    }
    emit commandsChanged();
}

PageView* DualMode::view() const noexcept
{
    return m_view;
}

void DualMode::setView(PageView* view)
{
    if (m_view == view) {
        return;
    }
    if (m_view) {
        disconnect(m_view, nullptr, this, nullptr);
    }
    m_view = view;
    if (m_view) {
        connect(m_view, &PageView::contactActiveChanged, this, &DualMode::applyPending);
        connect(m_view, &QQuickItem::widthChanged, this, &DualMode::updateArea);
        connect(m_view, &QQuickItem::heightChanged, this, &DualMode::updateArea);
        if (m_active) {
            poseTwoUserPolicy();
        }
    }
    emit viewChanged();
}

ToolHost* DualMode::tools() const noexcept
{
    return m_tools;
}

void DualMode::setTools(ToolHost* tools)
{
    if (m_tools == tools) {
        return;
    }
    m_tools = tools;
    if (m_tools) {
        m_tools->setDualUsers(m_active);
    }
    emit toolsChanged();
}

TouchModeController* DualMode::touchMode() const noexcept
{
    return m_touchMode;
}

void DualMode::setTouchMode(TouchModeController* touchMode)
{
    if (m_touchMode == touchMode) {
        return;
    }
    if (m_touchMode) {
        disconnect(m_touchMode, nullptr, this, nullptr);
    }
    m_touchMode = touchMode;
    if (m_touchMode) {
        // Le contrôleur pose sa politique puis émet modeChanged : allumé, la politique à deux est reposée.
        connect(m_touchMode, &TouchModeController::modeChanged, this, [this] {
            if (m_active) {
                poseTwoUserPolicy();
            }
        });
    }
    emit touchModeChanged();
}

bool DualMode::active() const noexcept
{
    return m_active;
}

void DualMode::toggle()
{
    m_wanted = !m_wanted;
    applyPending();
}

void DualMode::applyPending()
{
    if (m_wanted == m_active || (m_view && m_view->contactActive())) {
        return; // différée : contactActiveChanged la rejoue
    }
    m_active = m_wanted;
    if (m_tools) {
        m_tools->setDualUsers(m_active);
    }
    if (m_active) {
        poseTwoUserPolicy();
    } else {
        restoreTouchPolicy();
    }
    if (m_commands) {
        m_commands->setChecked(kToggle, m_active);
    }
    emit activeChanged();
}

void DualMode::poseTwoUserPolicy()
{
    if (!m_view) {
        return;
    }
    m_view->setSecondUserViewArea(secondArea());
    m_view->setRoutingPolicy(std::make_unique<input::TwoUserPolicy>(QRectF())); // aire reçue de la vue
}

void DualMode::restoreTouchPolicy()
{
    if (!m_view) {
        return;
    }
    input::TouchMode mode = input::TouchMode::Full;
    if (m_touchMode) {
        const std::string id = m_touchMode->mode().toStdString();
        if (!input::touchModeFromId(id, mode)) {
            mode = input::TouchMode::Full;
        }
    }
    m_view->setRoutingPolicy(input::makeTouchPolicy(mode));
}

void DualMode::updateArea()
{
    if (m_active && m_view) {
        m_view->setSecondUserViewArea(secondArea());
    }
}

QRectF DualMode::secondArea() const
{
    return QRectF(0, 0, m_view->width() / 2, m_view->height());
}

} // namespace actcraft::ui
