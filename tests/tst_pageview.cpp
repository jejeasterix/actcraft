#include <actcraft/core/model.h>
#include <actcraft/render/fixedpageitem.h>
#include <actcraft/render/inkitem.h>
#include <actcraft/ui/pageview.h>

#include <QPointingDevice>
#include <QQuickWindow>
#include <QSignalSpy>
#include <QTabletEvent>
#include <QTest>
#include <QTouchEvent>
#include <QWheelEvent>

#include <memory>
#include <variant>

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

// Compte les événements de tablette que la vue laisse passer au reste de la fenêtre (barres, palettes). Posé
// sur la fenêtre avant la vue, il n'est consulté qu'après le filtre de celle-ci : ce qu'elle prend, il ne le
// voit pas.
class PassedOn : public QObject
{
public:
    int count = 0;

protected:
    bool eventFilter(QObject* watched, QEvent* event) override
    {
        const QEvent::Type type = event->type();
        if (type == QEvent::TabletPress || type == QEvent::TabletMove || type == QEvent::TabletRelease) {
            ++count;
        }
        return QObject::eventFilter(watched, event);
    }
};

// Vue de 600 × 600 posée en x = 200 dans une fenêtre de 1000 × 600 : à gauche et à droite, « hors de la vue »
// (barre, palette). Page vierge, sans cache.
struct DrawFixture
{
    QQuickWindow window;
    PassedOn passedOn;
    PageView* view = nullptr;
    actcraft::render::FixedPageItem* fixed = nullptr;
    QPointingDevice* stylus = nullptr;
};

bool openDrawing(DrawFixture& f, QPointingDevice* stylus)
{
    f.stylus = stylus;
    f.window.resize(1000, 600);
    f.window.installEventFilter(&f.passedOn); // avant la vue : consulté après son filtre
    f.view = new PageView;
    f.view->setParentItem(f.window.contentItem());
    f.view->setPosition(QPointF(200, 0));
    f.view->setSize(QSizeF(600, 600));
    auto* content = new QQuickItem;
    content->setSize(QSizeF(1024, 768));
    content->setParentItem(f.view);
    f.fixed = new actcraft::render::FixedPageItem;
    f.fixed->setSize(content->size());
    f.fixed->setParentItem(content);
    auto* ink = new actcraft::render::InkItem;
    ink->setSize(content->size());
    ink->setParentItem(content);
    f.view->setCacheEnabled(false);
    f.view->setContent(content);
    f.view->setFixedPage(f.fixed);
    f.view->setInk(ink);
    f.fixed->setPage(actcraft::core::Page{});
    f.window.show();
    return QTest::qWaitForWindowExposed(&f.window) &&
           QTest::qWaitFor([&] { return f.fixed->isReady(); }, 5000);
}

void tablet(DrawFixture& f, QEvent::Type type, QPointF scene, Qt::MouseButtons buttons)
{
    QTabletEvent event(type, f.stylus, scene, f.window.mapToGlobal(scene), buttons ? 0.5 : 0.0, 0, 0, 0, 0, 0,
                       Qt::NoModifier, Qt::LeftButton, buttons);
    QCoreApplication::sendEvent(&f.window, &event);
}

// Appui, quatre déplacements, relâché : six événements.
void tabletStroke(DrawFixture& f, QPointF from, QPointF to)
{
    tablet(f, QEvent::TabletPress, from, Qt::LeftButton);
    for (int i = 1; i <= 4; ++i) {
        tablet(f, QEvent::TabletMove, from + (to - from) * i / 4.0, Qt::LeftButton);
    }
    tablet(f, QEvent::TabletRelease, to, Qt::NoButton);
}

const actcraft::core::Stroke& lastStroke(const DrawFixture& f)
{
    return std::get<actcraft::core::Stroke>(f.fixed->page().objects.back());
}

} // namespace

class TstPageView : public QObject
{
    Q_OBJECT

private:
    std::unique_ptr<QPointingDevice> m_stylus;

private slots:
    void initTestCase()
    {
        m_stylus = std::make_unique<QPointingDevice>(
            QStringLiteral("tst-pageview-stylus"), 5160, QInputDevice::DeviceType::Stylus,
            QPointingDevice::PointerType::Pen,
            QInputDevice::Capability::Position | QInputDevice::Capability::Pressure, 1, 3);
    }

    void penColorAndWidthAreClampedAndSignalled()
    {
        DrawFixture f;
        QVERIFY(openDrawing(f, m_stylus.get()));
        QCOMPARE(f.view->penColor(), QColor(Qt::black));
        QCOMPARE(f.view->penWidth(), kDefaultPenWidth);
        QSignalSpy changed(f.view, &PageView::penChanged);
        f.view->setPenColor(QColor(QStringLiteral("#E53935")));
        f.view->setPenWidth(0.0);
        QCOMPARE(f.view->penWidth(), kMinPenWidth);
        f.view->setPenWidth(500.0);
        QCOMPARE(f.view->penWidth(), kMaxPenWidth);
        f.view->setPenColor(QColor()); // invalide : ignorée
        QCOMPARE(f.view->penColor(), QColor(QStringLiteral("#E53935")));
        QCOMPARE(changed.count(), 3);
        // Appliquées au stylo en place…
        tabletStroke(f, QPointF(300, 300), QPointF(500, 300));
        QTRY_COMPARE(f.fixed->objectCount(), 1);
        QCOMPARE(lastStroke(f).color, 0xFFE53935u);
        QCOMPARE(lastStroke(f).baseWidth, kMaxPenWidth);
        // … et gardées quand il est reconstruit (la page change).
        f.view->setFixedPage(nullptr);
        f.view->setFixedPage(f.fixed);
        tabletStroke(f, QPointF(300, 400), QPointF(500, 400));
        QTRY_COMPARE(f.fixed->objectCount(), 2);
        QCOMPARE(lastStroke(f).color, 0xFFE53935u);
        QCOMPARE(lastStroke(f).baseWidth, kMaxPenWidth);
    }

    void scrollPropertiesFollowZoom()
    {
        Fixture f;
        QVERIFY(open(f));
        QCOMPARE(f.view->scrollSizeX(), 1.0);
        QCOMPARE(f.view->scrollSizeY(), 1.0);
        QSignalSpy scrolled(f.view, &PageView::scrollChanged);
        f.view->setZoom(4.0);
        QVERIFY(scrolled.count() >= 1);
        QVERIFY(f.view->scrollSizeX() < 1.0);
        QVERIFY(f.view->scrollSizeY() < 1.0);
        f.view->scrollTo(0.0, 0.0);
        QCOMPARE(f.view->pan(), QPointF(kViewMargin, kViewMargin));
        QCOMPARE(f.view->scrollPositionX(), 0.0);
        f.view->scrollTo(1.0, 1.0); // borné à 1 − taille
        QVERIFY(qAbs(f.view->scrollPositionX() + f.view->scrollSizeX() - 1.0) < 1e-9);
    }

    void strokeStartsOnlyOnTheViewThenFollowsTheStylus()
    {
        DrawFixture f;
        QVERIFY(openDrawing(f, m_stylus.get()));
        // Appui hors de la vue (palette) : rien n'est pris, pas même les déplacements qui passent sur la vue.
        tabletStroke(f, QPointF(100, 300), QPointF(500, 300));
        QTest::qWait(50);
        QCOMPARE(f.fixed->objectCount(), 0);
        QCOMPARE(f.passedOn.count, 6);
        // Appui sur la vue, relâché hors de la vue : tout le trait est pris, jusqu'au relâché.
        tabletStroke(f, QPointF(300, 300), QPointF(900, 300));
        QTRY_COMPARE(f.fixed->objectCount(), 1);
        QCOMPARE(f.passedOn.count, 6);
        // Le trait suivi est celui du stylet : un doigt qui dessine sur la vue ne fait pas prendre le stylet
        // posé sur la palette.
        QPointingDevice* screen = QTest::createTouchDevice();
        QTest::touchEvent(&f.window, screen).press(0, QPoint(300, 100));
        QTest::touchEvent(&f.window, screen).move(0, QPoint(450, 100));
        tabletStroke(f, QPointF(100, 500), QPointF(150, 500));
        QCOMPARE(f.passedOn.count, 12);
        QTest::touchEvent(&f.window, screen).release(0, QPoint(450, 100));
        QTRY_COMPARE(f.fixed->objectCount(), 2);
        QTest::qWait(50);
        QCOMPARE(f.fixed->objectCount(), 2);
    }

    void blockedInputStartsNoStroke()
    {
        DrawFixture f;
        QVERIFY(openDrawing(f, m_stylus.get()));
        QSignalSpy blocked(f.view, &PageView::inputBlockedChanged);
        f.view->setInputBlocked(true);
        QCOMPARE(blocked.count(), 1);
        tabletStroke(f, QPointF(300, 300), QPointF(500, 300));
        QTest::qWait(50);
        QCOMPARE(f.fixed->objectCount(), 0);
        f.view->setInputBlocked(false);
        tabletStroke(f, QPointF(300, 300), QPointF(500, 300));
        QTRY_COMPARE(f.fixed->objectCount(), 1);
    }

    void oneFingerDrawsTwoFingersDoNot()
    {
        DrawFixture f;
        QVERIFY(openDrawing(f, m_stylus.get()));
        QPointingDevice* screen = QTest::createTouchDevice();
        QTest::touchEvent(&f.window, screen).press(0, QPoint(300, 200));
        QTest::touchEvent(&f.window, screen).move(0, QPoint(400, 200));
        QTest::touchEvent(&f.window, screen).move(0, QPoint(500, 200));
        QTest::touchEvent(&f.window, screen).release(0, QPoint(500, 200));
        QTRY_COMPARE(f.fixed->objectCount(), 1);
        // Deux doigts posés ensemble : un geste (lot 3), jamais un trait.
        QTest::touchEvent(&f.window, screen).press(0, QPoint(300, 400)).press(1, QPoint(600, 400));
        QTest::touchEvent(&f.window, screen).move(0, QPoint(350, 400)).move(1, QPoint(650, 400));
        QTest::touchEvent(&f.window, screen).release(0, QPoint(350, 400)).release(1, QPoint(650, 400));
        QTest::qWait(50);
        QCOMPARE(f.fixed->objectCount(), 1);
        // Un deuxième doigt posé pendant le trait du premier le termine (son début est gardé) ; ensuite, ni
        // le deuxième doigt, ni les deux ensemble, ni le premier resté seul ne dessinent.
        QTest::touchEvent(&f.window, screen).press(0, QPoint(300, 500));
        QTest::touchEvent(&f.window, screen).move(0, QPoint(350, 500));
        QTest::touchEvent(&f.window, screen).stationary(0).press(1, QPoint(600, 500));
        QTRY_COMPARE(f.fixed->objectCount(), 2);
        QTest::touchEvent(&f.window, screen).stationary(0).move(1, QPoint(650, 500));
        QTest::touchEvent(&f.window, screen).move(0, QPoint(400, 500)).move(1, QPoint(700, 500));
        QTest::touchEvent(&f.window, screen).stationary(0).release(1, QPoint(700, 500));
        QTest::touchEvent(&f.window, screen).move(0, QPoint(450, 500));
        QTest::touchEvent(&f.window, screen).release(0, QPoint(450, 500));
        QTest::qWait(50);
        QCOMPARE(f.fixed->objectCount(), 2);
        QCOMPARE(lastStroke(f).points.size(), std::size_t{2}); // appui et déplacement du premier doigt
    }

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
