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

private slots:
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
