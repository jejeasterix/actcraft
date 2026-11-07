#include <actcraft/ui/fingermenus.h>

#include <QVariantList>
#include <QVariantMap>

#include <algorithm>

namespace actcraft::ui {

namespace {

// Coin d'un cadre de sélection : {x, y} (liste QML) ou QPointF.
QPointF cornerOf(const QVariant& value)
{
    const QVariantMap map = value.toMap();
    if (!map.isEmpty()) {
        return {map.value(QStringLiteral("x")).toDouble(), map.value(QStringLiteral("y")).toDouble()};
    }
    return value.toPointF();
}

} // namespace

FingerMenus::FingerMenus(QObject* parent) : QObject(parent) {}

void FingerMenus::setView(PageView* view)
{
    if (m_view == view) {
        return;
    }
    if (m_view) {
        disconnect(m_view, &PageView::touchHeld, this, nullptr);
    }
    m_view = view;
    if (m_view) {
        connect(m_view, &PageView::touchHeld, this, &FingerMenus::onTouchHeld);
    }
    emit wiringChanged();
}

void FingerMenus::setTools(ToolHost* tools)
{
    if (m_tools == tools) {
        return;
    }
    m_tools = tools;
    emit wiringChanged();
}

void FingerMenus::reset()
{
    setKind(QString());
}

void FingerMenus::setKind(const QString& kind)
{
    if (m_kind == kind) {
        return;
    }
    m_kind = kind;
    emit kindChanged();
}

bool FingerMenus::insideSelection(QPointF pagePoint) const
{
    const QVariantMap frame = m_tools->selectionFrame();
    const QVariantList corners = frame.value(QStringLiteral("corners")).toList();
    if (!frame.value(QStringLiteral("visible")).toBool() || corners.size() < 4) {
        return false;
    }
    qreal left = cornerOf(corners.first()).x();
    qreal right = left;
    qreal top = cornerOf(corners.first()).y();
    qreal bottom = top;
    for (const QVariant& c : corners) {
        const QPointF p = cornerOf(c);
        left = std::min(left, p.x());
        right = std::max(right, p.x());
        top = std::min(top, p.y());
        bottom = std::max(bottom, p.y());
    }
    return pagePoint.x() >= left && pagePoint.x() <= right && pagePoint.y() >= top && pagePoint.y() <= bottom;
}

void FingerMenus::onTouchHeld(QPointF pagePoint, QPointF windowPoint, int /*user*/)
{
    if (!m_tools || m_tools->currentTool() != QLatin1String("select")) {
        return; // outil d'encre (ou autre) : l'appui long ne montre aucun menu
    }
    if (insideSelection(pagePoint)) {
        setKind(QStringLiteral("object"));
        emit objectMenuRequested(windowPoint);
    } else {
        setKind(QStringLiteral("page"));
        emit pageMenuRequested(windowPoint);
    }
}

} // namespace actcraft::ui
