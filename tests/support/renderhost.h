#pragma once

#include <actcraft/render/objectrenderer.h>

#include <QQuickItem>
#include <QQuickWindow>
#include <QSGNode>
#include <QSGRendererInterface>

#include <vector>

// Élément de test : dessine une liste d'objets par le registre de moteurs, comme la page fixe.
class RenderHostItem : public QQuickItem
{
public:
    RenderHostItem() { setFlag(ItemHasContents); }

    std::vector<actcraft::core::PageObject> objects;
    const actcraft::render::ImageStore* images = nullptr;

protected:
    QSGNode* updatePaintNode(QSGNode* old, UpdatePaintNodeData*) override
    {
        QSGNode* root = old ? old : new QSGNode;
        while (QSGNode* child = root->firstChild()) {
            root->removeChildNode(child);
            delete child;
        }
        actcraft::render::RenderContext context;
        context.window = window();
        context.software = window()->rendererInterface()->graphicsApi() == QSGRendererInterface::Software;
        context.images = images;
        for (std::size_t i = 0; i < objects.size(); ++i) {
            if (const auto* renderer = m_registry.find(actcraft::core::kindOf(objects[i]))) {
                if (QSGNode* node = renderer->createNode(objects[i], i, context)) {
                    root->appendChildNode(node);
                }
            }
        }
        return root;
    }

private:
    actcraft::render::RendererRegistry m_registry = actcraft::render::RendererRegistry::withDefaults();
};

inline bool colorNear(QColor actual, QColor expected, int tolerance)
{
    return qAbs(actual.red() - expected.red()) <= tolerance &&
           qAbs(actual.green() - expected.green()) <= tolerance &&
           qAbs(actual.blue() - expected.blue()) <= tolerance;
}
