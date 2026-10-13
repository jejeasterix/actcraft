#include <actcraft/core/clock.h>
#include <actcraft/render/inkitem.h>
#include <actcraft/render/objectrenderer.h>
#include <actcraft/render/painternode.h>
#include <actcraft/render/strokegeometry.h>

#include <QQuickWindow>
#include <QSGFlatColorMaterial>
#include <QSGGeometryNode>
#include <QSGRendererInterface>

#include <algorithm>
#include <cstddef>
#include <cstring>
#include <numbers>
#include <span>
#include <vector>

namespace actcraft::render {

namespace {

void clearChildren(QSGNode* root)
{
    while (QSGNode* child = root->firstChild()) {
        root->removeChildNode(child);
        delete child;
    }
}

// Matériau d'un bloc : couleur unie, jamais égal à celui d'un autre bloc. Le moteur de rendu par lots range
// dans un même lot les nœuds dont les matériaux ont le même type() et compare() == 0, et renvoie tout le lot
// quand l'un d'eux change : avec un QSGFlatColorMaterial commun, chaque point renvoyait tous les blocs du
// trait (revue T9, mesuré en D3D11 avec QSG_RENDERER_DEBUG=upload). QSGMaterial::NoBatching ne suffit pas :
// le lot, non fusionné, est encore renvoyé en entier. Un lot par bloc : seul le bloc modifié est renvoyé.
class InkBlockMaterial final : public QSGFlatColorMaterial
{
public:
    InkBlockMaterial(QColor color, std::size_t block) : m_block(block) { setColor(color); }

    // Type propre : le moteur ne compare que des matériaux du même type, `other` est donc un
    // InkBlockMaterial.
    QSGMaterialType* type() const override
    {
        static QSGMaterialType type;
        return &type;
    }

    int compare(const QSGMaterial* other) const override
    {
        if (const int byColor = QSGFlatColorMaterial::compare(other); byColor != 0) {
            return byColor;
        }
        const std::size_t otherBlock = static_cast<const InkBlockMaterial*>(other)->m_block;
        return m_block < otherBlock ? -1 : (m_block > otherBlock ? 1 : 0);
    }

private:
    std::size_t m_block;
};

// Bloc RHI : géométrie allouée une fois à sa capacité. Seuls les nouveaux segments sont écrits. Ne pas
// reprendre makeTrianglesNode (StrokeRenderer) : sa géométrie a la taille exacte du trait, la réallouer à
// chaque point recopierait tout le trait à chaque image.
class InkBlockNode final : public QSGGeometryNode
{
public:
    static constexpr int kCapacity = static_cast<int>((InkBuffer::kBlockPoints - 1) * kVerticesPerSegment);

    // `first` : premier point du bloc. La place libre y est posée : triangles dégénérés, invisibles, dans les
    // bornes du bloc (à zéro, ils étendraient les bornes du nœud jusqu'à l'origine).
    InkBlockNode(QColor color, std::size_t block, const core::StrokePoint& first)
        : m_geometry(QSGGeometry::defaultAttributes_Point2D(), kCapacity), m_material(color, block)
    {
        m_geometry.setDrawingMode(QSGGeometry::DrawTriangles);
        m_geometry.setVertexDataPattern(QSGGeometry::DynamicPattern);
        const Vertex2D rest{static_cast<float>(first.x), static_cast<float>(first.y)};
        std::fill_n(static_cast<Vertex2D*>(m_geometry.vertexData()), kCapacity, rest);
        setGeometry(&m_geometry);
        setMaterial(&m_material);
    }

    // `points` : tout le bloc. Les segments [m_segments, points.size() − 1) sont nouveaux ; chacun ne dépend
    // que de ses deux points (largeur au premier, bouts carrés), comme dans appendStrokeTriangles.
    void appendFrom(std::span<const core::StrokePoint> points, double baseWidth)
    {
        if (points.size() < 2 || points.size() - 1 <= m_segments) {
            return;
        }
        m_scratch.clear();
        appendStrokeTriangles(points.subspan(m_segments), baseWidth, m_scratch);
        auto* vertices = static_cast<Vertex2D*>(m_geometry.vertexData());
        std::memcpy(vertices + m_segments * kVerticesPerSegment, m_scratch.data(),
                    m_scratch.size() * sizeof(Vertex2D));
        m_segments = points.size() - 1;
        m_geometry.markVertexDataDirty();
        markDirty(QSGNode::DirtyGeometry);
    }

private:
    QSGGeometry m_geometry;
    InkBlockMaterial m_material;
    std::size_t m_segments = 0;
    std::vector<Vertex2D> m_scratch;
};

// Bloc logiciel : le moteur logiciel repeint tout le rect() d'un nœud sali, et tout ce qui le recouvre. Ne
// pas reprendre StrokePainterNode (StrokeRenderer) : un nœud unique au rect() du trait entier repeindrait
// tout le trait à chaque point. Un nœud par bloc, rect() de son bloc : un bloc plein ne change plus, ni ses
// points ni son rect(), et n'est plus jamais sali. Les points sont copiés : render() peut tourner sur le fil
// de rendu.
class InkBlockPainterNode final : public PainterNode
{
public:
    InkBlockPainterNode(QQuickWindow* window, QColor color, double baseWidth)
        : PainterNode(window), m_color(color), m_baseWidth(baseWidth)
    {}

    // `points` : tout le bloc, dont les m_points.size() premiers sont déjà copiés (un bloc ne fait que
    // grandir). `bounds` : InkBuffer::blockBounds du bloc.
    void appendFrom(std::span<const core::StrokePoint> points, const core::RectF& bounds)
    {
        if (points.size() <= m_points.size()) {
            return;
        }
        for (const core::StrokePoint& point : points.subspan(m_points.size())) {
            m_points.push_back(point);
            m_maxHalf = std::max(m_maxHalf, core::strokeWidthAt(m_baseWidth, point.pressure) / 2);
        }
        // blockBounds agrandit les points de la demi-largeur h ; le coin d'un bout carré est à h√2 de son
        // point (segment en diagonale) et en sort donc jusqu'à (√2 − 1) × h. Hors de rect(), le moteur le
        // rognerait.
        const double margin = (std::numbers::sqrt2 - 1) * m_maxHalf;
        setRect(QRectF(bounds.x - margin, bounds.y - margin, bounds.width + 2 * margin,
                       bounds.height + 2 * margin));
        markDirty(QSGNode::DirtyMaterial);
    }

protected:
    void paint(QPainter& painter) override { paintStrokeSegments(painter, m_points, m_baseWidth, m_color); }

private:
    QColor m_color;
    double m_baseWidth;
    double m_maxHalf = 0;
    std::vector<core::StrokePoint> m_points;
};

} // namespace

InkItem::InkItem(QQuickItem* parent) : QQuickItem(parent)
{
    setFlag(ItemHasContents);
}

void InkItem::beginStroke(core::Argb color, double baseWidth)
{
    m_buffer.begin(color, baseWidth);
    update();
}

void InkItem::addPoint(const core::StrokePoint& point, std::uint64_t seq)
{
    m_buffer.add(point);
    m_lastSeq = seq;
    m_seqPending = true;
    update();
}

void InkItem::clear()
{
    // Le numéro en attente est gardé : en fin de trait, l'outil fige le trait (FixedPageItem::appendStroke)
    // dans la même image, et ses derniers échantillons y sont bien affichés. En cas d'annulation, ils sont
    // signalés à la sonde sans être dessinés.
    m_buffer.clear();
    update();
}

void InkItem::setProbe(core::InkProbe* probe) noexcept
{
    m_probe = probe;
}

std::size_t InkItem::pointCount() const noexcept
{
    return m_buffer.pointCount();
}

std::size_t InkItem::blockCount() const noexcept
{
    return m_buffer.blockCount();
}

// Synchronisation : le fil principal est bloqué, m_buffer et m_lastSeq se lisent sans verrou.
QSGNode* InkItem::updatePaintNode(QSGNode* old, UpdatePaintNodeData*)
{
    QSGNode* root = old ? old : new QSGNode;
    if (window()->rendererInterface()->graphicsApi() == QSGRendererInterface::Software) {
        updateSoftware(root);
    } else {
        syncRhiBlocks(root, m_buffer);
    }
    m_buffer.markClean();
    // DA4 : les échantillons jusqu'à m_lastSeq sont dans la géométrie de l'image en préparation. Une seule
    // fois par image qui apporte des points (une synchronisation sans point nouveau ne signale rien).
    if (m_probe && m_seqPending) {
        m_probe->samplesSynced(m_lastSeq, core::monotonicNowNs());
    }
    m_seqPending = false;
    return root;
}

void InkItem::syncRhiBlocks(QSGNode* root, const InkBuffer& buffer)
{
    if (buffer.clearedSinceClean()) {
        clearChildren(root); // trait vidé ou remplacé depuis la dernière synchronisation
    }
    const std::size_t count = buffer.blockCount();
    const std::size_t firstDirty = buffer.firstDirtyBlock();
    QSGNode* child = root->firstChild();
    // Nœuds existants jusqu'au premier bloc sale (sans y toucher), puis blocs sales et nœuds manquants.
    for (std::size_t index = 0; index < count; ++index) {
        const bool created = child == nullptr;
        if (created) {
            child = new InkBlockNode(toQColor(buffer.color()), index, buffer.block(index).front());
            root->appendChildNode(child);
        }
        // Un nœud créé s'écrit en entier même sur un tampon propre : Qt rappelle updatePaintNode(nullptr)
        // après une réinsertion de l'élément, une perte du périphérique ou releaseResources().
        if (created || index >= firstDirty) {
            static_cast<InkBlockNode*>(child)->appendFrom(buffer.block(index), buffer.baseWidth());
        }
        child = child->nextSibling();
    }
}

// Chemin logiciel (DA5) : mêmes blocs et mêmes règles que syncRhiBlocks, un nœud peint par bloc.
void InkItem::updateSoftware(QSGNode* root)
{
    if (m_buffer.clearedSinceClean()) {
        clearChildren(root); // trait vidé ou remplacé depuis la dernière synchronisation
    }
    const std::size_t count = m_buffer.blockCount();
    const std::size_t firstDirty = m_buffer.firstDirtyBlock();
    QSGNode* child = root->firstChild();
    for (std::size_t index = 0; index < count; ++index) {
        if (child == nullptr) {
            // Un nœud créé se remplit même sur un tampon propre (updatePaintNode(nullptr) après une
            // réinsertion de l'élément), avant son ajout : le moteur lit son rect() à l'ajout.
            auto* node = new InkBlockPainterNode(window(), toQColor(m_buffer.color()), m_buffer.baseWidth());
            node->appendFrom(m_buffer.block(index), m_buffer.blockBounds(index));
            root->appendChildNode(node);
            child = node;
        } else if (index >= firstDirty) {
            static_cast<InkBlockPainterNode*>(child)->appendFrom(m_buffer.block(index),
                                                                 m_buffer.blockBounds(index));
        }
        child = child->nextSibling();
    }
}

} // namespace actcraft::render
