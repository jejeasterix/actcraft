#pragma once

#include <actcraft/core/document.h>
#include <actcraft/core/model.h>
#include <actcraft/core/pagecache.h>
#include <actcraft/render/imagestore.h>
#include <actcraft/render/objectrenderer.h>
#include <actcraft/render/paintedrenderer.h>

#include <QQuickItem>

#include <atomic>
#include <vector>

namespace actcraft::render {

// Contenu fixe de la page affichée : modèle → nœuds par les moteurs (décision 10). La page est LUE dans le
// document (setSource), jamais copiée ; ses changements arrivent par l'interface de cache (DA11). Pour le
// banc et les tests, une page interne (setPage, appendStroke, loadGeneratedPage). Le cache
// (ShaderEffectSource, PageArea.qml) prend cet élément pour source.
class FixedPageItem : public QQuickItem, public core::PageCache
{
    Q_OBJECT
    Q_PROPERTY(bool ready READ isReady NOTIFY readyChanged)
    Q_PROPERTY(int objectCount READ objectCount NOTIFY objectCountChanged)

public:
    explicit FixedPageItem(QQuickItem* parent = nullptr);

    // Page des 300 objets (core::generatePage(core::kDefaultSeed)).
    Q_INVOKABLE void loadGeneratedPage();
    // Banc et tests (et relais du contrôleur jusqu'à L2-T15) : la page devient celle de la page interne ; la
    // source est oubliée. Tout repart, images et readyChanged compris : la vue redessine son cache sur
    // readyChanged.
    void setPage(core::Page page);
    // Page affichée : celle du document source (référence dans le document), sinon la page interne. Jamais
    // hors des bornes : une page source retirée du document se lit comme la page interne, vide, jusqu'à
    // setSource.
    const core::Page& page() const noexcept;
    // Page interne seulement : refusé, avec un avertissement, quand l'élément montre un document. Le trait va
    // au sommet de la couche supérieure ; ajouté à la scène sans reconstruire le reste (insert).
    void appendStroke(core::Stroke stroke);
    // Page `pageIndex` de `document` : le document doit survivre à l'élément, ou appeler setSource(nullptr,
    // 0) avant de mourir. nullptr, ou indice absent du document (avertissement) : page interne vide.
    // L'ancienne page interne est oubliée dans tous les cas.
    void setSource(const core::Document* document, std::size_t pageIndex);
    const core::Document* sourceDocument() const noexcept;
    std::size_t sourcePage() const noexcept;
    // core::PageCache (DA11), implémentation du lot 2 : ajout en fin de page sans reconstruction (seul le
    // nœud de l'objet est construit ; une image ou un média passe par la reconstruction, qui les décode),
    // sinon reconstruction complète. remove et invalidate reconstruisent toute la page : l'identifiant et la
    // zone qu'ils portent serviront au lot « cache incrémental » à viser le nœud. Chaque appel émet
    // contentChanged.
    void insert(core::ObjectId id) override;
    void remove(core::ObjectId id, const core::RectF& area) override;
    void invalidate(core::ObjectId id, const core::CacheArea& area) override;
    // Vrai quand les images sont prêtes et les nœuds construits à la prochaine image.
    bool isReady() const noexcept;
    int objectCount() const noexcept;
    // Pixels d'écran par pixel de page : échelle de rastérisation du moteur peint en RHI (décision 37). Ne
    // reconstruit qu'en RHI et seulement si la page a des éléments peints.
    void setRasterScale(double scale);
    double rasterScale() const noexcept;
    int rebuildCount() const noexcept; // crochet de test : reconstructions complètes de la scène
    // Crochet de test : le moteur peint, le fond peint et la grille passent par des textures (chemin RHI)
    // même en rendu logiciel ; reconstruit la scène.
    void setForceRasterized(bool on);
    // Crochet de test : nœuds rastérisés créés (construction et changements d'échelle).
    int rasterizationCount() const noexcept;

signals:
    void readyChanged();
    void objectCountChanged();
    void strokeAppended();
    void contentChanged();

protected:
    QSGNode* updatePaintNode(QSGNode* old, UpdatePaintNodeData* data) override;
    void geometryChange(const QRectF& newGeometry, const QRectF& oldGeometry) override;

private:
    // Nœud rastérisé (RHI) refait en place quand l'échelle change : fond peint, grille ou objet d'indice
    // `object`.
    struct RasterSlot
    {
        QSGNode* node = nullptr;
        std::size_t object = 0;
    };

    const core::Document& document() const noexcept; // source, ou page interne
    // Reconstruction complète ; images régénérées si `redecode` ou si le magasin ne couvre plus la page.
    void restart(bool redecode);
    void onImagesReady();
    void addRasterSlot(QSGNode* node, std::size_t object);
    void rescaleRasterNodes(const core::Page& page, const RenderContext& painted);

    const core::Document* m_source = nullptr;
    std::size_t m_sourcePage = 0;
    core::Document m_own; // page interne (une page)
    RendererRegistry m_registry;
    PaintedRenderer m_painted;
    ImageStore m_images;
    bool m_ready = false;
    bool m_rebuild = true;
    std::size_t m_builtCount = 0;
    double m_rasterScale = 1;
    QSGNode* m_gridDrawnAbove = nullptr; // fil de rendu : les objets s'insèrent avant la grille du dessus
    std::atomic<int> m_rebuildCount{0};
    bool m_forceRasterized = false;
    bool m_rescale = false;                // nouvelle échelle : refaire les nœuds rastérisés seulement
    std::vector<RasterSlot> m_rasterSlots; // fil de rendu
    std::atomic<int> m_rasterNodes{0};
    std::atomic<int> m_rasterizationCount{0};
};

} // namespace actcraft::render
