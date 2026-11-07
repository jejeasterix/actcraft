// Talon L11 : rempli par L11-T21.
#include <actcraft/ui/updatechecker.h>

namespace actcraft::ui {

UpdateChecker::UpdateChecker(QObject* parent) : QObject(parent) {}

Preferences* UpdateChecker::preferences() const noexcept
{
    return m_preferences;
}

void UpdateChecker::setPreferences(Preferences* preferences)
{
    if (m_preferences == preferences) {
        return;
    }
    m_preferences = preferences;
    emit preferencesChanged();
}

} // namespace actcraft::ui
