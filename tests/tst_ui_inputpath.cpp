#include <actcraft/core/model.h>
#include <actcraft/input/routing.h>
#include <actcraft/render/fixedpageitem.h>
#include <actcraft/render/inkitem.h>
#include <actcraft/render/pendinglayeritem.h>
#include <actcraft/tools/tool.h>
#include <actcraft/ui/documentcontroller.h>
#include <actcraft/ui/pageview.h>
#include <actcraft/ui/toolhost.h>
#include <actcraft/ui/viewtransform.h>

#include <QElapsedTimer>
#include <QPointingDevice>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QQuickWindow>
#include <QSignalSpy>
#include <QTabletEvent>
#include <QTest>

#include <memory>
#include <variant>

using namespace actcraft;
using ui::PageView;

namespace {

// Événements de tablette que la vue laisse passer au reste de la fenêtre. Posé sur la fenêtre avant la vue,
// il n'est consulté qu'après le filtre de celle-ci : ce qu'elle prend, il ne le voit pas.
class PassedOn : public QObject
{
public:
    int count = 0;
    int presses = 0;

protected:
    bool eventFilter(QObject* watched, QEvent* event) override
    {
        const QEvent::Type type = event->type();
        if (type == QEvent::TabletPress || type == QEvent::TabletMove || type == QEvent::TabletRelease) {
            ++count;
            presses += type == QEvent::TabletPress ? 1 : 0;
        }
        return QObject::eventFilter(watched, event);
    }
};

// Montage de tst_pageview (vue avec cache, couche d'attente et contrôleur, sans QML de coquille), au zoom 1
// et sans défilement : point de scène = point de vue = point de page. Couverture d'essai : élément QML enfant
// du contenu, (0, 0, 200, 200) de page, avec un PointHandler qui compte ses activations.
struct Fixture
{
    QQmlEngine engine; // avant la fenêtre : détruit après les éléments qu'il a créés
    QQuickWindow window;
    PassedOn passedOn;
    PageView* view = nullptr;
    render::FixedPageItem* fixed = nullptr;
    QQuickItem* cover = nullptr;
    QPointingDevice* stylus = nullptr;
    std::unique_ptr<ui::DocumentController> controller; // détruit avant la fenêtre et la page fixe
};

bool settleCache(Fixture& f)
{
    // Personne n'appelle notifyCacheUpdated hors de PageArea.qml : le test termine les redessins demandés.
    for (int i = 0; i < 5 && !f.view->cacheReady(); ++i) {
        f.view->notifyCacheUpdated();
    }
    return f.view->cacheReady();
}

bool openFixture(Fixture& f, QPointingDevice* stylus)
{
    f.stylus = stylus;
    f.window.resize(800, 600);
    f.window.installEventFilter(&f.passedOn); // avant la vue : consulté après son filtre
    f.view = new PageView;
    f.view->setParentItem(f.window.contentItem());
    f.view->setSize(QSizeF(800, 600));
    auto* content = new QQuickItem;
    content->setSize(QSizeF(1024, 768));
    content->setParentItem(f.view);
    f.fixed = new render::FixedPageItem;
    f.fixed->setSize(content->size());
    f.fixed->setParentItem(content);
    auto* layer = new render::PendingLayerItem;
    layer->setSize(content->size());
    layer->setParentItem(content);
    layer->setFixedPage(f.fixed);
    auto* ink = new render::InkItem;
    ink->setSize(content->size());
    ink->setParentItem(content);
    f.view->setContent(content); // cache activé (défaut)
    f.view->setFixedPage(f.fixed);
    f.view->setInk(ink);
    f.view->setPendingLayer(layer);
    f.fixed->setPage(core::Page{});
    f.window.show();
    if (!QTest::qWaitForWindowExposed(&f.window) ||
        !QTest::qWaitFor([&] { return f.fixed->isReady(); }, 5000)) {
        return false;
    }
    f.controller = std::make_unique<ui::DocumentController>();
    f.controller->setFixedPage(f.fixed);
    f.view->setDocumentController(f.controller.get());
    f.view->setZoom(1.0);
    f.view->setPan(QPointF(0, 0));
    QQmlComponent component(&f.engine);
    component.setData("import QtQuick\n"
                      "Item {\n"
                      "    id: cover\n"
                      "    width: 200; height: 200\n"
                      "    property int presses: 0\n"
                      "    PointHandler {\n"
                      "        acceptedDevices: PointerDevice.AllDevices\n"
                      "        onActiveChanged: if (active) cover.presses += 1\n"
                      "    }\n"
                      "}\n",
                      QUrl());
    f.cover = qobject_cast<QQuickItem*>(component.create());
    if (!f.cover) {
        return false;
    }
    f.cover->setParent(content);
    f.cover->setParentItem(content);
    f.view->addInputCover(f.cover);
    return settleCache(f) && f.fixed->deferredFolding() &&
           f.view->pageToScene(QPointF(300, 300)) == QPointF(300, 300);
}

int coverPresses(const Fixture& f)
{
    return f.cover->property("presses").toInt();
}

// Bouton qui change : celui de l'appui et du relâché, aucun pour un déplacement (comme les événements du
// système). Piège : QSinglePointEvent tient pour un appui tout événement dont `button` est dans `buttons` ;
// un TabletMove construit avec button = LeftButton porte un point Pressed, et un PointHandler s'y réactive à
// chaque déplacement (cinq activations pour un trait : constaté, tabletMoveCarriesAnUpdatedPoint).
Qt::MouseButton changedButton(QEvent::Type type)
{
    return type == QEvent::TabletMove ? Qt::NoButton : Qt::LeftButton;
}

void tablet(Fixture& f, QEvent::Type type, QPointF scene, Qt::MouseButtons buttons)
{
    QTabletEvent event(type, f.stylus, scene, f.window.mapToGlobal(scene), buttons ? 0.5 : 0.0, 0, 0, 0, 0, 0,
                       Qt::NoModifier, changedButton(type), buttons);
    QCoreApplication::sendEvent(&f.window, &event);
}

// Appui, quatre déplacements, relâché : six événements.
void tabletStroke(Fixture& f, QPointF from, QPointF to)
{
    tablet(f, QEvent::TabletPress, from, Qt::LeftButton);
    for (int i = 1; i <= 4; ++i) {
        tablet(f, QEvent::TabletMove, from + (to - from) * i / 4.0, Qt::LeftButton);
    }
    tablet(f, QEvent::TabletRelease, to, Qt::NoButton);
}

void mouseStroke(Fixture& f, QPoint from, QPoint to)
{
    QTest::mousePress(&f.window, Qt::LeftButton, Qt::NoModifier, from);
    for (int i = 1; i <= 4; ++i) {
        QTest::mouseMove(&f.window, from + (to - from) * i / 4);
    }
    QTest::mouseRelease(&f.window, Qt::LeftButton, Qt::NoModifier, to);
}

// Qt Quick retarde et compresse les TouchUpdate jusqu'à l'image suivante : une image entre deux étapes.
void settleTouch()
{
    QTest::qWait(50);
}

void touchStroke(Fixture& f, QPointingDevice* screen, QPoint from, QPoint to)
{
    QTest::touchEvent(&f.window, screen).press(0, from);
    settleTouch();
    for (int i = 1; i <= 4; ++i) {
        QTest::touchEvent(&f.window, screen).move(0, from + (to - from) * i / 4);
        settleTouch();
    }
    QTest::touchEvent(&f.window, screen).release(0, to);
    settleTouch();
}

const core::Stroke& lastStroke(const Fixture& f)
{
    return std::get<core::Stroke>(f.fixed->page().objects.back());
}

bool closeTo(double a, double b)
{
    return qAbs(a - b) < 0.01;
}

input::Sample shiftRight(const input::Sample& sample)
{
    input::Sample out = sample;
    out.x += 50;
    return out;
}

} // namespace

class TstUiInputpath : public QObject
{
    Q_OBJECT

private:
    std::unique_ptr<QPointingDevice> m_stylus;
    QPointingDevice* m_screen = nullptr;

private slots:
    void initTestCase()
    {
        m_stylus = std::make_unique<QPointingDevice>(
            QStringLiteral("tst-ui-inputpath-stylus"), 5170, QInputDevice::DeviceType::Stylus,
            QPointingDevice::PointerType::Pen,
            QInputDevice::Capability::Position | QInputDevice::Capability::Pressure, 1, 3);
        m_screen = QTest::createTouchDevice();
    }

    // D-S1 : une couverture est une route de la politique, celle du routeur de la vue, pour les trois
    // périphériques. Une vue qui la testerait à côté du routeur (deuxième route) rendrait Tool ici.
    void coverIsARouteOfTheRouter()
    {
        Fixture f;
        QVERIFY(openFixture(f, m_stylus.get()));
        input::Sample press;
        press.phase = input::Phase::Press;
        press.x = 100;
        press.y = 100;
        for (const input::DeviceKind device :
             {input::DeviceKind::Stylus, input::DeviceKind::Mouse, input::DeviceKind::Touch}) {
            press.device = device;
            press.pointer = device == input::DeviceKind::Touch ? 7 : 0;
            QCOMPARE(f.view->router().route(press).route, input::Route::Cover);
            f.view->router().release(press.pointer);
        }
        press.device = input::DeviceKind::Stylus;
        press.pointer = 0;
        press.x = 400;
        QCOMPARE(f.view->router().route(press).route, input::Route::Tool);
        f.view->router().release(0);
    }

    void coverStopsTablet()
    {
        Fixture f;
        QVERIFY(openFixture(f, m_stylus.get()));
        tabletStroke(f, QPointF(100, 100), QPointF(150, 150));
        QTest::qWait(50);
        QCOMPARE(f.fixed->objectCount(), 0);
        QCOMPARE(f.passedOn.presses, 1); // l'appui n'est pas pris par le filtre de la vue
        QCOMPARE(f.passedOn.count, 6);
    }

    void coverStopsFinger()
    {
        Fixture f;
        QVERIFY(openFixture(f, m_stylus.get()));
        touchStroke(f, m_screen, QPoint(100, 100), QPoint(150, 150));
        QCOMPARE(f.fixed->objectCount(), 0);
    }

    void coverStopsMouse()
    {
        Fixture f;
        QVERIFY(openFixture(f, m_stylus.get()));
        mouseStroke(f, QPoint(100, 100), QPoint(150, 150));
        QTest::qWait(50);
        QCOMPARE(f.fixed->objectCount(), 0);
    }

    // Le piège de l'aide tablet() : un déplacement dont `button` est dans `buttons` porte un point Pressed
    // (vu par les gestionnaires de Qt Quick comme un nouvel appui) ; avec button = NoButton, un point
    // Updated.
    void tabletMoveCarriesAnUpdatedPoint()
    {
        const QPointF at(100, 100);
        QTabletEvent pressedMove(QEvent::TabletMove, m_stylus.get(), at, at, 0.5, 0, 0, 0, 0, 0,
                                 Qt::NoModifier, Qt::LeftButton, Qt::LeftButton);
        QCOMPARE(pressedMove.point(0).state(), QEventPoint::State::Pressed);
        QTabletEvent move(QEvent::TabletMove, m_stylus.get(), at, at, 0.5, 0, 0, 0, 0, 0, Qt::NoModifier,
                          changedButton(QEvent::TabletMove), Qt::LeftButton);
        QCOMPARE(move.point(0).state(), QEventPoint::State::Updated);
    }

    // Le gestionnaire de la surcouche reçoit l'appui : un filtre qui avalerait l'événement serait rouge.
    void coverHandlerGetsThePress()
    {
        Fixture f;
        QVERIFY(openFixture(f, m_stylus.get()));
        QCOMPARE(coverPresses(f), 0);
        tabletStroke(f, QPointF(100, 100), QPointF(150, 150));
        QTRY_COMPARE(coverPresses(f), 1);
        touchStroke(f, m_screen, QPoint(100, 100), QPoint(150, 150));
        QTRY_COMPARE(coverPresses(f), 2);
        mouseStroke(f, QPoint(100, 100), QPoint(150, 150));
        QTRY_COMPARE(coverPresses(f), 3);
        QCOMPARE(f.fixed->objectCount(), 0);
    }

    // Un appui couvert du stylet ne pose pas stylusDown : tenu ou relâché, un doigt hors de la couverture
    // trace.
    void coveredStylusTapLeavesFingersFree()
    {
        Fixture f;
        QVERIFY(openFixture(f, m_stylus.get()));
        tablet(f, QEvent::TabletPress, QPointF(100, 100), Qt::LeftButton);
        touchStroke(f, m_screen, QPoint(400, 300), QPoint(500, 300));
        QTRY_COMPARE(f.fixed->objectCount(), 1);
        tablet(f, QEvent::TabletRelease, QPointF(100, 100), Qt::NoButton);
        tablet(f, QEvent::TabletPress, QPointF(120, 120), Qt::LeftButton);
        tablet(f, QEvent::TabletRelease, QPointF(120, 120), Qt::NoButton);
        touchStroke(f, m_screen, QPoint(400, 400), QPoint(500, 400));
        QTRY_COMPARE(f.fixed->objectCount(), 2);
    }

    void uncoveredStillDraws()
    {
        Fixture f;
        QVERIFY(openFixture(f, m_stylus.get()));
        f.view->removeInputCover(f.cover);
        tabletStroke(f, QPointF(100, 100), QPointF(150, 150));
        QTRY_COMPARE(f.fixed->objectCount(), 1);
        touchStroke(f, m_screen, QPoint(100, 120), QPoint(150, 170));
        QTRY_COMPARE(f.fixed->objectCount(), 2);
        mouseStroke(f, QPoint(100, 140), QPoint(150, 190));
        QTRY_COMPARE(f.fixed->objectCount(), 3);
        // Déclarée de nouveau, elle couvre ; détruite, elle ne couvre plus.
        f.view->addInputCover(f.cover);
        tabletStroke(f, QPointF(100, 100), QPointF(150, 150));
        QTest::qWait(50);
        QCOMPARE(f.fixed->objectCount(), 3);
        delete f.cover;
        f.cover = nullptr;
        tabletStroke(f, QPointF(100, 100), QPointF(150, 150));
        QTRY_COMPARE(f.fixed->objectCount(), 4);
    }

    void filterReachesThePen()
    {
        Fixture f;
        QVERIFY(openFixture(f, m_stylus.get()));
        f.view->setSampleFilter(shiftRight);
        tabletStroke(f, QPointF(300, 300), QPointF(400, 300));
        QTRY_COMPARE(f.fixed->objectCount(), 1);
        QVERIFY2(closeTo(lastStroke(f).points.front().x, 350), "premier point filtré");
        QVERIFY(closeTo(lastStroke(f).points.front().y, 300));
        QVERIFY(closeTo(lastStroke(f).points.back().x, 450));
        f.view->setSampleFilter({});
        tabletStroke(f, QPointF(300, 400), QPointF(400, 400));
        QTRY_COMPARE(f.fixed->objectCount(), 2);
        QVERIFY2(closeTo(lastStroke(f).points.front().x, 300), "filtre vide : aucun");
    }

    // Relâchés synthétisés : au dernier point brut, filtré comme lui. Non filtré, le trait finirait en 400 ;
    // filtré deux fois (dernier point filtré pris pour brut), en 500.
    void syntheticReleaseIsFiltered()
    {
        Fixture f;
        QVERIFY(openFixture(f, m_stylus.get()));
        f.view->setSampleFilter(shiftRight);
        tablet(f, QEvent::TabletPress, QPointF(300, 300), Qt::LeftButton);
        tablet(f, QEvent::TabletMove, QPointF(400, 300), Qt::LeftButton);
        f.view->finishStroke();
        QTRY_COMPARE(f.fixed->objectCount(), 1);
        QCOMPARE(lastStroke(f).points.size(), std::size_t{2});
        QVERIFY2(closeTo(lastStroke(f).points.back().x, 450), "stylet : relâché synthétisé filtré");
        tablet(f, QEvent::TabletRelease, QPointF(400, 300), Qt::NoButton); // la suite du contact n'écrit plus
        QTest::qWait(20);
        QCOMPARE(f.fixed->objectCount(), 1);
        // Doigt terminé par un deuxième doigt (finishTouchStroke).
        QTest::touchEvent(&f.window, m_screen).press(0, QPoint(300, 400));
        settleTouch();
        QTest::touchEvent(&f.window, m_screen).move(0, QPoint(400, 400));
        settleTouch();
        QTest::touchEvent(&f.window, m_screen).stationary(0).press(1, QPoint(600, 500));
        QTRY_COMPARE(f.fixed->objectCount(), 2);
        QCOMPARE(lastStroke(f).points.size(), std::size_t{2});
        QVERIFY2(closeTo(lastStroke(f).points.back().x, 450), "doigt : relâché synthétisé filtré");
        QVERIFY(closeTo(lastStroke(f).points.back().y, 400));
        QTest::touchEvent(&f.window, m_screen).release(0, QPoint(400, 400)).release(1, QPoint(600, 500));
        settleTouch();
        QCOMPARE(f.fixed->objectCount(), 2);
    }

    // D-S9 : jamais de versement pendant un geste de l'outil (stylet tenu immobile plus longtemps que le
    // délai).
    void foldWaitsForTheBusyTool()
    {
        Fixture f;
        QVERIFY(openFixture(f, m_stylus.get()));
        QSignalSpy requested(f.view, &PageView::cacheRefreshRequested);
        tabletStroke(f, QPointF(300, 300), QPointF(500, 300));
        QTRY_COMPARE(f.fixed->pendingCount(), 1);
        QVERIFY(f.view->foldTimerActive());
        QCOMPARE(requested.count(), 0);
        tablet(f, QEvent::TabletPress, QPointF(300, 400), Qt::LeftButton);
        QVERIFY(f.view->currentTool()->isBusy());
        QTest::qWait(ui::kCacheFoldIdleMs + 200);
        QCOMPARE(requested.count(), 0);
        QCOMPARE(f.fixed->pendingCount(), 1);
        tablet(f, QEvent::TabletRelease, QPointF(300, 400), Qt::NoButton);
        QTRY_COMPARE_WITH_TIMEOUT(requested.count(), 1, ui::kCacheFoldIdleMs + 2000);
        QCOMPARE(f.fixed->pendingCount(), 0);
    }

    // Le filtre passe avant le Pipeline : le trait filtré attend au-dessus du cache comme un autre.
    void filteredStrokeStaysPendingUntilIdle()
    {
        Fixture f;
        QVERIFY(openFixture(f, m_stylus.get()));
        f.view->setSampleFilter(shiftRight);
        QSignalSpy requested(f.view, &PageView::cacheRefreshRequested);
        tabletStroke(f, QPointF(300, 300), QPointF(400, 300));
        QCOMPARE(f.fixed->pendingCount(), 1); // versé au relâché, dans le même tour (décision 10)
        QCOMPARE(requested.count(), 0);
        QVERIFY(f.view->foldTimerActive());
        QTRY_COMPARE_WITH_TIMEOUT(requested.count(), 1, ui::kCacheFoldIdleMs + 2000);
        QCOMPARE(f.fixed->pendingCount(), 0);
    }

    void rightClickRequestsTheContextMenu()
    {
        Fixture f;
        QVERIFY(openFixture(f, m_stylus.get()));
        QSignalSpy menu(f.view, &PageView::contextMenuRequested);
        QTest::mouseClick(&f.window, Qt::RightButton, Qt::NoModifier, QPoint(300, 300));
        QCOMPARE(menu.count(), 1);
        QCOMPARE(menu.at(0).at(0).toPointF(), QPointF(300, 300)); // position de vue
        // La suite d'un contact droit n'écrit rien.
        QTest::mousePress(&f.window, Qt::RightButton, Qt::NoModifier, QPoint(300, 350));
        QTest::mouseMove(&f.window, QPoint(400, 350));
        QTest::mouseRelease(&f.window, Qt::RightButton, Qt::NoModifier, QPoint(400, 350));
        QCOMPARE(menu.count(), 2);
        QTest::qWait(20);
        QCOMPARE(f.fixed->objectCount(), 0);
    }

    void coverTakesTheRightClick()
    {
        Fixture f;
        QVERIFY(openFixture(f, m_stylus.get()));
        QSignalSpy menu(f.view, &PageView::contextMenuRequested);
        QTest::mouseClick(&f.window, Qt::RightButton, Qt::NoModifier, QPoint(100, 100));
        QCOMPARE(menu.count(), 0);
        QTest::mouseClick(&f.window, Qt::RightButton, Qt::NoModifier, QPoint(300, 300));
        QCOMPARE(menu.count(), 1);
    }

    void zoomAroundKeepsThePointStill()
    {
        Fixture f;
        QVERIFY(openFixture(f, m_stylus.get()));
        const QPointF anchor(300, 200);
        const QPointF before = f.view->sceneToPage(f.view->mapToScene(anchor));
        QSignalSpy refresh(f.view, &PageView::cacheRefreshRequested);
        QElapsedTimer clock;
        clock.start();
        f.view->zoomAround(anchor, 2.0);
        QCOMPARE(f.view->zoom(), 2.0);
        const QPointF after = f.view->sceneToPage(f.view->mapToScene(anchor));
        QVERIFY(qAbs(after.x() - before.x()) <= 0.5);
        QVERIFY(qAbs(after.y() - before.y()) <= 0.5);
        QCOMPARE(f.view->sceneToPage(f.view->pageToScene(QPointF(123, 45))), QPointF(123, 45));
        QCOMPARE(refresh.count(), 0); // comme la molette : pas tout de suite
        QVERIFY(!f.view->cacheReady());
        QTRY_COMPARE_WITH_TIMEOUT(refresh.count(), 1, ui::kCacheRefreshDelayMs + 1000);
        QVERIFY(clock.elapsed() >= ui::kCacheRefreshDelayMs * 9 / 10); // minuterie grossière : 5 % près
    }

    void cacheRefreshCountCounts()
    {
        Fixture f;
        QVERIFY(openFixture(f, m_stylus.get()));
        const int start = f.view->cacheRefreshCount();
        f.view->requestCacheRefresh();
        f.view->notifyCacheUpdated();
        QCOMPARE(f.view->cacheRefreshCount(), start + 1);
        f.view->requestCacheRefresh();
        f.view->requestCacheRefresh(); // pendant le redessin : sa fin en relance un
        f.view->notifyCacheUpdated();  // relance : ne compte pas
        QCOMPARE(f.view->cacheRefreshCount(), start + 1);
        f.view->notifyCacheUpdated();
        QCOMPARE(f.view->cacheRefreshCount(), start + 2);
        QCOMPARE(f.view->property("cacheRefreshCount").toInt(), start + 2);
    }

    // Sans ToolHost, ou si son outil actif est nullptr (socle), la voie Tool va au stylo interne.
    void currentToolFallsBackToThePen()
    {
        Fixture f;
        QVERIFY(openFixture(f, m_stylus.get()));
        QVERIFY(f.view->currentTool());
        QCOMPARE(f.view->currentTool()->id(), QStringLiteral("pen"));
        ui::ToolHost host;
        QSignalSpy changed(f.view, &PageView::toolsChanged);
        f.view->setTools(&host);
        QCOMPARE(changed.count(), 1);
        QCOMPARE(f.view->tools(), &host);
        QCOMPARE(f.view->currentTool()->id(), QStringLiteral("pen"));
        QVERIFY(!f.view->gestureActive());
        tabletStroke(f, QPointF(300, 300), QPointF(400, 300));
        QTRY_COMPARE(f.fixed->objectCount(), 1);
        f.view->setTools(nullptr);
        QCOMPARE(changed.count(), 2);
    }
};

QTEST_MAIN(TstUiInputpath)
#include "tst_ui_inputpath.moc"
