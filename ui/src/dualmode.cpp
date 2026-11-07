// Talon L11 : rempli par L11-T9.
#include <actcraft/ui/dualmode.h>

namespace actcraft::ui {

DualMode::DualMode(QObject* parent) : QObject(parent) {}

Commands* DualMode::commands() const noexcept
{
    return m_commands;
}

void DualMode::setCommands(Commands* commands)
{
    if (m_commands == commands) {
        return;
    }
    m_commands = commands;
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
    m_view = view;
    emit viewChanged();
}

bool DualMode::active() const noexcept
{
    return m_active;
}

} // namespace actcraft::ui
