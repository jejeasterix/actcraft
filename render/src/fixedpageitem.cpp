#include <actcraft/core/generator.h>
#include <actcraft/render/fixedpageitem.h>

#include <QQuickWindow>
#include <QSGRectangleNode>
#include <QSGRendererInterface>

namespace actcraft::render {

namespace {
void clearChildren(QSGNode* root)
{
    while (QSGNode* child = root->firstChild()) {
        root->removeChildNode(child);
        delete child;
    }
}
} // namespace

FixedPageItem::FixedPageItem(QQuickItem* parent)
    : QQuickItem(parent), m_registry(RendererRegistry::withDefaults())
{
    setFlag(ItemHasContents);
    connect(&m_images, &ImageStore::ready, this, &FixedPageItem::onImagesReady);
}

void FixedPageItem::loadGeneratedPage()
{
    setPage(core::generatePage(core::kDefaultSeed));
}

void FixedPageItem::setPage(core::Page page)
{
    m_page = std::move(page);
    m_rebuild = true;
    if (m_ready) {
        m_ready = false;
        emit readyChanged();
    }
    m_images.generate(m_page);
    emit objectCountChanged();
    update();
}

const core::Page& FixedPageItem::page() const noexcept
{
    return m_page;
}

void FixedPageItem::appendStroke(core::Stroke stroke)
{
    m_page.objects.emplace_back(std::move(stroke));
    emit objectCountChanged();
    emit strokeAppended();
    update();
}

bool FixedPageItem::isReady() const noexcept
{
    return m_ready;
}

int FixedPageItem::objectCount() const noexcept
{
    return static_cast<int>(m_page.objects.size());
}

void FixedPageItem::onImagesReady()
{
    m_rebuild = true;
    m_ready = true;
    emit readyChanged();
    update();
}

void FixedPageItem::geometryChange(const QRectF& newGeometry, const QRectF& oldGeometry)
{
    QQuickItem::geometryChange(newGeometry, oldGeometry);
    m_rebuild = true;
    update();
}

QSGNode* FixedPageItem::updatePaintNode(QSGNode* old, UpdatePaintNodeData*)
{
    QSGNode* root = old ? old : new QSGNode;
    if (m_rebuild || !old) {
        clearChildren(root);
        QSGRectangleNode* background = window()->createRectangleNode();
        background->setRect(boundingRect());
        background->setColor(Qt::white);
        root->appendChildNode(background);
        m_builtCount = 0;
        m_rebuild = !m_ready; // tant que les images manquent, on ne dessine que le fond
    }
    if (m_ready) {
        RenderContext context;
        context.window = window();
        context.software = window()->rendererInterface()->graphicsApi() == QSGRendererInterface::Software;
        context.images = &m_images;
        for (std::size_t i = m_builtCount; i < m_page.objects.size(); ++i) {
            if (const ObjectRenderer* renderer = m_registry.find(core::kindOf(m_page.objects[i]))) {
                if (QSGNode* node = renderer->createNode(m_page.objects[i], i, context)) {
                    root->appendChildNode(node);
                }
            }
        }
        m_builtCount = m_page.objects.size();
    }
    return root;
}

} // namespace actcraft::render
