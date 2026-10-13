#pragma once

#include <actcraft/core/model.h>
#include <actcraft/render/imagestore.h>
#include <actcraft/render/objectrenderer.h>

#include <QQuickItem>

namespace actcraft::render {

// Contenu fixe de la page : modèle → nœuds par le registre de moteurs. Les traits figés s'y ajoutent
// (décision 10). Le cache (ShaderEffectSource, Main.qml) prend cet élément pour source.
class FixedPageItem : public QQuickItem
{
    Q_OBJECT
    Q_PROPERTY(bool ready READ isReady NOTIFY readyChanged)
    Q_PROPERTY(int objectCount READ objectCount NOTIFY objectCountChanged)

public:
    explicit FixedPageItem(QQuickItem* parent = nullptr);

    // Page des 300 objets (core::generatePage(core::kDefaultSeed)).
    Q_INVOKABLE void loadGeneratedPage();
    void setPage(core::Page page);
    const core::Page& page() const noexcept;
    // Trait figé : ajouté au modèle et à la scène à la prochaine synchronisation, sans reconstruire le reste.
    void appendStroke(core::Stroke stroke);
    // Vrai quand les images sont prêtes et les nœuds construits à la prochaine image.
    bool isReady() const noexcept;
    int objectCount() const noexcept;

signals:
    void readyChanged();
    void objectCountChanged();
    void strokeAppended();

protected:
    QSGNode* updatePaintNode(QSGNode* old, UpdatePaintNodeData* data) override;
    void geometryChange(const QRectF& newGeometry, const QRectF& oldGeometry) override;

private:
    void onImagesReady();

    core::Page m_page;
    RendererRegistry m_registry;
    ImageStore m_images;
    bool m_ready = false;
    bool m_rebuild = true;
    std::size_t m_builtCount = 0;
};

} // namespace actcraft::render
