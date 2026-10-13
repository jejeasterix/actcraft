#include "support/renderhost.h"

#include <actcraft/core/generator.h>
#include <actcraft/render/fixedpageitem.h>

#include <QSignalSpy>
#include <QTest>

using namespace actcraft;

namespace {
QColor argb(core::Argb c)
{
    return QColor::fromRgba(c);
}
} // namespace

class TstRenderFixedPage : public QObject
{
    Q_OBJECT

private:
    struct Fixture
    {
        QQuickWindow window;
        render::FixedPageItem* page = nullptr;
    };

    static void open(Fixture& f)
    {
        f.window.setColor(QColor(0x3c, 0x3f, 0x45));
        f.window.resize(1024, 768);
        f.page = new render::FixedPageItem;
        f.page->setSize(QSizeF(1024, 768));
        f.page->setParentItem(f.window.contentItem());
        f.window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&f.window));
    }

    // Rendu logiciel de Qt 6.12 : un nœud ajouté sous une racine QSGNode déjà rendue perd la transformation
    // de l'élément. Élément en (100, 50), échelle 2 : (x, y) de l'élément -> (100 + 2x, 50 + 2y) de la
    // fenêtre.
    static void openTransformed(Fixture& f)
    {
        f.window.setColor(QColor(0x3c, 0x3f, 0x45));
        f.window.resize(800, 600);
        f.page = new render::FixedPageItem;
        f.page->setSize(QSizeF(300, 200));
        f.page->setPosition(QPointF(100, 50));
        f.page->setTransformOrigin(QQuickItem::TopLeft);
        f.page->setScale(2.0);
        f.page->setParentItem(f.window.contentItem());
        f.window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&f.window));
    }

    static core::Stroke lineAt(double y)
    {
        core::Stroke stroke;
        stroke.baseWidth = 6;
        stroke.color = 0xFF000000u;
        stroke.points = {{10, y, 0.5f}, {60, y, 0.5f}, {110, y, 0.5f}};
        return stroke;
    }

    static void waitFrame(Fixture& f)
    {
        QSignalSpy swapped(&f.window, &QQuickWindow::frameSwapped);
        f.page->update();
        QVERIFY(swapped.wait(2000));
    }

    static void verifyTransformedStroke(Fixture& f, const char* label)
    {
        const QImage image = f.window.grabWindow();
        QVERIFY2(colorNear(image.pixelColor(220, 70), QColor(Qt::black), 40), label);
        QVERIFY2(colorNear(image.pixelColor(60, 10), f.window.color(), 1), label);
        QVERIFY2(colorNear(image.pixelColor(650, 400), QColor(Qt::white), 1), label);
        QVERIFY2(colorNear(image.pixelColor(50, 10), f.window.color(), 1), label);
    }

private slots:
    void strokeAppendedAfterFirstFrameKeepsTransform()
    {
        Fixture f;
        openTransformed(f);
        f.page->setPage(core::Page{});
        QTRY_VERIFY_WITH_TIMEOUT(f.page->isReady(), 2000);
        waitFrame(f);
        f.page->appendStroke(lineAt(10));
        verifyTransformedStroke(f, "trait ajouté après la première image");
    }

    void rebuildKeepsTransform()
    {
        Fixture f;
        openTransformed(f);
        f.page->setPage(core::Page{});
        QTRY_VERIFY_WITH_TIMEOUT(f.page->isReady(), 2000);
        waitFrame(f);
        core::Page page;
        page.objects.emplace_back(lineAt(10));
        f.page->setPage(std::move(page));
        QTRY_VERIFY_WITH_TIMEOUT(f.page->isReady(), 20000);
        verifyTransformedStroke(f, "reconstruction (setPage)");
        f.page->setSize(QSizeF(310, 200));
        f.page->setSize(QSizeF(300, 200));
        verifyTransformedStroke(f, "reconstruction (geometryChange)");
    }
    void generatedPageBecomesReadyWithWitnesses()
    {
        Fixture f;
        open(f);
        f.page->loadGeneratedPage();
        QTRY_VERIFY_WITH_TIMEOUT(f.page->isReady(), 20000);
        QCOMPARE(f.page->objectCount(), 300);
        const QImage image = f.window.grabWindow();
        const core::Witnesses& w = core::witnesses();
        const auto at = [&](core::PointF p) {
            return image.pixelColor(static_cast<int>(p.x), static_cast<int>(p.y));
        };
        QVERIFY(colorNear(at(w.shapeRect.center()), argb(w.shapeColor), 3));
        QVERIFY(colorNear(at(w.imageRect.center()), argb(w.imageCenterColor), 3));
        QVERIFY(colorNear(at(w.strokeProbe), argb(w.strokeColor), 10));
    }

    void appendedStrokeIsDrawnOnTop()
    {
        Fixture f;
        open(f);
        f.page->loadGeneratedPage();
        QTRY_VERIFY_WITH_TIMEOUT(f.page->isReady(), 20000);
        QSignalSpy appended(f.page, &render::FixedPageItem::strokeAppended);
        core::Stroke stroke;
        stroke.baseWidth = 6;
        stroke.color = 0xFF000000u;
        stroke.points = {{300, 690, 0.5f}, {400, 690, 0.5f}, {500, 690, 0.5f}};
        f.page->appendStroke(stroke);
        QCOMPARE(appended.count(), 1);
        QCOMPARE(f.page->objectCount(), 301);
        const QImage image = f.window.grabWindow();
        QVERIFY(colorNear(image.pixelColor(400, 690), QColor(Qt::black), 40));
    }

    void emptyPageIsWhite()
    {
        Fixture f;
        open(f);
        f.page->setPage(core::Page{});
        QTRY_VERIFY_WITH_TIMEOUT(f.page->isReady(), 2000);
        QCOMPARE(f.page->objectCount(), 0);
        const QImage image = f.window.grabWindow();
        QVERIFY(colorNear(image.pixelColor(512, 384), QColor(Qt::white), 1));
    }
};

QTEST_MAIN(TstRenderFixedPage)
#include "tst_render_fixedpage.moc"
