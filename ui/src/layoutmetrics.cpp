// Talon L11 : rempli par L11-T10.
#include <actcraft/ui/layoutmetrics.h>

namespace actcraft::ui {

LayoutMetrics::LayoutMetrics(QObject* parent) : QObject(parent) {}

Preferences* LayoutMetrics::preferences() const noexcept
{
    return m_preferences;
}

void LayoutMetrics::setPreferences(Preferences* preferences)
{
    if (m_preferences == preferences) {
        return;
    }
    m_preferences = preferences;
    emit preferencesChanged();
}

qreal LayoutMetrics::windowWidth() const noexcept
{
    return m_windowWidth;
}

void LayoutMetrics::setWindowWidth(qreal windowWidth)
{
    if (m_windowWidth == windowWidth) {
        return;
    }
    m_windowWidth = windowWidth;
    emit windowWidthChanged();
}

qreal LayoutMetrics::windowHeight() const noexcept
{
    return m_windowHeight;
}

void LayoutMetrics::setWindowHeight(qreal windowHeight)
{
    if (m_windowHeight == windowHeight) {
        return;
    }
    m_windowHeight = windowHeight;
    emit windowHeightChanged();
}

} // namespace actcraft::ui
