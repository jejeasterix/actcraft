// Talon L11 : rempli par L11-T6.
#include <actcraft/ui/touchmode.h>

namespace actcraft::ui {

TouchModeController::TouchModeController(QObject* parent) : QObject(parent) {}

Commands* TouchModeController::commands() const noexcept
{
    return m_commands;
}

void TouchModeController::setCommands(Commands* commands)
{
    if (m_commands == commands) {
        return;
    }
    m_commands = commands;
    emit commandsChanged();
}

Preferences* TouchModeController::preferences() const noexcept
{
    return m_preferences;
}

void TouchModeController::setPreferences(Preferences* preferences)
{
    if (m_preferences == preferences) {
        return;
    }
    m_preferences = preferences;
    emit preferencesChanged();
}

PageView* TouchModeController::view() const noexcept
{
    return m_view;
}

void TouchModeController::setView(PageView* view)
{
    if (m_view == view) {
        return;
    }
    m_view = view;
    emit viewChanged();
}

} // namespace actcraft::ui
