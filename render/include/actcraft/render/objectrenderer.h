#pragma once

#include <actcraft/core/model.h>

#include <QColor>

#include <array>
#include <cstddef>
#include <memory>

class QQuickWindow;
class QSGNode;

namespace actcraft::render {

class ImageStore; // T5

struct RenderContext
{
    QQuickWindow* window = nullptr;
    bool software = false; // rendererInterface()->graphicsApi() == Software
    const ImageStore* images = nullptr;
    double rasterScale = 1; // pixels d'écran par pixel de page (RHI : rastérisation du moteur peint)
};

QColor toQColor(core::Argb argb, double opacity = 1.0);

// Un moteur de rendu par type d'objet (décision 5).
class ObjectRenderer
{
public:
    virtual ~ObjectRenderer() = default;
    virtual core::ObjectKind kind() const noexcept = 0;
    // Fil de rendu, pendant updatePaintNode. Le nœud appartient à l'appelant ; nullptr si rien à dessiner.
    // `objectIndex` : indice de l'objet dans Page::objects (sert aux images).
    virtual QSGNode* createNode(const core::PageObject& object, std::size_t objectIndex,
                                const RenderContext& context) const = 0;
};

class RendererRegistry
{
public:
    // Remplace le moteur du même type.
    void add(std::unique_ptr<ObjectRenderer> renderer);
    const ObjectRenderer* find(core::ObjectKind kind) const noexcept;
    std::size_t size() const noexcept;
    static RendererRegistry withDefaults();

private:
    std::array<std::unique_ptr<ObjectRenderer>, core::kObjectKindCount> m_renderers;
};

} // namespace actcraft::render
