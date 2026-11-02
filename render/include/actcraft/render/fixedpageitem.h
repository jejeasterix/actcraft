#pragma once

#include <actcraft/core/document.h>
#include <actcraft/core/model.h>
#include <actcraft/core/pagecache.h>
#include <actcraft/render/imagestore.h>
#include <actcraft/render/objectrenderer.h>
#include <actcraft/render/paintedrenderer.h>

#include <QQuickItem>
#include <QRectF>

#include <atomic>
#include <cstdint>
#include <string>
#include <vector>

class QSGRectangleNode;

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
    Q_PROPERTY(int pendingCount READ pendingCount NOTIFY pendingChanged)

public:
    explicit FixedPageItem(QQuickItem* parent = nullptr);

    // Page des 300 objets (core::generatePage(core::kDefaultSeed)).
    Q_INVOKABLE void loadGeneratedPage();
    // Banc et tests : la page devient celle de la page interne ; la source est oubliée. Tout repart, images
    // et readyChanged compris (redécodage forcé). Depuis L2-T15 la vue redessine aussi sur contentChanged :
    // ce redécodage forcé est devenu redondant ; il coûte un décodage à chaque setPage (banc et tests
    // seulement).
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
    // sinon reconstruction complète. remove et invalidate reconstruisent toute la page, sauf pour un objet en
    // attente (lot 10, setDeferredFolding ci-dessous). Chaque appel qui change la scène émet contentChanged ;
    // un objet mis en attente, retiré ou modifié pendant qu'il attend émet seulement pendingChanged.
    void insert(core::ObjectId id) override;
    void remove(core::ObjectId id, const core::RectF& area) override;
    void invalidate(core::ObjectId id, const core::CacheArea& area) override;
    // Lot 10 (DA11, cache incrémental). Différé (posé par la vue quand une couche d'attente est branchée et
    // le cache actif) : un objet inséré en fin de page, sans image, ni encre révélatrice, page prête et sans
    // grille dessinée au-dessus (canWait), n'entre PAS dans la scène de l'élément (celle que le cache
    // photographie) : il attend, dessiné par la couche d'attente (PendingLayerItem), jusqu'à foldPending().
    // Retiré (remove) ou modifié (invalidate) pendant qu'il attend : seule la couche d'attente change. Tout
    // autre changement reconstruit tout et verse ce qui attend. Sans différé (défaut) : comportement du
    // lot 2. Repasser à faux verse tout.
    void setDeferredFolding(bool on);
    bool deferredFolding() const noexcept;
    // Verse les objets en attente dans la scène : à appeler dans le même tour de boucle que la demande de
    // redessin du cache (la vue le fait dans requestCacheRefresh) ; la même image montre alors le cache qui
    // les contient et la couche d'attente vide.
    void foldPending();
    // Objets en attente : toujours les derniers de la page, dans l'ordre de la page.
    int pendingCount() const noexcept;
    const std::vector<core::ObjectId>& pendingIds() const noexcept;
    // Augmente quand un objet en attente change sans changer de place (invalidate) : la couche refait tout.
    std::uint64_t pendingRevision() const noexcept;
    // Objets de la page que la scène de l'élément contient : les cachedObjectCount() premiers. La scène
    // s'arrête là à chaque synchronisation (updatePaintNode) ; la couche d'attente dessine les suivants.
    std::size_t cachedObjectCount() const noexcept;
    bool forceRasterized()
        const noexcept; // crochet de test lu par les couches d'attente et soulevée (même chemin RHI)
    // Magasin des images décodées de la page affichée, en lecture (L3-C1) : la couche soulevée y cherche
    // par clé les images à ressource de ses copies, sans second décodage. Lecture sûre depuis le fil de rendu
    // (ImageStore). Les images synthétiques y sont rangées par indice de cette page.
    const ImageStore& images() const noexcept;
    // Vrai quand les images sont prêtes et les nœuds construits à la prochaine image. Tant qu'il est faux
    // après un changement de page, l'élément garde les nœuds de la page précédente (pas d'éclair blanc,
    // L2-T23).
    bool isReady() const noexcept;
    int objectCount() const noexcept;
    // Pixels d'écran par pixel de page : échelle de rastérisation du moteur peint en RHI (décision 37). Ne
    // reconstruit qu'en RHI et seulement si la page a des éléments peints.
    void setRasterScale(double scale);
    double rasterScale() const noexcept;
    int rebuildCount() const noexcept; // crochet de test : reconstructions complètes de la scène
    int decodeCount() const noexcept;  // crochet de test : décodages d'images lancés
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
    void pendingChanged();

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
    // `Force` : image ajoutée ou page remplacée, on redécode ; `IfUncovered` : seulement si le magasin ne
    // couvre plus la page.
    enum class Decode : std::uint8_t
    {
        IfUncovered,
        Force
    };
    // `SamePage` : la page affichée a seulement changé de contenu (cache, trait) ; pendant son décodage, le
    // décodage en cours la couvre et n'est pas relancé (sauf `Force`).
    enum class Scope : std::uint8_t
    {
        OtherPage,
        SamePage
    };
    // Reconstruction complète ; images régénérées selon `decode`.
    void restart(Decode decode, Scope scope);
    void onImagesReady();
    void addRasterSlot(QSGNode* node, std::size_t object);
    void rescaleRasterNodes(const core::Page& page, const RenderContext& painted);
    // Invariant de la couche d'attente : les objets en attente sont, dans l'ordre, les derniers de la page.
    bool pendingAreTheLastObjects() const noexcept;

    const core::Document* m_source = nullptr;
    std::size_t m_sourcePage = 0;
    core::Document m_own; // page interne (une page)
    RendererRegistry m_registry;
    PaintedRenderer m_painted;
    ImageStore m_images;
    bool m_ready = false;
    bool m_rebuild = true;
    bool m_showsPage = false; // fil de rendu : les nœuds montrent une page prête, pas le fond d'attente
    QRectF m_shownRect;       // fil de rendu : rectangle de l'élément quand la page affichée a été construite
    QSGRectangleNode* m_underlay = nullptr; // fil de rendu : fond blanc à la taille de la page suivante
    int m_decodeCount = 0;
    // Images remises au décodage en cours, avec leur indice d'objet (le magasin range les images
    // synthétiques par indice) : il ne couvre la page que si elle les a encore aux mêmes indices.
    std::string m_decodingInputs;
    std::size_t m_builtCount = 0;
    double m_rasterScale = 1;
    QSGNode* m_gridDrawnAbove = nullptr; // fil de rendu : les objets s'insèrent avant la grille du dessus
    std::atomic<int> m_rebuildCount{0};
    bool m_forceRasterized = false;
    bool m_rescale = false;                // nouvelle échelle : refaire les nœuds rastérisés seulement
    std::vector<RasterSlot> m_rasterSlots; // fil de rendu
    std::atomic<int> m_rasterNodes{0};
    std::atomic<int> m_rasterizationCount{0};
    bool m_deferred = false;
    std::vector<core::ObjectId> m_pendingIds; // fil principal ; lu par les synchronisations
    std::uint64_t m_pendingRevision = 0;
};

} // namespace actcraft::render
