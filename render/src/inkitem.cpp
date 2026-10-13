#include <actcraft/core/clock.h>
#include <actcraft/render/inkitem.h>
#include <actcraft/render/objectrenderer.h>
#include <actcraft/render/strokegeometry.h>

#include <QQuickWindow>
#include <QSGFlatColorMaterial>
#include <QSGGeometryNode>
#include <QSGRendererInterface>

#include <cstring>
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

// Bloc RHI : géométrie allouée une fois à sa capacité, place libre à zéro (triangles dégénérés, invisibles).
// Seuls les nouveaux segments sont écrits. Ne pas reprendre makeTrianglesNode (StrokeRenderer) : sa géométrie
// a la taille exacte du trait, la réallouer à chaque point recopierait tout le trait à chaque image.
class InkBlockNode final : public QSGGeometryNode
{
public:
    static constexpr int kCapacity = static_cast<int>((InkBuffer::kBlockPoints - 1) * kVerticesPerSegment);

    explicit InkBlockNode(QColor color) : m_geometry(QSGGeometry::defaultAttributes_Point2D(), kCapacity)
    {
        m_geometry.setDrawingMode(QSGGeometry::DrawTriangles);
        m_geometry.setVertexDataPattern(QSGGeometry::DynamicPattern);
        std::memset(m_geometry.vertexData(), 0, static_cast<std::size_t>(kCapacity) * sizeof(Vertex2D));
        setGeometry(&m_geometry);
        m_material.setColor(color);
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
    QSGFlatColorMaterial m_material;
    std::size_t m_segments = 0;
    std::vector<Vertex2D> m_scratch;
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
    if (m_buffer.clearedSinceClean()) {
        clearChildren(root);
    }
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
    const std::size_t count = buffer.blockCount();
    const std::size_t firstDirty = buffer.firstDirtyBlock();
    QSGNode* child = root->firstChild();
    // Nœuds existants jusqu'au premier bloc sale (sans y toucher), puis blocs sales et nœuds manquants.
    for (std::size_t index = 0; index < count; ++index) {
        if (!child) {
            child = new InkBlockNode(toQColor(buffer.color()));
            root->appendChildNode(child);
        }
        if (index >= firstDirty) {
            static_cast<InkBlockNode*>(child)->appendFrom(buffer.block(index), buffer.baseWidth());
        }
        child = child->nextSibling();
    }
}

void InkItem::updateSoftware(QSGNode*)
{
    // Chemin logiciel : tâche T10 (un nœud de rendu par bloc). Rien n'est dessiné en logiciel avant elle.
}

} // namespace actcraft::render
