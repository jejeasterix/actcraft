#include <actcraft/ui/pageview.h>

#include <QPointingDevice>
#include <QQuickWindow>
#include <QSignalSpy>
#include <QTest>
#include <QWheelEvent>

using namespace actcraft::ui;

namespace {

struct Fixture
{
    QQuickWindow window;
    PageView* view = nullptr;
    QQuickItem* content = nullptr;
};

bool open(Fixture& f)
{
    f.window.resize(1280, 800);
    f.view = new PageView;
    f.view->setParentItem(f.window.contentItem());
    f.view->setSize(QSizeF(1280, 800));
    f.content = new QQuickItem;
    f.content->setSize(QSizeF(1024, 768));
    f.content->setParentItem(f.view);
    f.view->setContent(f.content);
    f.window.show();
    if (!QTest::qWaitForWindowExposed(&f.window)) {
        return false;
    }
    // Pas de QML ici : le test termine lui-même les redessins demandés à l'ouverture (chaque fin peut en
    // relancer un, d'où la boucle bornée).
    for (int i = 0; i < 5 && !f.view->cacheReady(); ++i) {
        f.view->notifyCacheUpdated();
    }
    return f.view->cacheReady();
}

void wheel(Fixture& f, QPointF pos, QPoint angle, Qt::KeyboardModifiers modifiers,
           const QPointingDevice* device = QPointingDevice::primaryPointingDevice())
{
    QWheelEvent event(pos, f.window.mapToGlobal(pos), QPoint(), angle, Qt::NoButton, modifiers,
                      Qt::NoScrollPhase, false, Qt::MouseEventNotSynthesized, device);
    QCoreApplication::sendEvent(&f.window, &event);
}

} // namespace

class TstPageView : public QObject
{
    Q_OBJECT

private slots:
    void startsInFitMode()
    {
        Fixture f;
        QVERIFY(open(f));
        QVERIFY(qFuzzyCompare(f.view->zoom(), (800.0 - 64.0) / 768.0));
        QVERIFY(qFuzzyCompare(f.content->scale(), f.view->zoom()));
        QVERIFY(qFuzzyCompare(f.content->x(), (1280.0 - 1024.0 * f.view->zoom()) / 2));
    }

    void ctrlWheelZoomsAtCursor()
    {
        Fixture f;
        QVERIFY(open(f));
        f.view->setZoom(2.0);
        const QPointF anchor(300, 200);
        const QPointF before = f.view->viewTransform().viewToPage(anchor);
        wheel(f, anchor, QPoint(0, 120), Qt::ControlModifier);
        QVERIFY(qFuzzyCompare(f.view->zoom(), 2.0 * 1.15));
        const QPointF after = f.view->viewTransform().viewToPage(anchor);
        QVERIFY(qAbs(after.x() - before.x()) < 0.01 && qAbs(after.y() - before.y()) < 0.01);
    }

    void zoomStaysWithinLimits()
    {
        Fixture f;
        QVERIFY(open(f));
        for (int i = 0; i < 40; ++i) {
            wheel(f, QPointF(640, 400), QPoint(0, 120), Qt::ControlModifier);
        }
        QCOMPARE(f.view->zoom(), 8.0);
        for (int i = 0; i < 80; ++i) {
            wheel(f, QPointF(640, 400), QPoint(0, -120), Qt::ControlModifier);
        }
        QCOMPARE(f.view->zoom(), 0.25);
    }

    void wheelScrollsVerticallyThenHorizontallyWithShift()
    {
        Fixture f;
        QVERIFY(open(f));
        f.view->setZoom(4.0);
        f.view->setPan(QPointF(32, 32));
        QCOMPARE(f.view->pan(), QPointF(32, 32));
        wheel(f, QPointF(640, 400), QPoint(0, -120), Qt::NoModifier);
        QCOMPARE(f.view->pan(), QPointF(32, -28));
        wheel(f, QPointF(640, 400), QPoint(0, -120), Qt::ShiftModifier);
        QCOMPARE(f.view->pan(), QPointF(-28, -28));
    }

    void zoomGestureRefreshesCacheAfterDelay()
    {
        Fixture f;
        QVERIFY(open(f));
        QSignalSpy refresh(f.view, &PageView::cacheRefreshRequested);
        f.view->setZoom(2.0); // zoom imposé : tout de suite
        QCOMPARE(refresh.count(), 1);
        f.view->notifyCacheUpdated();
        QVERIFY(f.view->cacheReady());
        refresh.clear();
        wheel(f, QPointF(640, 400), QPoint(0, 120), Qt::ControlModifier);
        QCOMPARE(refresh.count(), 0);
        QVERIFY(!f.view->cacheReady());
        QTRY_COMPARE_WITH_TIMEOUT(refresh.count(), 1, 1000);
        QCOMPARE(f.view->cacheTextureSize(),
                 cacheTextureSize(QSizeF(1024, 768), f.view->zoom(), f.window.effectiveDevicePixelRatio()));
    }

    void overlappingRequestsWaitForTheLastOne()
    {
        Fixture f;
        QVERIFY(open(f));
        QSignalSpy refresh(f.view, &PageView::cacheRefreshRequested);
        f.view->requestCacheRefresh();
        f.view->requestCacheRefresh(); // pendant le premier redessin : émise tout de suite quand même
        QCOMPARE(refresh.count(), 2);
        f.view->notifyCacheUpdated(); // fin d'un redessin peut-être antérieur à la seconde demande
        QVERIFY(!f.view->cacheReady());
        QCOMPARE(refresh.count(), 3); // un redessin de plus
        f.view->notifyCacheUpdated();
        QVERIFY(f.view->cacheReady());
        QCOMPARE(refresh.count(), 3);
    }

    void cacheUpdatedSignalMakesReady()
    {
        Fixture f;
        QVERIFY(open(f));
        QSignalSpy updated(f.view, &PageView::cacheUpdated);
        f.view->requestCacheRefresh();
        QVERIFY(!f.view->cacheReady());
        f.view->notifyCacheUpdated();
        QVERIFY(f.view->cacheReady());
        QCOMPARE(updated.count(), 1);
    }

    void withoutCacheReadyAtOnce()
    {
        Fixture f;
        QVERIFY(open(f));
        f.view->setCacheEnabled(false);
        QSignalSpy refresh(f.view, &PageView::cacheRefreshRequested);
        QSignalSpy updated(f.view, &PageView::cacheUpdated);
        f.view->setZoom(3.0);
        QCOMPARE(refresh.count(), 0);
        QCOMPARE(updated.count(), 1);
        QVERIFY(f.view->cacheReady());
    }

    void exclusiveDevicesFilterTheWheel()
    {
        Fixture f;
        QVERIFY(open(f));
        QPointingDevice synthetic(QStringLiteral("test-wheel"), 991, QInputDevice::DeviceType::Mouse,
                                  QPointingDevice::PointerType::Generic, QInputDevice::Capability::Position,
                                  1, 3);
        f.view->setExclusiveDevices({&synthetic});
        const qreal before = f.view->zoom();
        wheel(f, QPointF(640, 400), QPoint(0, 120), Qt::ControlModifier);
        QCOMPARE(f.view->zoom(), before);
        wheel(f, QPointF(640, 400), QPoint(0, 120), Qt::ControlModifier, &synthetic);
        QVERIFY(f.view->zoom() > before);
        f.view->setExclusiveDevices({});
    }
};

QTEST_MAIN(TstPageView)
#include "tst_pageview.moc"
