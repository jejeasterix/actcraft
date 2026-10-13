#include <actcraft/core/inkprobe.h>
#include <actcraft/render/inkbuffer.h>
#include <actcraft/render/inkitem.h>
#include <actcraft/render/strokegeometry.h>

#include <QQuickWindow>
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

bool allPoison(std::span<const render::Vertex2D> vertices)
{
    for (const auto& v : vertices) {
        if (!same(v, kPoison)) {
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
        std::vector<render::Vertex2D> expected;
        render::appendStrokeTriangles(all, 3.0, expected);
        std::vector<render::Vertex2D> drawn;
        for (int b = 0; b < root.childCount(); ++b) {
            QSGGeometryNode* node = blockNode(root, b);
            QCOMPARE(node->geometry()->vertexCount(), kCapacity);
            QCOMPARE(node->geometry()->vertexDataPattern(), QSGGeometry::DynamicPattern);
            QCOMPARE(node->geometry()->drawingMode(), QSGGeometry::DrawTriangles);
            const std::size_t written =
                (buffer.block(static_cast<std::size_t>(b)).size() - 1) * render::kVerticesPerSegment;
            const auto vertices = verticesOf(node);
            drawn.insert(drawn.end(), vertices.begin(),
                         vertices.begin() + static_cast<std::ptrdiff_t>(written));
            if (b == root.childCount() - 1) {
                // Place libre du dernier bloc : triangles dégénérés (sommets tous confondus), rien de
                // visible.
                for (std::size_t v = written; v < vertices.size(); ++v) {
                    QVERIFY(same(vertices[v], vertices[written]));
                }
            }
        }
        QCOMPARE(drawn.size(), expected.size());
        for (std::size_t v = 0; v < expected.size(); ++v) {
            QVERIFY2(same(drawn[v], expected[v]), qPrintable(QStringLiteral("sommet %1").arg(v)));
        }
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
