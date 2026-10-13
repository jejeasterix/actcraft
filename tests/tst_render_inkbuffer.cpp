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

// Capacité d'un bloc RHI : 255 segments × 6 sommets.
constexpr int kCapacity =
    static_cast<int>((render::InkBuffer::kBlockPoints - 1) * render::kVerticesPerSegment);
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

// Sommets d'un segment du trait figé (chemin de référence : StrokeRenderer → appendStrokeTriangles).
std::vector<render::Vertex2D> segment(int from, double baseWidth)
{
    const std::vector<core::StrokePoint> points{pt(from), pt(from + 1)};
    std::vector<render::Vertex2D> out;
    render::appendStrokeTriangles(points, baseWidth, out);
    return out;
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
    return (buffer.block(static_cast<std::size_t>(block)).size() - 1) * render::kVerticesPerSegment;
}

// Sommets écrits des blocs mis bout à bout, place libre exclue. `root` a autant d'enfants que de blocs.
std::vector<render::Vertex2D> drawnVertices(QSGNode& root, const render::InkBuffer& buffer)
{
    std::vector<render::Vertex2D> drawn;
    for (int b = 0; b < root.childCount(); ++b) {
        const auto vertices = verticesOf(blockNode(root, b));
        drawn.insert(drawn.end(), vertices.begin(),
                     vertices.begin() + static_cast<std::ptrdiff_t>(writtenVertices(buffer, b)));
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
    // Les blocs mis bout à bout donnent exactement la géométrie du trait figé : ni trou ni doublon aux
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
        std::vector<render::Vertex2D> expected;
        render::appendStrokeTriangles(all, 3.0, expected);
        QVERIFY2(sameVertices(drawnVertices(root, buffer), expected), "blocs ≠ trait figé");
        // Place libre du dernier bloc : triangles dégénérés posés sur le premier point du bloc (510, 6), pas
        // à l'origine, qui étendrait les bornes du nœud jusqu'à (0, 0).
        const auto tail = verticesOf(blockNode(root, 2)).subspan(writtenVertices(buffer, 2));
        QVERIFY(!tail.empty());
        QVERIFY2(allEqual(tail, vertexAt(buffer.block(2).front())), "place libre hors du bloc");
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
        std::vector<render::Vertex2D> expected;
        render::appendStrokeTriangles(all, 3.0, expected);
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
        std::vector<render::Vertex2D> expected;
        render::appendStrokeTriangles(next, 5.0, expected);
        QVERIFY2(sameVertices(drawnVertices(root, buffer), expected), "ancien trait encore dessiné");
        const auto* flat = dynamic_cast<const QSGFlatColorMaterial*>(blockNode(root, 0)->material());
        QVERIFY(flat);
        QCOMPARE(flat->color(), QColor(Qt::red));
    }

    // Le test qui garde l'invariant : après un point, ni réallocation, ni réécriture de ce qui est déjà
    // écrit. Tout ce qui a été écrit est empoisonné ; seul le nouveau segment doit apparaître, à sa place.
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
        const std::size_t at = newSegment * render::kVerticesPerSegment;
        QVERIFY2(allPoison(last.first(at)), "segments déjà écrits réécrits");
        const auto expected = segment(699, 3.0);
        for (std::size_t v = 0; v < render::kVerticesPerSegment; ++v) {
            QVERIFY2(same(last[at + v], expected[v]), "nouveau segment absent ou mal placé");
        }
        QVERIFY2(allPoison(last.subspan(at + render::kVerticesPerSegment)),
                 "écriture hors du nouveau segment");
    }

    // Ouvrir un bloc ne touche pas le bloc plein ; le premier segment du nouveau bloc part de la jonction.
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
        QVERIFY2(allPoison(verticesOf(blockNode(root, 0))), "bloc plein réécrit");
        const auto opened = verticesOf(blockNode(root, 1));
        const auto expected = segment(255, 3.0); // jonction 255 → 256
        for (std::size_t v = 0; v < render::kVerticesPerSegment; ++v) {
            QVERIFY2(same(opened[v], expected[v]), "segment de jonction absent");
        }
        // Place libre du bloc ouvert : sur sa jonction (255, 3), pas à l'origine.
        QVERIFY2(allEqual(opened.subspan(render::kVerticesPerSegment), vertexAt(pt(255))),
                 "place libre hors du bloc");
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
