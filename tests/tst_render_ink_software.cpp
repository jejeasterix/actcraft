#include "support/renderhost.h"

#include <actcraft/render/inkitem.h>

#include <QSGRenderNode>
#include <QSignalSpy>
#include <QTest>

#include <atomic>

using namespace actcraft;

namespace {

// 10 rangées de 100 points : x de 50 à 941, y = 100 + 50 × rangée.
core::StrokePoint pointAt(int i)
{
    return {50.0 + 9.0 * (i % 100), 100.0 + 50.0 * (i / 100), 0.5f};
}

QPoint midSegment(int i)
{
    const core::StrokePoint a = pointAt(i);
    const core::StrokePoint b = pointAt(i + 1);
    return QPoint(static_cast<int>((a.x + b.x) / 2), static_cast<int>((a.y + b.y) / 2));
}

// Nœud espion du rendu logiciel : ne peint rien, compte ses passages dans render(). Le moteur logiciel ne
// rappelle render() que si la zone repeinte de l'image touche son rect() : il compte les images qui
// repeignent sa zone.
class SpyNode final : public QSGRenderNode
{
public:
    SpyNode(QRectF rect, std::atomic_int* renders) : m_rect(rect), m_renders(renders) {}

    void render(const RenderState*) override { ++*m_renders; }
    StateFlags changedStates() const override { return {}; }
    RenderingFlags flags() const override { return BoundedRectRendering; }
    QRectF rect() const override { return m_rect; }

private:
    QRectF m_rect;
    std::atomic_int* m_renders;
};

class SpyItem : public QQuickItem
{
public:
    explicit SpyItem(QRectF rect) : m_rect(rect) { setFlag(ItemHasContents); }

    std::atomic_int renders{0};

protected:
    QSGNode* updatePaintNode(QSGNode* old, UpdatePaintNodeData*) override
    {
        return old ? old : new SpyNode(m_rect, &renders);
    }

private:
    QRectF m_rect;
};

} // namespace

class TstRenderInkSoftware : public QObject
{
    Q_OBJECT

private slots:
    void firstSegmentSurvivesFourBlocks()
    {
        QQuickWindow window;
        window.setColor(Qt::white);
        window.resize(1024, 768);
        auto* ink = new render::InkItem;
        ink->setSize(QSizeF(1024, 768));
        ink->setParentItem(window.contentItem());
        window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));

        ink->beginStroke(0xFF000000u, 4.0);
        QSignalSpy swapped(&window, &QQuickWindow::frameSwapped);
        for (int i = 0; i < 1000; ++i) {
            ink->addPoint(pointAt(i), static_cast<std::uint64_t>(i));
            if (i % 100 == 99) {
                swapped.clear();
                QVERIFY(swapped.wait(2000)); // une image entre deux paquets : mise à jour incrémentale
            }
        }
        QCOMPARE(ink->blockCount(), std::size_t{4});
        const QImage image = window.grabWindow();
        QVERIFY2(colorNear(image.pixelColor(midSegment(0)), QColor(Qt::black), 60), "premier segment");
        QVERIFY2(colorNear(image.pixelColor(midSegment(255)), QColor(Qt::black), 60), "jonction 0-1");
        QVERIFY2(colorNear(image.pixelColor(midSegment(510)), QColor(Qt::black), 60), "jonction 1-2");
        QVERIFY2(colorNear(image.pixelColor(midSegment(997)), QColor(Qt::black), 60), "dernier segment");
        QVERIFY(colorNear(image.pixelColor(QPoint(1000, 700)), QColor(Qt::white), 1));
    }

    // Le coût d'un point ne dépend pas de la longueur du trait (décision 9, DA5) : un bloc figé dont le
    // rect() ne touche pas celui du dernier bloc n'est plus repeint (le bloc précédent l'est encore, par la
    // jonction : voir crossingStrokesPaintOnlyNearTheRepaintedArea). Sous un nœud unique pour tout le trait
    // (patron de StrokeRenderer), chaque image repeint toute la zone du trait et ce qu'elle recouvre :
    // l'espion posé sous le premier segment serait repeint à chaque point.
    void appendingLeavesDistantFrozenBlocksAlone()
    {
        QQuickWindow window;
        window.setColor(Qt::white);
        window.resize(1024, 768);
        // Sous l'encre, comme la page fixe : autour du premier segment, hors des blocs 1 à 3.
        auto* spy = new SpyItem(QRectF(40, 90, 30, 20));
        spy->setSize(QSizeF(1024, 768));
        spy->setParentItem(window.contentItem());
        auto* ink = new render::InkItem;
        ink->setSize(QSizeF(1024, 768));
        ink->setParentItem(window.contentItem());
        window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        QSignalSpy swapped(&window, &QQuickWindow::frameSwapped);

        ink->beginStroke(0xFF000000u, 4.0);
        spy->renders = 0;
        for (int i = 0; i < 1000; ++i) {
            ink->addPoint(pointAt(i), static_cast<std::uint64_t>(i));
            if (i % 100 == 99) {
                swapped.clear();
                QVERIFY(swapped.wait(2000));
                if (i == 99) {
                    // Témoin : l'espion voit bien la zone repeinte d'un bloc qui le recouvre.
                    QVERIFY2(spy->renders > 0, "l'espion n'a pas vu le bloc 0 se peindre");
                }
            }
        }
        QCOMPARE(ink->blockCount(), std::size_t{4});

        spy->renders = 0;
        for (int i = 1000; i < 1010; i += 2) {
            ink->addPoint(pointAt(i), static_cast<std::uint64_t>(i));
            ink->addPoint(pointAt(i + 1), static_cast<std::uint64_t>(i + 1));
            swapped.clear();
            QVERIFY(swapped.wait(2000));
        }
        QCOMPARE(ink->blockCount(), std::size_t{4});
        QCOMPARE(spy->renders.load(), 0);
        QVERIFY2(colorNear(window.grabWindow().pixelColor(midSegment(1005)), QColor(Qt::black), 60),
                 "points ajoutés");
    }

    // Traits qui se recroisent : le moteur rappelle chaque bloc figé dont le rect() chevauche la zone
    // repeinte, ici tous. Seuls leurs segments qui touchent cette zone sont peints : le travail d'une image
    // reste sous un bloc, quelle que soit la longueur du trait.
    void crossingStrokesPaintOnlyNearTheRepaintedArea()
    {
        QQuickWindow window;
        window.setColor(Qt::white);
        window.resize(1024, 768);
        auto* ink = new render::InkItem;
        ink->setSize(QSizeF(1024, 768));
        ink->setParentItem(window.contentItem());
        window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        QSignalSpy swapped(&window, &QQuickWindow::frameSwapped);

        // 17 allers-retours de 60 points, x de 100 à 900, y = 100 + 12 × passage, puis (500, 80) : 1021
        // points, 4 blocs pleins couvrant toute la bande.
        ink->beginStroke(0xFF000000u, 4.0);
        for (int i = 0; i < 1020; ++i) {
            const int pass = i / 60;
            const double along = 800.0 * (i % 60) / 59;
            ink->addPoint({pass % 2 == 0 ? 100 + along : 900 - along, 100.0 + 12 * pass, 0.5f},
                          static_cast<std::uint64_t>(i));
            if (i % 100 == 99) {
                swapped.clear();
                QVERIFY(swapped.wait(2000));
            }
        }
        ink->addPoint({500, 80, 0.5f}, 1020);
        swapped.clear();
        QVERIFY(swapped.wait(2000));
        QCOMPARE(ink->blockCount(), std::size_t{4});

        // Bloc 4 : une verticale en x = 500 qui recoupe les 17 passages.
        for (int j = 1; j <= 40; ++j) {
            ink->addPoint({500, 80 + 5.0 * j, 0.5f}, static_cast<std::uint64_t>(1020 + j));
        }
        swapped.clear();
        QVERIFY(swapped.wait(2000));
        QCOMPARE(ink->blockCount(), std::size_t{5});

        for (int k = 1; k <= 3; ++k) {
            const std::uint64_t before = render::InkItem::softwareSegmentsPainted();
            ink->addPoint({500, 280 + 5.0 * k, 0.5f}, static_cast<std::uint64_t>(1060 + k));
            swapped.clear();
            QVERIFY(swapped.wait(2000));
            const std::uint64_t painted = render::InkItem::softwareSegmentsPainted() - before;
            QVERIFY2(painted > 0, "rien de peint");
            QVERIFY2(painted <= render::InkBuffer::kBlockPoints,
                     qPrintable(QStringLiteral("%1 segments peints pour un point").arg(painted)));
        }
        // Les passages restent entiers de part et d'autre de la verticale, dans la zone repeinte.
        const QImage image = window.grabWindow();
        for (const int x : {496, 497, 502, 503}) {
            QVERIFY2(colorNear(image.pixelColor(x, 160), QColor(Qt::black), 60),
                     qPrintable(QStringLiteral("passage y = 160 coupé en x = %1").arg(x)));
        }
        QVERIFY(colorNear(image.pixelColor(500, 250), QColor(Qt::black), 60));
    }

    // Le moteur logiciel donne à un nœud la transformation de son parent : l'encre suit la position et
    // l'échelle de l'élément, y compris pour les blocs ouverts après la première image.
    void transformedItemPaintsAtItsPlace()
    {
        QQuickWindow window;
        window.setColor(Qt::white);
        window.resize(800, 600);
        auto* ink = new render::InkItem;
        ink->setSize(QSizeF(350, 250));
        ink->setPosition(QPointF(100, 50));
        ink->setTransformOrigin(QQuickItem::TopLeft);
        ink->setScale(2.0);
        ink->setParentItem(window.contentItem());
        window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        QSignalSpy swapped(&window, &QQuickWindow::frameSwapped);

        // y = 10, x = 10 + 0,5 × i : bloc 0 jusqu'à x = 137,5, bloc 1 au-delà, ouvert à la deuxième image.
        ink->beginStroke(0xFF000000u, 3.0);
        for (int i = 0; i < 300; ++i) {
            ink->addPoint({10 + 0.5 * i, 10, 0.5f}, static_cast<std::uint64_t>(i));
            if (i == 199 || i == 299) {
                swapped.clear();
                QVERIFY(swapped.wait(2000));
            }
        }
        QCOMPARE(ink->blockCount(), std::size_t{2});
        // Élément (x, 10) → fenêtre (100 + 2x, 70).
        const QImage image = window.grabWindow();
        QVERIFY2(colorNear(image.pixelColor(200, 70), QColor(Qt::black), 60), "bloc 0");
        QVERIFY2(colorNear(image.pixelColor(400, 70), QColor(Qt::black), 60), "bloc 1");
        QVERIFY2(colorNear(image.pixelColor(50, 10), QColor(Qt::white), 1), "bloc 0 sans transformation");
        QVERIFY2(colorNear(image.pixelColor(150, 10), QColor(Qt::white), 1), "bloc 1 sans transformation");
    }

    // blockBounds borne les points à la demi-largeur près ; le coin d'un bout carré en diagonale en sort de
    // 0,41 × demi-largeur. Le moteur logiciel découpe la peinture d'un nœud à son rect() : les coins seraient
    // rognés.
    void diagonalSquareCapsAreNotClipped()
    {
        QQuickWindow window;
        window.setColor(Qt::white);
        window.resize(400, 400);
        auto* ink = new render::InkItem;
        ink->setSize(QSizeF(400, 400));
        ink->setParentItem(window.contentItem());
        window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));

        // Largeur 40 (demi-largeur 20) : coins aux pointes (71.7, 100), (100, 71.7), (228.3, 200),
        // (200, 228.3) ; blockBounds s'arrête à 80 et à 220. Les sondes à 7 px des bornes exigent une marge
        // d'au moins 0,35 × demi-largeur.
        ink->beginStroke(0xFF000000u, 40.0);
        ink->addPoint({100, 100, 0.5f}, 0);
        ink->addPoint({200, 200, 0.5f}, 1);
        const QImage image = window.grabWindow();
        QVERIFY(colorNear(image.pixelColor(150, 150), QColor(Qt::black), 60));
        QVERIFY2(colorNear(image.pixelColor(73, 100), QColor(Qt::black), 60), "coin gauche");
        QVERIFY2(colorNear(image.pixelColor(100, 73), QColor(Qt::black), 60), "coin haut");
        QVERIFY2(colorNear(image.pixelColor(226, 200), QColor(Qt::black), 60), "coin droit");
        QVERIFY2(colorNear(image.pixelColor(200, 226), QColor(Qt::black), 60), "coin bas");
        QVERIFY(colorNear(image.pixelColor(66, 100), QColor(Qt::white), 1));
    }

    // Qt rappelle updatePaintNode(nullptr) quand l'élément revient dans une fenêtre : les nœuds recréés
    // repeignent le trait, même si le tampon est propre (aucun point nouveau).
    void reinsertedItemRedrawsItsInk()
    {
        QQuickWindow window;
        window.setColor(Qt::white);
        window.resize(400, 300);
        auto* ink = new render::InkItem;
        ink->setSize(QSizeF(400, 300));
        ink->setParentItem(window.contentItem());
        window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        ink->beginStroke(0xFF000000u, 6.0);
        ink->addPoint({50, 100, 0.5f}, 0);
        ink->addPoint({350, 100, 0.5f}, 1);
        QVERIFY(colorNear(window.grabWindow().pixelColor(200, 100), QColor(Qt::black), 60));

        ink->setParentItem(nullptr);
        QVERIFY(colorNear(window.grabWindow().pixelColor(200, 100), QColor(Qt::white), 1));
        ink->setParentItem(window.contentItem());
        QVERIFY(colorNear(window.grabWindow().pixelColor(200, 100), QColor(Qt::black), 60));
        delete ink;
    }

    void clearRemovesTheInk()
    {
        QQuickWindow window;
        window.setColor(Qt::white);
        window.resize(400, 300);
        auto* ink = new render::InkItem;
        ink->setSize(QSizeF(400, 300));
        ink->setParentItem(window.contentItem());
        window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        ink->beginStroke(0xFF000000u, 6.0);
        ink->addPoint({50, 100, 0.5f}, 0);
        ink->addPoint({350, 100, 0.5f}, 1);
        QVERIFY(colorNear(window.grabWindow().pixelColor(200, 100), QColor(Qt::black), 60));
        ink->clear();
        QCOMPARE(ink->pointCount(), std::size_t{0});
        QVERIFY(colorNear(window.grabWindow().pixelColor(200, 100), QColor(Qt::white), 1));
    }
};

QTEST_MAIN(TstRenderInkSoftware)
#include "tst_render_ink_software.moc"
