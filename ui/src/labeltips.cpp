#include <actcraft/ui/labeltips.h>

namespace actcraft::ui {

LabelTips::LabelTips(QObject* parent) : QObject(parent) {}

DocumentController* LabelTips::document() const noexcept
{
    return m_document.data();
}

void LabelTips::setDocument(DocumentController* document)
{
    if (document == m_document) {
        return;
    }
    m_document = document;
    emit documentChanged();
}

QString LabelTips::text() const
{
    return m_text;
}

bool LabelTips::isVisible() const noexcept
{
    return m_visible;
}

QString LabelTips::fontFamily() const
{
    return m_fontFamily;
}

qreal LabelTips::pointSize() const noexcept
{
    return m_pointSize;
}

QColor LabelTips::color() const
{
    return m_color;
}

QColor LabelTips::outlineColor() const
{
    return m_outlineColor;
}

QColor LabelTips::fill() const
{
    return m_fill;
}

qreal LabelTips::tolerance() const noexcept
{
    return m_tolerance;
}

void LabelTips::setTolerance(qreal tolerance)
{
    if (tolerance == m_tolerance) {
        return;
    }
    m_tolerance = tolerance;
    emit toleranceChanged();
}

void LabelTips::hoverAt(QPointF) {}

void LabelTips::leave() {}

void LabelTips::clear() {}

} // namespace actcraft::ui
