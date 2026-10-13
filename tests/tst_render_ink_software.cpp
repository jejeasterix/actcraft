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

    // Le coût d'un point ne dépend pas de la longueur du trait (décision 9, DA5) : une fois le premier bloc
    // figé, les images suivantes ne repeignent plus sa zone. Sous un nœud unique pour tout le trait (patron
    // de StrokeRenderer), chaque image repeint toute la zone du trait et ce qu'elle recouvre : l'espion posé
    // sous le premier segment serait repeint à chaque point.
    void appendingRepaintsOnlyTheLastBlock()
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
        // (200, 228.3) ; blockBounds s'arrête à 80 et à 220.
        ink->beginStroke(0xFF000000u, 40.0);
        ink->addPoint({100, 100, 0.5f}, 0);
        ink->addPoint({200, 200, 0.5f}, 1);
        const QImage image = window.grabWindow();
        QVERIFY(colorNear(image.pixelColor(150, 150), QColor(Qt::black), 60));
        QVERIFY2(colorNear(image.pixelColor(76, 100), QColor(Qt::black), 60), "coin gauche");
        QVERIFY2(colorNear(image.pixelColor(100, 76), QColor(Qt::black), 60), "coin haut");
        QVERIFY2(colorNear(image.pixelColor(223, 200), QColor(Qt::black), 60), "coin droit");
        QVERIFY2(colorNear(image.pixelColor(200, 223), QColor(Qt::black), 60), "coin bas");
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
