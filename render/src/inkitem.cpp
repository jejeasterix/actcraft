#include <actcraft/core/clock.h>
#include <actcraft/render/inkitem.h>
#include <actcraft/render/objectrenderer.h>
#include <actcraft/render/painternode.h>
#include <actcraft/render/strokegeometry.h>

#include <QPainter>
#include <QPainterPath>
#include <QPen>
#include <QQuickWindow>
#include <QSGFlatColorMaterial>
#include <QSGGeometryNode>
#include <QSGRendererInterface>
#include <QSGTransformNode>

#include <algorithm>
#include <atomic>
#include <cstddef>
#include <cstring>
#include <numbers>
#include <optional>
#include <span>
#include <vector>

namespace actcraft::render {

namespace {

// Segments peints par les blocs logiciels (InkItem::softwareSegmentsPainted).
std::atomic<std::uint64_t> g_softwareSegments{0};

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

core::PointF pointOf(const core::StrokePoint& p)
{
    return {p.x, p.y};
}

// Bloc RHI : géométrie allouée une fois à sa capacité. Seuls les nouveaux segments sont écrits (forme ronde :
// avec le segment provisoire qu'ils remplacent). Ne pas reprendre makeTrianglesNode (StrokeRenderer) : sa
// géométrie a la taille exacte du trait, la réallouer à chaque point recopierait tout le trait à chaque
// image.
class InkBlockNode final : public QSGGeometryNode
{
public:
    static constexpr int kCapacity = static_cast<int>((InkBuffer::kBlockPoints - 1) * kVerticesPerSegment);
    static constexpr int kRoundCapacity = static_cast<int>(InkItem::kRoundBlockVertices);

    // `first` : premier point du bloc. La place libre y est posée : triangles dégénérés, invisibles, dans les
    // bornes du bloc (à zéro, ils étendraient les bornes du nœud jusqu'à l'origine).
    InkBlockNode(QColor color, std::size_t block, const core::StrokePoint& first, bool rounded)
        : m_geometry(QSGGeometry::defaultAttributes_Point2D(), rounded ? kRoundCapacity : kCapacity),
          m_material(color, block), m_rest{static_cast<float>(first.x), static_cast<float>(first.y)},
          m_round(rounded)
    {
        m_geometry.setDrawingMode(QSGGeometry::DrawTriangles);
        m_geometry.setVertexDataPattern(QSGGeometry::DynamicPattern);
        std::fill_n(vertices(), m_geometry.vertexCount(), m_rest);
        setGeometry(&m_geometry);
        setMaterial(&m_material);
    }

    // `points` : tout le bloc ; `next` : point qui suit le bloc (second point du bloc suivant), nul pour le
    // dernier bloc ; `firstBlock`, `lastBlock` : place du bloc dans le trait.
    void appendFrom(std::span<const core::StrokePoint> points, const core::StrokePoint* next, bool firstBlock,
                    bool lastBlock, double baseWidth)
    {
        if (m_round) {
            appendRound(points, next, firstBlock, lastBlock, baseWidth);
            return;
        }
        // Prototype : les segments [m_segments, points.size() − 1) sont nouveaux ; chacun ne dépend que de
        // ses deux points (largeur au premier, bouts carrés), comme dans appendStrokeTriangles.
        if (points.size() < 2 || points.size() - 1 <= m_segments) {
            return;
        }
        m_scratch.clear();
        appendStrokeTriangles(points.subspan(m_segments), baseWidth, m_scratch);
        write(m_segments * kVerticesPerSegment);
        m_segments = points.size() - 1;
        m_geometry.markVertexDataDirty();
        markDirty(QSGNode::DirtyGeometry);
    }

private:
    Vertex2D* vertices() { return static_cast<Vertex2D*>(m_geometry.vertexData()); }

    void write(std::size_t at)
    {
        std::memcpy(vertices() + at, m_scratch.data(), m_scratch.size() * sizeof(Vertex2D));
    }

    // Forme ronde : segments depuis le premier qui n'est pas final (le provisoire, puis les nouveaux), disque
    // de départ (bloc 0, une fois), disque d'arrivée (dernier bloc, sur son dernier point ; retiré quand le
    // bloc cesse d'être le dernier). Rien n'est écrit, ni marqué modifié, pour un bloc déjà final.
    void appendRound(std::span<const core::StrokePoint> points, const core::StrokePoint* next,
                     bool firstBlock, bool lastBlock, double baseWidth)
    {
        bool changed = false;
        const std::size_t segments = points.size() < 2 ? 0 : points.size() - 1;
        if (firstBlock && !m_startCap && !points.empty()) {
            m_scratch.clear();
            appendRoundInkCap(pointOf(points.front()), baseWidth, m_scratch);
            write(0);
            m_startCap = true;
            changed = true;
        }
        for (std::size_t s = m_final; s < segments; ++s) {
            const core::StrokePoint* after = s + 2 < points.size() ? &points[s + 2] : next;
            const core::PointF afterPoint = after ? pointOf(*after) : core::PointF{};
            m_scratch.clear();
            appendRoundInkSegment(pointOf(points[s]), pointOf(points[s + 1]), after ? &afterPoint : nullptr,
                                  firstBlock && s == 0, baseWidth, m_scratch);
            write(kRoundInkCapVertices + s * kRoundInkVerticesPerSegment);
            if (after) {
                m_final = s + 1;
            }
            changed = true;
        }
        constexpr std::size_t endCap =
            kRoundInkCapVertices + (InkBuffer::kBlockPoints - 1) * kRoundInkVerticesPerSegment;
        const bool wantEnd = lastBlock && segments > 0;
        if (wantEnd && (changed || !m_endCap)) {
            m_scratch.clear();
            appendRoundInkCap(pointOf(points.back()), baseWidth, m_scratch);
            write(endCap);
            changed = true;
        } else if (!wantEnd && m_endCap) {
            std::fill_n(vertices() + endCap, kRoundInkCapVertices, m_rest);
            changed = true;
        }
        m_endCap = wantEnd;
        if (changed) {
            m_geometry.markVertexDataDirty();
            markDirty(QSGNode::DirtyGeometry);
        }
    }

    QSGGeometry m_geometry;
    InkBlockMaterial m_material;
    Vertex2D m_rest;
    bool m_round;
    std::size_t m_segments = 0; // prototype : segments écrits
    std::size_t m_final = 0;    // forme ronde : segments finals écrits (ils ne bougent plus)
    bool m_startCap = false;
    bool m_endCap = false;
    std::vector<Vertex2D> m_scratch;
};

// Bloc logiciel : le moteur logiciel repeint tout le rect() d'un nœud sali, et rappelle render() de chaque
// nœud dont le rect() chevauche la zone repeinte, découpé à cette zone. Ne pas reprendre StrokePainterNode
// (StrokeRenderer) : un nœud unique au rect() du trait entier repeindrait tout le trait à chaque point. Un
// nœud par bloc, rect() de son bloc : un bloc plein ne change plus, ni ses points ni son rect(), et n'est
// plus marqué modifié (forme ronde : une fois de plus, quand le point qui suit sa jonction arrive et que son
// dernier segment devient final). Il est encore rappelé quand son rect() chevauche celui du dernier bloc
// (toujours le bloc précédent, par la jonction ; tous les blocs d'un trait qui se recroise) : paint() n'en
// peint alors que les segments qui touchent la zone repeinte. Les points sont copiés : render() peut tourner
// sur le fil de rendu.
class InkBlockPainterNode final : public PainterNode
{
public:
    InkBlockPainterNode(QQuickWindow* window, QColor color, double baseWidth, bool rounded, bool firstBlock)
        : PainterNode(window), m_color(color), m_baseWidth(baseWidth), m_round(rounded),
          m_firstBlock(firstBlock)
    {}

    // `points` : tout le bloc, dont les m_points.size() premiers sont déjà copiés (un bloc ne fait que
    // grandir). `bounds` : InkBuffer::blockBounds du bloc. `next` : point qui suit le bloc (forme ronde).
    void appendFrom(std::span<const core::StrokePoint> points, const core::RectF& bounds,
                    const core::StrokePoint* next)
    {
        const bool gotNext = m_round && next && !m_next;
        if (points.size() <= m_points.size() && !gotNext) {
            return;
        }
        for (const core::StrokePoint& point : points.subspan(m_points.size())) {
            m_points.push_back(point);
            m_maxHalf = std::max(m_maxHalf, core::strokeWidthAt(m_baseWidth, point.pressure) / 2);
        }
        if (gotNext) {
            m_next = *next;
        }
        // blockBounds agrandit les points de la demi-largeur h ; le coin d'un bout carré est à h√2 de son
        // point (segment en diagonale) et en sort donc jusqu'à (√2 − 1) × h. Forme ronde : largeur baseWidth,
        // courbe dans l'enveloppe des points et du point suivant. Hors de rect(), le moteur le rognerait.
        const double margin =
            m_round ? std::max(0.0, m_baseWidth / 2 - m_maxHalf) + 1 : (std::numbers::sqrt2 - 1) * m_maxHalf;
        QRectF area(bounds.x - margin, bounds.y - margin, bounds.width + 2 * margin,
                    bounds.height + 2 * margin);
        if (m_next) {
            const double reach = m_baseWidth / 2 + 1;
            area |= QRectF(m_next->x - reach, m_next->y - reach, 2 * reach, 2 * reach);
        }
        setRect(area);
        markDirty(QSGNode::DirtyMaterial);
    }

protected:
    // Seuls les segments qui touchent la découpe (zone repeinte, en coordonnées de l'élément) sont peints,
    // par suites de segments consécutifs.
    void paint(QPainter& painter) override
    {
        const std::size_t segments = m_points.size() < 2 ? 0 : m_points.size() - 1;
        if (!painter.hasClipping()) {
            paintRun(painter, 0, segments);
            return;
        }
        const QRectF clip = painter.clipBoundingRect();
        std::size_t runStart = 0;
        bool inRun = false;
        for (std::size_t i = 0; i < segments; ++i) {
            const bool meets = segmentReach(i).intersects(clip);
            if (meets && !inRun) {
                runStart = i;
                inRun = true;
            } else if (!meets && inRun) {
                paintRun(painter, runStart, i - runStart);
                inRun = false;
            }
        }
        if (inRun) {
            paintRun(painter, runStart, segments - runStart);
        }
    }

private:
    // Point qui suit le segment s (forme ronde) : dans le bloc, ou le premier du bloc suivant ; nul sinon.
    const core::StrokePoint* afterOf(std::size_t s) const
    {
        if (s + 2 < m_points.size()) {
            return &m_points[s + 2];
        }
        return m_next ? &*m_next : nullptr;
    }

    // Rectangle que le segment s peut toucher. Prototype : bout carré, coins à demi-largeur × √2 de leur
    // point. Forme ronde : enveloppe de ses deux points et du suivant, demi-largeur baseWidth / 2. Plus une
    // unité d'arrondi.
    QRectF segmentReach(std::size_t s) const
    {
        const core::StrokePoint& a = m_points[s];
        const core::StrokePoint& b = m_points[s + 1];
        QRectF r(QPointF(std::min(a.x, b.x), std::min(a.y, b.y)),
                 QPointF(std::max(a.x, b.x), std::max(a.y, b.y)));
        double reach = core::strokeWidthAt(m_baseWidth, a.pressure) / 2 * std::numbers::sqrt2 + 1;
        if (m_round) {
            reach = m_baseWidth / 2 + 1;
            if (const core::StrokePoint* after = afterOf(s)) {
                // Pas d'union avec un rectangle vide : QRectF::united l'ignorerait.
                r.setLeft(std::min(r.left(), after->x));
                r.setTop(std::min(r.top(), after->y));
                r.setRight(std::max(r.right(), after->x));
                r.setBottom(std::max(r.bottom(), after->y));
            }
        }
        return r.adjusted(-reach, -reach, reach, reach);
    }

    // Segments [first, first + count).
    void paintRun(QPainter& painter, std::size_t first, std::size_t count) const
    {
        if (count == 0) {
            return;
        }
        if (m_round) {
            paintRound(painter, first, count);
        } else {
            paintStrokeSegments(painter,
                                std::span<const core::StrokePoint>(m_points).subspan(first, count + 1),
                                m_baseWidth, m_color);
        }
        g_softwareSegments.fetch_add(count, std::memory_order_relaxed);
    }

    // Forme ronde : la courbe par les milieux de ces segments (appendRoundInkSegment, même découpe), d'un
    // seul chemin à bouts et jonctions ronds ; le dernier segment du trait, sans point suivant, est droit.
    void paintRound(QPainter& painter, std::size_t first, std::size_t count) const
    {
        const auto mid = [](const core::StrokePoint& a, const core::StrokePoint& b) {
            return QPointF((a.x + b.x) / 2, (a.y + b.y) / 2);
        };
        QPainterPath path;
        path.moveTo(m_firstBlock && first == 0 ? QPointF(m_points[0].x, m_points[0].y)
                                               : mid(m_points[first], m_points[first + 1]));
        for (std::size_t s = first; s < first + count; ++s) {
            const core::StrokePoint& b = m_points[s + 1];
            if (const core::StrokePoint* after = afterOf(s)) {
                path.quadTo(QPointF(b.x, b.y), mid(b, *after));
            } else {
                path.lineTo(b.x, b.y);
            }
        }
        QPen pen(m_color);
        pen.setWidthF(m_baseWidth);
        pen.setCapStyle(Qt::RoundCap);
        pen.setJoinStyle(Qt::RoundJoin);
        painter.setPen(pen);
        painter.setBrush(Qt::NoBrush);
        painter.drawPath(path);
    }

    QColor m_color;
    double m_baseWidth;
    bool m_round;
    bool m_firstBlock;
    double m_maxHalf = 0;
    std::vector<core::StrokePoint> m_points;
    std::optional<core::StrokePoint> m_next;
};

} // namespace

std::uint64_t InkItem::softwareSegmentsPainted() noexcept
{
    return g_softwareSegments.load(std::memory_order_relaxed);
}

InkItem::InkItem(QQuickItem* parent) : QQuickItem(parent)
{
    setFlag(ItemHasContents);
}

void InkItem::beginStroke(core::Argb color, double baseWidth, core::PenCap cap)
{
    const std::uint32_t alpha = color >> 24;
    const bool translucent = alpha < 0xFFu;
    m_buffer.begin(translucent ? (color | 0xFF000000u) : color, baseWidth);
    m_cap = cap;
    setTranslucent(translucent);
    setOpacity(translucent ? alpha / 255.0 : 1.0);
    update();
}

bool InkItem::isTranslucent() const noexcept
{
    return m_translucent;
}

core::Argb InkItem::drawnColor() const noexcept
{
    return m_buffer.color();
}

// Couche de l'élément par le méta-objet (propriété QML layer, objet QQuickItemLayer) : aucune API privée.
void InkItem::setTranslucent(bool on)
{
    if (m_translucent == on) {
        return;
    }
    m_translucent = on;
    if (QObject* layer = property("layer").value<QObject*>()) {
        layer->setProperty("enabled", on);
    }
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
    if (m_translucent) {
        setTranslucent(false);
        setOpacity(1.0);
    }
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
    const bool software = window()->rendererInterface()->graphicsApi() == QSGRendererInterface::Software;
    QSGNode* root = old;
    if (root == nullptr) {
        // Logiciel : racine de transformation (identité). Sous une racine QSGNode nue déjà rendue, le moteur
        // logiciel donnait aux blocs ajoutés la transformation identité (et une découpe non transformée) au
        // lieu de la position et de l'échelle de l'élément ; sous un QSGTransformNode, ils reçoivent celles
        // de l'élément (relevé sur Qt 6.12 ; test transformedItemPaintsAtItsPlace).
        root = software ? new QSGTransformNode : new QSGNode;
    }
    if (software) {
        updateSoftware(root);
    } else {
        syncRhiBlocks(root, m_buffer, m_cap);
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

void InkItem::syncRhiBlocks(QSGNode* root, const InkBuffer& buffer, core::PenCap cap)
{
    if (buffer.clearedSinceClean()) {
        clearChildren(root); // trait vidé ou remplacé depuis la dernière synchronisation
    }
    const bool rounded = cap != core::PenCap::Square;
    const std::size_t count = buffer.blockCount();
    const std::size_t firstDirty = buffer.firstDirtyBlock();
    // Forme ronde : le bloc qui précède le premier bloc sale reçoit le point qui suit sa jonction (son
    // dernier segment, provisoire, devient final ; une fois, puis il n'écrit plus rien).
    const std::size_t from = rounded && firstDirty > 0 && firstDirty < count ? firstDirty - 1 : firstDirty;
    QSGNode* child = root->firstChild();
    // Nœuds existants jusqu'au premier bloc à reprendre (sans y toucher), puis blocs sales et nœuds
    // manquants.
    for (std::size_t index = 0; index < count; ++index) {
        const bool created = child == nullptr;
        if (created) {
            child = new InkBlockNode(toQColor(buffer.color()), index, buffer.block(index).front(), rounded);
            root->appendChildNode(child);
        }
        // Un nœud créé s'écrit en entier même sur un tampon propre : Qt rappelle updatePaintNode(nullptr)
        // après une réinsertion de l'élément, une perte du périphérique ou releaseResources().
        if (created || index >= from) {
            const core::StrokePoint* next = index + 1 < count ? &buffer.block(index + 1)[1] : nullptr;
            static_cast<InkBlockNode*>(child)->appendFrom(buffer.block(index), next, index == 0,
                                                          index + 1 == count, buffer.baseWidth());
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
    const bool rounded = m_cap != core::PenCap::Square;
    const std::size_t count = m_buffer.blockCount();
    const std::size_t firstDirty = m_buffer.firstDirtyBlock();
    const std::size_t from = rounded && firstDirty > 0 && firstDirty < count ? firstDirty - 1 : firstDirty;
    QSGNode* child = root->firstChild();
    for (std::size_t index = 0; index < count; ++index) {
        const core::StrokePoint* next = index + 1 < count ? &m_buffer.block(index + 1)[1] : nullptr;
        if (child == nullptr) {
            // Un nœud créé se remplit même sur un tampon propre (updatePaintNode(nullptr) après une
            // réinsertion de l'élément), avant son ajout : le moteur lit son rect() à l'ajout.
            auto* node = new InkBlockPainterNode(window(), toQColor(m_buffer.color()), m_buffer.baseWidth(),
                                                 rounded, index == 0);
            node->appendFrom(m_buffer.block(index), m_buffer.blockBounds(index), next);
            root->appendChildNode(node);
            child = node;
        } else if (index >= from) {
            static_cast<InkBlockPainterNode*>(child)->appendFrom(m_buffer.block(index),
                                                                 m_buffer.blockBounds(index), next);
        }
        child = child->nextSibling();
    }
}

} // namespace actcraft::render
