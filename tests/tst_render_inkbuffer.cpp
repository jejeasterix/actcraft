#include <actcraft/core/inkprobe.h>
#include <actcraft/render/inkbuffer.h>
#include <actcraft/render/inkitem.h>
#include <actcraft/render/strokegeometry.h>

#include <QQuickWindow>
#include <QSGFlatColorMaterial>
#include <QSGGeometryNode>
#include <QSignalSpy>
#include <QTest>

#include <atomic>
#include <mutex>
#include <span>
#include <vector>

using namespace actcraft;

namespace {

core::StrokePoint pt(int i)
{
    return {static_cast<double>(i), static_cast<double>(i % 7), 0.5f};
}

// Capacité d'un bloc RHI. Amendé par L3-T11 : forme ronde (D2), disque de départ, 255 segments de
// kRoundInkVerticesPerSegment sommets, disque d'arrivée (prototype : 255 segments × 6 sommets,
// kSquareCapacity).
constexpr int kCapacity = static_cast<int>(render::InkItem::kRoundBlockVertices);
constexpr int kSquareCapacity =
    static_cast<int>((render::InkBuffer::kBlockPoints - 1) * render::kVerticesPerSegment);
// Forme ronde : premier segment après le disque de départ ; disque d'arrivée en fin de bloc.
constexpr std::size_t kSlots = render::kRoundInkCapVertices;
constexpr std::size_t kPerSegment = render::kRoundInkVerticesPerSegment;
constexpr std::size_t kEndCap = kSlots + (render::InkBuffer::kBlockPoints - 1) * kPerSegment;
constexpr render::Vertex2D kPoison{-1000.0f, -1000.0f};

bool same(const render::Vertex2D& a, const render::Vertex2D& b)
{
    return a.x == b.x && a.y == b.y;
}

QSGGeometryNode* blockNode(QSGNode& root, int index)
{
    return static_cast<QSGGeometryNode*>(root.childAtIndex(index));
}

std::span<render::Vertex2D> verticesOf(QSGGeometryNode* node)
{
    QSGGeometry* geometry = node->geometry();
    return {static_cast<render::Vertex2D*>(geometry->vertexData()),
            static_cast<std::size_t>(geometry->vertexCount())};
}

core::PointF pointOf(const core::StrokePoint& p)
{
    return {p.x, p.y};
}

// Sommets du segment from → from + 1. Amendé par L3-T11 : la référence n'est plus le trait figé du prototype
// (appendStrokeTriangles) mais appendRoundInkSegment (forme ronde, D2) ; `settled` : le point from + 2 est
// arrivé (sinon segment provisoire, droit).
std::vector<render::Vertex2D> segment(int from, double baseWidth, bool settled)
{
    const core::PointF next = pointOf(pt(from + 2));
    std::vector<render::Vertex2D> out;
    render::appendRoundInkSegment(pointOf(pt(from)), pointOf(pt(from + 1)), settled ? &next : nullptr,
                                  from == 0, baseWidth, out);
    return out;
}

// Chemin de référence de l'encre vive ronde : un segment appendRoundInkSegment par paire de points, bout à
// bout, le dernier provisoire.
std::vector<render::Vertex2D> strokeSegments(const std::vector<core::StrokePoint>& all, double baseWidth)
{
    std::vector<render::Vertex2D> out;
    for (std::size_t j = 0; j + 1 < all.size(); ++j) {
        const core::PointF next = j + 2 < all.size() ? pointOf(all[j + 2]) : core::PointF{};
        render::appendRoundInkSegment(pointOf(all[j]), pointOf(all[j + 1]),
                                      j + 2 < all.size() ? &next : nullptr, j == 0, baseWidth, out);
    }
    return out;
}

std::vector<render::Vertex2D> capAt(const core::StrokePoint& p, double baseWidth)
{
    std::vector<render::Vertex2D> out;
    render::appendRoundInkCap(pointOf(p), baseWidth, out);
    return out;
}

std::vector<render::Vertex2D> copyOf(std::span<const render::Vertex2D> vertices)
{
    return {vertices.begin(), vertices.end()};
}

bool allEqual(std::span<const render::Vertex2D> vertices, const render::Vertex2D& value)
{
    for (const auto& v : vertices) {
        if (!same(v, value)) {
            return false;
        }
    }
    return true;
}

bool allPoison(std::span<const render::Vertex2D> vertices)
{
    return allEqual(vertices, kPoison);
}

render::Vertex2D vertexAt(const core::StrokePoint& p)
{
    return {static_cast<float>(p.x), static_cast<float>(p.y)};
}

std::size_t writtenVertices(const render::InkBuffer& buffer, int block)
{
    return (buffer.block(static_cast<std::size_t>(block)).size() - 1) * kPerSegment;
}

// Sommets écrits des blocs mis bout à bout, place libre exclue. `root` a autant d'enfants que de blocs.
std::vector<render::Vertex2D> drawnVertices(QSGNode& root, const render::InkBuffer& buffer)
{
    std::vector<render::Vertex2D> drawn;
    for (int b = 0; b < root.childCount(); ++b) {
        const auto vertices = verticesOf(blockNode(root, b));
        // Disques exclus (L3-T11). Pas de variable « slots » : c'est une macro de Qt.
        const auto segments = vertices.subspan(kSlots, writtenVertices(buffer, b));
        drawn.insert(drawn.end(), segments.begin(), segments.end());
    }
    return drawn;
}

bool sameVertices(const std::vector<render::Vertex2D>& a, const std::vector<render::Vertex2D>& b)
{
    if (a.size() != b.size()) {
        return false;
    }
    for (std::size_t v = 0; v < a.size(); ++v) {
        if (!same(a[v], b[v])) {
            return false;
        }
    }
    return true;
}

class SyncProbe : public core::InkProbe
{
public:
    explicit SyncProbe(const std::atomic_bool& syncing) : m_syncing(syncing) {}

    void sampleReceived(std::uint64_t, std::int64_t) override {}
    void samplesSynced(std::uint64_t lastSeq, std::int64_t) override
    {
        std::lock_guard lock(m_mutex);
        m_synced.push_back(lastSeq);
        m_allDuringSync = m_allDuringSync && m_syncing.load();
    }
    std::vector<std::uint64_t> snapshot()
    {
        std::lock_guard lock(m_mutex);
        return m_synced;
    }
    bool allDuringSync()
    {
        std::lock_guard lock(m_mutex);
        return m_allDuringSync;
    }

private:
    const std::atomic_bool& m_syncing;
    std::mutex m_mutex;
    std::vector<std::uint64_t> m_synced;
    bool m_allDuringSync = true;
};

} // namespace

class TstRenderInkBuffer : public QObject
{
    Q_OBJECT

private slots:
    void thousandPointsMakeFourBlocks()
    {
        render::InkBuffer buffer;
        buffer.begin(0xFF000000u, 3.0);
        for (int i = 0; i < 1000; ++i) {
            buffer.add(pt(i));
        }
        QCOMPARE(buffer.pointCount(), std::size_t{1000});
        QCOMPARE(buffer.blockCount(), std::size_t{4}); // 256 + 255 + 255 + 234
        QCOMPARE(buffer.block(0).size(), render::InkBuffer::kBlockPoints);
        QCOMPARE(buffer.block(3).size(), std::size_t{235}); // jonction + 234
    }

    void blocksShareTheirJoint()
    {
        render::InkBuffer buffer;
        buffer.begin(0xFF000000u, 3.0);
        for (int i = 0; i < 600; ++i) {
            buffer.add(pt(i));
        }
        const auto b0 = buffer.block(0);
        const auto b1 = buffer.block(1);
        QCOMPARE(b1.front().x, b0.back().x);
        QCOMPARE(b1.front().y, b0.back().y);
        QCOMPARE(b1[1].x, 256.0);
    }

    void onlyTheLastBlockIsDirtyAfterAnAdd()
    {
        render::InkBuffer buffer;
        buffer.begin(0xFF000000u, 3.0);
        for (int i = 0; i < 700; ++i) {
            buffer.add(pt(i));
        }
        QCOMPARE(buffer.firstDirtyBlock(), std::size_t{0});
        buffer.markClean();
        QCOMPARE(buffer.firstDirtyBlock(), buffer.blockCount());
        buffer.add(pt(700));
        QCOMPARE(buffer.firstDirtyBlock(), buffer.blockCount() - 1);
    }

    void newBlockIsDirtyFromItsIndex()
    {
        render::InkBuffer buffer;
        buffer.begin(0xFF000000u, 3.0);
        for (int i = 0; i < 256; ++i) {
            buffer.add(pt(i));
        }
        buffer.markClean();
        buffer.add(pt(256)); // ouvre le bloc 1
        QCOMPARE(buffer.blockCount(), std::size_t{2});
        QCOMPARE(buffer.firstDirtyBlock(), std::size_t{1});
    }

    void clearEmptiesAndFlags()
    {
        render::InkBuffer buffer;
        buffer.begin(0xFF000000u, 3.0);
        buffer.add(pt(0));
        buffer.add(pt(1));
        buffer.markClean();
        buffer.clear();
        QVERIFY(buffer.empty());
        QVERIFY(buffer.clearedSinceClean());
        buffer.markClean();
        QVERIFY(!buffer.clearedSinceClean());
    }

    void blockBoundsCoverTheBlock()
    {
        render::InkBuffer buffer;
        buffer.begin(0xFF000000u, 4.0);
        buffer.add({10, 10, 1.0f});
        buffer.add({30, 20, 0.5f});
        const core::RectF r = buffer.blockBounds(0);
        QVERIFY(r.x <= 7.0 && r.y <= 7.0); // demi-largeur max 3
        QVERIFY(r.x + r.width >= 33.0 && r.y + r.height >= 23.0);
    }

    // Chemin RHI au niveau des nœuds : hors écran, Qt prend toujours le rendu logiciel et n'y passe jamais.
    // Les blocs mis bout à bout donnent exactement les segments de référence (amendé par L3-T11 : forme
    // ronde, strokeSegments ; prototype : rhiPrototypeBlocksMatchTheSquareSegments) : ni trou ni doublon aux
    // jonctions, y compris quand une image tombe au milieu d'un bloc ou à cheval sur deux blocs.
    void rhiBlocksMatchTheFrozenStrokeGeometry()
    {
        render::InkBuffer buffer;
        buffer.begin(0xFF000000u, 3.0);
        QSGNode root;
        std::vector<core::StrokePoint> all;
        for (int i = 0; i < 700; ++i) {
            all.push_back(pt(i));
            buffer.add(all.back());
            if (i % 50 == 49) { // une image tous les 50 points
                render::InkItem::syncRhiBlocks(&root, buffer);
                buffer.markClean();
            }
        }
        render::InkItem::syncRhiBlocks(&root, buffer);
        buffer.markClean();

        QCOMPARE(root.childCount(), 3);
        for (int b = 0; b < root.childCount(); ++b) {
            QSGGeometryNode* node = blockNode(root, b);
            QCOMPARE(node->geometry()->vertexCount(), kCapacity);
            QCOMPARE(node->geometry()->vertexDataPattern(), QSGGeometry::DynamicPattern);
            QCOMPARE(node->geometry()->drawingMode(), QSGGeometry::DrawTriangles);
        }
        const std::vector<render::Vertex2D> expected = strokeSegments(all, 3.0);
        QVERIFY2(sameVertices(drawnVertices(root, buffer), expected), "blocs ≠ segments de référence");
        // Disques (L3-T11) : départ dans le bloc 0, arrivée sur le dernier point dans le dernier bloc ;
        // ailleurs, place libre.
        QVERIFY2(sameVertices(copyOf(verticesOf(blockNode(root, 0)).first(kSlots)), capAt(all.front(), 3.0)),
                 "disque de départ");
        QVERIFY2(
            sameVertices(copyOf(verticesOf(blockNode(root, 2)).subspan(kEndCap)), capAt(all.back(), 3.0)),
            "disque d'arrivée");
        for (int b = 0; b < 2; ++b) {
            QVERIFY2(allEqual(verticesOf(blockNode(root, b)).subspan(kEndCap),
                              vertexAt(buffer.block(static_cast<std::size_t>(b)).front())),
                     "disque d'arrivée hors du dernier bloc");
        }
        // Place libre du dernier bloc : triangles dégénérés posés sur le premier point du bloc (510, 6), pas
        // à l'origine, qui étendrait les bornes du nœud jusqu'à (0, 0).
        const std::size_t used = kSlots + writtenVertices(buffer, 2);
        const auto tail = verticesOf(blockNode(root, 2)).subspan(used, kEndCap - used);
        QVERIFY(!tail.empty());
        QVERIFY2(allEqual(tail, vertexAt(buffer.block(2).front())), "place libre hors du bloc");
    }

    // Prototype (bout carré) : les blocs mis bout à bout donnent la géométrie du trait figé du prototype
    // (StrokeRenderer → appendStrokeTriangles), 6 sommets par segment. Ajouté par L3-T11 : c'était la
    // référence de rhiBlocksMatchTheFrozenStrokeGeometry avant la forme ronde.
    void rhiPrototypeBlocksMatchTheSquareSegments()
    {
        render::InkBuffer buffer;
        buffer.begin(0xFF000000u, 3.0);
        QSGNode root;
        std::vector<core::StrokePoint> all;
        for (int i = 0; i < 700; ++i) {
            all.push_back(pt(i));
            buffer.add(all.back());
            if (i % 50 == 49) {
                render::InkItem::syncRhiBlocks(&root, buffer, core::PenCap::Square);
                buffer.markClean();
            }
        }
        render::InkItem::syncRhiBlocks(&root, buffer, core::PenCap::Square);
        buffer.markClean();
        QCOMPARE(root.childCount(), 3);
        std::vector<render::Vertex2D> drawn;
        for (int b = 0; b < 3; ++b) {
            QCOMPARE(blockNode(root, b)->geometry()->vertexCount(), kSquareCapacity);
            const auto vertices = verticesOf(blockNode(root, b));
            const std::size_t written =
                (buffer.block(static_cast<std::size_t>(b)).size() - 1) * render::kVerticesPerSegment;
            drawn.insert(drawn.end(), vertices.begin(),
                         vertices.begin() + static_cast<std::ptrdiff_t>(written));
        }
        std::vector<render::Vertex2D> expected;
        render::appendStrokeTriangles(all, 3.0, expected);
        QVERIFY2(sameVertices(drawn, expected), "blocs ≠ trait figé du prototype");
    }

    // Matériau des blocs (revue T9, mesuré en D3D11) : le moteur de rendu par lots range dans un même lot les
    // nœuds dont les matériaux ont le même type() et compare() == 0, et renvoie tout le lot quand l'un
    // change. Avec un QSGFlatColorMaterial commun, chaque point renvoyait les 4 blocs d'un trait de 1000
    // points. Hors écran, seul le critère se vérifie : deux blocs ne sont jamais égaux.
    void rhiBlockMaterialsNeverShareABatch()
    {
        render::InkBuffer buffer;
        buffer.begin(0xFF102030u, 3.0);
        for (int i = 0; i < 600; ++i) {
            buffer.add(pt(i));
        }
        QSGNode root;
        render::InkItem::syncRhiBlocks(&root, buffer);
        QCOMPARE(root.childCount(), 3);
        QSGMaterial* m0 = blockNode(root, 0)->material();
        QSGMaterial* m1 = blockNode(root, 1)->material();
        QSGMaterial* m2 = blockNode(root, 2)->material();

        // Couleur unie du trait, même shader que QSGFlatColorMaterial…
        const auto* flat = dynamic_cast<const QSGFlatColorMaterial*>(m0);
        QVERIFY(flat);
        QCOMPARE(flat->color(), QColor::fromRgba(0xFF102030u));
        // … mais un type propre : le moteur ne compare jamais un bloc à un autre matériau de couleur unie.
        const QSGFlatColorMaterial plain;
        QVERIFY(m0->type() == m1->type() && m1->type() == m2->type());
        QVERIFY(m0->type() != plain.type());
        // Deux blocs ne sont jamais égaux : un lot par bloc, seul le bloc modifié est renvoyé.
        QVERIFY(m0->compare(m1) != 0);
        QVERIFY(m1->compare(m2) != 0);
        QVERIFY(m0->compare(m2) != 0);
        QVERIFY((m0->compare(m1) < 0) == (m1->compare(m0) > 0)); // ordre cohérent
        QCOMPARE(m0->compare(m0), 0);
        // Même bloc, nœud recréé sous une autre racine : égal.
        QSGNode other;
        render::InkItem::syncRhiBlocks(&other, buffer);
        QCOMPARE(blockNode(other, 1)->material()->compare(m1), 0);
    }

    // Qt rappelle updatePaintNode(nullptr) sur un tampon déjà propre (réinsertion de l'élément, perte du
    // périphérique, releaseResources()) : les nœuds recréés doivent être remplis, sinon l'encre disparaît.
    void rhiRefillsTheBlocksOfAFreshRoot()
    {
        render::InkBuffer buffer;
        buffer.begin(0xFF000000u, 3.0);
        std::vector<core::StrokePoint> all;
        for (int i = 0; i < 700; ++i) {
            all.push_back(pt(i));
            buffer.add(all.back());
        }
        QSGNode first;
        render::InkItem::syncRhiBlocks(&first, buffer);
        buffer.markClean();

        QSGNode fresh;
        render::InkItem::syncRhiBlocks(&fresh, buffer);
        QCOMPARE(fresh.childCount(), 3);
        // Amendé par L3-T11 : référence de la forme ronde (strokeSegments).
        const std::vector<render::Vertex2D> expected = strokeSegments(all, 3.0);
        QVERIFY2(sameVertices(drawnVertices(fresh, buffer), expected), "blocs recréés vides");
    }

    // clear() puis nouveau trait : les nœuds de l'ancien trait disparaissent, le nouveau est écrit en entier,
    // dans sa couleur.
    void rhiClearThenNewStrokeReplacesTheBlocks()
    {
        render::InkBuffer buffer;
        buffer.begin(0xFF000000u, 3.0);
        for (int i = 0; i < 300; ++i) {
            buffer.add(pt(i));
        }
        QSGNode root;
        render::InkItem::syncRhiBlocks(&root, buffer);
        buffer.markClean();
        QCOMPARE(root.childCount(), 2);

        buffer.clear();
        buffer.begin(0xFFFF0000u, 5.0);
        std::vector<core::StrokePoint> next;
        for (int i = 0; i < 100; ++i) {
            next.push_back(pt(1000 + i));
            buffer.add(next.back());
        }
        render::InkItem::syncRhiBlocks(&root, buffer);
        buffer.markClean();

        QCOMPARE(root.childCount(), 1);
        // Amendé par L3-T11 : référence de la forme ronde (strokeSegments).
        const std::vector<render::Vertex2D> expected = strokeSegments(next, 5.0);
        QVERIFY2(sameVertices(drawnVertices(root, buffer), expected), "ancien trait encore dessiné");
        const auto* flat = dynamic_cast<const QSGFlatColorMaterial*>(blockNode(root, 0)->material());
        QVERIFY(flat);
        QCOMPARE(flat->color(), QColor(Qt::red));
    }

    // Le test qui garde l'invariant : après un point, ni réallocation, ni réécriture de ce qui est déjà
    // final. Tout ce qui a été écrit est empoisonné ; seuls le segment provisoire (réécrit, final), le
    // nouveau segment (provisoire) et le disque d'arrivée doivent apparaître, à leur place. Amendé par L3-T11
    // : forme ronde, le dernier segment dépend du point suivant (D2) ; avant, seul le nouveau segment était
    // écrit.
    void rhiWritesOnlyTheNewSegment()
    {
        render::InkBuffer buffer;
        buffer.begin(0xFF000000u, 3.0);
        QSGNode root;
        for (int i = 0; i < 700; ++i) {
            buffer.add(pt(i));
        }
        render::InkItem::syncRhiBlocks(&root, buffer);
        buffer.markClean();
        QCOMPARE(root.childCount(), 3);
        std::vector<const void*> storage;
        for (int b = 0; b < 3; ++b) {
            storage.push_back(blockNode(root, b)->geometry()->vertexData());
            for (auto& v : verticesOf(blockNode(root, b))) {
                v = kPoison;
            }
        }

        buffer.add(pt(700));
        render::InkItem::syncRhiBlocks(&root, buffer);
        buffer.markClean();

        QCOMPARE(root.childCount(), 3);
        for (int b = 0; b < 3; ++b) {
            QCOMPARE(blockNode(root, b)->geometry()->vertexData(), storage[static_cast<std::size_t>(b)]);
            QCOMPARE(blockNode(root, b)->geometry()->vertexCount(), kCapacity);
        }
        QVERIFY2(allPoison(verticesOf(blockNode(root, 0))), "bloc 0 réécrit");
        QVERIFY2(allPoison(verticesOf(blockNode(root, 1))), "bloc 1 réécrit");
        const auto last = verticesOf(blockNode(root, 2));
        const std::size_t newSegment = buffer.block(2).size() - 2; // 699 → 700, dernier segment du bloc
        const std::size_t settledAt =
            kSlots + (newSegment - 1) * kPerSegment; // 698 → 699, provisoire jusque-là
        const std::size_t at = settledAt + kPerSegment;
        QVERIFY2(allPoison(last.first(settledAt)), "segments finals réécrits");
        QVERIFY2(sameVertices(copyOf(last.subspan(settledAt, kPerSegment)), segment(698, 3.0, true)),
                 "segment provisoire non réécrit en final");
        QVERIFY2(sameVertices(copyOf(last.subspan(at, kPerSegment)), segment(699, 3.0, false)),
                 "nouveau segment absent ou mal placé");
        QVERIFY2(allPoison(last.subspan(at + kPerSegment, kEndCap - at - kPerSegment)),
                 "écriture hors des deux segments");
        QVERIFY2(sameVertices(copyOf(last.subspan(kEndCap)), capAt(pt(700), 3.0)), "disque d'arrivée absent");
    }

    // Ouvrir un bloc : le bloc plein reçoit son dernier segment, final (le point qui suit sa jonction vient
    // d'arriver), et perd son disque d'arrivée, une fois ; rien d'autre n'y est écrit, et plus rien ensuite.
    // Le premier segment du nouveau bloc part de la jonction. Amendé par L3-T11 : forme ronde (D2) ; avant,
    // le bloc plein n'était jamais touché, son dernier segment ne dépendant pas du point suivant.
    void rhiOpeningABlockLeavesTheFullOneUntouched()
    {
        render::InkBuffer buffer;
        buffer.begin(0xFF000000u, 3.0);
        QSGNode root;
        for (int i = 0; i < 256; ++i) {
            buffer.add(pt(i));
        }
        render::InkItem::syncRhiBlocks(&root, buffer);
        buffer.markClean();
        QCOMPARE(root.childCount(), 1);
        const void* fullStorage = blockNode(root, 0)->geometry()->vertexData();
        for (auto& v : verticesOf(blockNode(root, 0))) {
            v = kPoison;
        }

        buffer.add(pt(256));
        render::InkItem::syncRhiBlocks(&root, buffer);
        buffer.markClean();

        QCOMPARE(root.childCount(), 2);
        QCOMPARE(blockNode(root, 0)->geometry()->vertexData(), fullStorage);
        const auto full = verticesOf(blockNode(root, 0));
        const std::size_t lastAt = kSlots + 254 * kPerSegment; // 254 → 255
        QVERIFY2(allPoison(full.first(lastAt)), "segments finals du bloc plein réécrits");
        QVERIFY2(sameVertices(copyOf(full.subspan(lastAt, kPerSegment)), segment(254, 3.0, true)),
                 "dernier segment du bloc plein resté provisoire");
        QVERIFY2(allEqual(full.subspan(kEndCap), vertexAt(pt(0))),
                 "disque d'arrivée resté sur le bloc plein");
        const auto opened = verticesOf(blockNode(root, 1));
        QVERIFY2(allEqual(opened.first(kSlots), vertexAt(pt(255))), "disque de départ hors du bloc 0");
        QVERIFY2(sameVertices(copyOf(opened.subspan(kSlots, kPerSegment)), segment(255, 3.0, false)),
                 "segment de jonction absent");
        // Place libre du bloc ouvert : sur sa jonction (255, 3), pas à l'origine.
        QVERIFY2(
            allEqual(opened.subspan(kSlots + kPerSegment, kEndCap - kSlots - kPerSegment), vertexAt(pt(255))),
            "place libre hors du bloc");
        QVERIFY2(sameVertices(copyOf(opened.subspan(kEndCap)), capAt(pt(256), 3.0)),
                 "disque d'arrivée absent");

        // Ensuite, le bloc plein ne bouge plus.
        for (auto& v : verticesOf(blockNode(root, 0))) {
            v = kPoison;
        }
        buffer.add(pt(257));
        render::InkItem::syncRhiBlocks(&root, buffer);
        buffer.markClean();
        QVERIFY2(allPoison(verticesOf(blockNode(root, 0))), "bloc plein réécrit après sa jonction");
    }

    // La sonde reçoit, pendant la synchronisation, le numéro du dernier échantillon entré dans la géométrie :
    // une fois par image qui apporte des points, jamais à l'ajout, jamais sans point nouveau.
    void probeReportsLastSyncedSequence()
    {
        QQuickWindow window;
        window.resize(200, 200);
        auto* ink = new render::InkItem;
        ink->setSize(QSizeF(200, 200));
        ink->setParentItem(window.contentItem());
        std::atomic_bool syncing{false};
        QObject::connect(
            &window, &QQuickWindow::beforeSynchronizing, &window, [&syncing] { syncing = true; },
            Qt::DirectConnection);
        QObject::connect(
            &window, &QQuickWindow::afterSynchronizing, &window, [&syncing] { syncing = false; },
            Qt::DirectConnection);
        SyncProbe probe(syncing);
        ink->setProbe(&probe);
        window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        QSignalSpy swapped(&window, &QQuickWindow::frameSwapped);

        ink->beginStroke(0xFF000000u, 3.0);
        ink->addPoint({10, 10, 0.5f}, 7);
        ink->addPoint({20, 10, 0.5f}, 8);
        // Ni à l'ajout, ni aux images d'avant (exposition de la fenêtre) qui n'apportaient aucun point.
        QVERIFY2(probe.snapshot().empty(), "signalé avant la synchronisation des points 7 et 8");
        swapped.clear();
        QVERIFY(swapped.wait(2000));
        QTRY_VERIFY_WITH_TIMEOUT(!probe.snapshot().empty(), 2000);
        QCOMPARE(probe.snapshot().size(), std::size_t{1}); // une fois par image, pas une fois par point
        QCOMPARE(probe.snapshot().back(), std::uint64_t{8});
        QCOMPARE(ink->pointCount(), std::size_t{2});

        ink->update(); // synchronisation de l'élément sans nouveau point : pas de nouveau signalement
        swapped.clear();
        QVERIFY(swapped.wait(2000));
        QTest::qWait(50);
        QCOMPARE(probe.snapshot().size(), std::size_t{1});

        ink->addPoint({30, 10, 0.5f}, 9);
        ink->addPoint({40, 10, 0.5f}, 10);
        ink->addPoint({50, 10, 0.5f}, 11);
        swapped.clear();
        QVERIFY(swapped.wait(2000));
        QTRY_COMPARE_WITH_TIMEOUT(probe.snapshot().size(), std::size_t{2}, 2000);
        QCOMPARE(probe.snapshot().back(), std::uint64_t{11});
        QVERIFY2(probe.allDuringSync(), "signalé hors de la synchronisation");
    }
};

QTEST_MAIN(TstRenderInkBuffer)
#include "tst_render_inkbuffer.moc"
