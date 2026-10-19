#include <actcraft/render/liftlayeritem.h>

// Talon L0-S : rempli par L3-T10.

namespace actcraft::render {

LiftLayerItem::LiftLayerItem(QQuickItem* parent) : QQuickItem(parent) {}

FixedPageItem* LiftLayerItem::fixedPage() const noexcept
{
    return m_fixedPage.data();
}

void LiftLayerItem::setFixedPage(FixedPageItem* page)
{
    m_fixedPage = page;
}

void LiftLayerItem::setObjects(std::vector<core::PageObject>)
{
    static_cast<void>(m_objects);
}

void LiftLayerItem::setExtraTransform(const core::Transform&) {}

void LiftLayerItem::clear() {}

int LiftLayerItem::objectCount() const noexcept
{
    return 0;
}

int LiftLayerItem::builtCount() const noexcept
{
    return 0;
}

QSGNode* LiftLayerItem::updatePaintNode(QSGNode* old, UpdatePaintNodeData*)
{
    return old;
}

} // namespace actcraft::render
