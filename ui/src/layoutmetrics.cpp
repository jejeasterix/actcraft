#include <actcraft/ui/layoutmetrics.h>

namespace actcraft::ui {

LayoutMetrics::LayoutMetrics(QObject* parent) : QObject(parent) {}

bool LayoutMetrics::narrow() const noexcept
{
    return m_windowWidth > 0 && m_windowWidth < kNarrowBelow;
}

bool LayoutMetrics::compact() const noexcept
{
    return m_windowWidth > 0 && m_windowWidth < kCompactBelow;
}

bool LayoutMetrics::portrait() const noexcept
{
    return m_windowHeight > m_windowWidth;
}

bool LayoutMetrics::touchUi() const
{
#ifdef Q_OS_ANDROID
    return true;
#else
    return m_preferences && m_preferences->value(QString::fromLatin1(kTouchUiKey), false).toBool();
#endif
}

qreal LayoutMetrics::targetSize() const
{
    return touchUi() ? kTouchTarget : kPointerTarget;
}

qreal LayoutMetrics::spacing() const
{
    return touchUi() ? kTouchSpacing : kPointerSpacing;
}

void LayoutMetrics::stackItemBefore(QQuickItem* item, QQuickItem* sibling) const
{
    if (item && sibling) {
        item->stackBefore(sibling);
    }
}

void LayoutMetrics::stackItemAfter(QQuickItem* item, QQuickItem* sibling) const
{
    if (item && sibling) {
        item->stackAfter(sibling);
    }
}

Preferences* LayoutMetrics::preferences() const noexcept
{
    return m_preferences;
}

void LayoutMetrics::setPreferences(Preferences* preferences)
{
    if (m_preferences == preferences) {
        return;
    }
    if (m_preferences) {
        disconnect(m_preferences, &Preferences::changed, this, &LayoutMetrics::metricsChanged);
    }
    m_preferences = preferences;
    if (m_preferences) {
        connect(m_preferences, &Preferences::changed, this, &LayoutMetrics::metricsChanged);
    }
    emit preferencesChanged();
    emit metricsChanged();
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
    emit metricsChanged();
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
    emit metricsChanged();
}

} // namespace actcraft::ui
