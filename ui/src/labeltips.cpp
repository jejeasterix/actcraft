#include <actcraft/core/hittest.h>
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
    if (m_document) {
        disconnect(m_document, nullptr, this, nullptr);
    }
    m_document = document;
    if (m_document) {
        // Le contenu change (annulation, suppression, autre page) : la bulle n'est plus celle du survol.
        connect(m_document, &DocumentController::pagesChanged, this, &LabelTips::refresh);
        connect(m_document, &DocumentController::documentChanged, this, &LabelTips::refresh);
        connect(m_document, &DocumentController::pageContentChanged, this, &LabelTips::refresh);
    }
    clear();
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

// Le contenu a changé : la bulle reste si le même point désigne encore une légende, sinon elle disparaît.
void LabelTips::refresh()
{
    if (m_visible) {
        hoverAt(m_point);
    }
}

void LabelTips::hoverAt(QPointF pagePoint)
{
    m_point = pagePoint;
    if (!m_document || m_document->document().pageCount() == 0) {
        clear();
        return;
    }
    const core::Page& page =
        m_document->document().page(static_cast<std::size_t>(m_document->currentIndex()));
    const std::optional<core::ObjectId> id =
        core::objectAt(page, core::PointF{pagePoint.x(), pagePoint.y()}, m_tolerance);
    const core::ObjectCommon* common = nullptr;
    if (id) {
        for (const core::PageObject& object : page.objects) {
            if (object.common.id == *id) {
                common = &object.common;
                break;
            }
        }
    }
    if (!common || !common->visible || common->label.empty()) {
        clear();
        return;
    }
    const core::LabelStyle& style = common->labelStyle;
    m_text = QString::fromStdString(common->label);
    m_fontFamily = QString::fromStdString(style.fontFamily);
    m_pointSize = style.pointSize;
    m_color = QColor::fromRgba(style.color);
    m_outlineColor = style.outline == core::PenStyle::None ? QColor(Qt::transparent)
                                                           : QColor::fromRgba(style.outlineColor);
    m_fill = QColor::fromRgba(style.fill);
    m_visible = true;
    emit tipChanged();
}

void LabelTips::leave()
{
    clear();
}

void LabelTips::clear()
{
    if (!m_visible && m_text.isEmpty()) {
        return;
    }
    m_visible = false;
    m_text.clear();
    emit tipChanged();
}

} // namespace actcraft::ui
