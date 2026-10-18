#include <actcraft/core/document.h>
#include <actcraft/core/model.h>
#include <actcraft/render/fixedpageitem.h>
#include <actcraft/render/inkitem.h>
#include <actcraft/render/pendinglayeritem.h>
#include <actcraft/ui/documentcontroller.h>
#include <actcraft/ui/pageview.h>
#include <actcraft/ui/viewtransform.h>

#include <QElapsedTimer>
#include <QMouseEvent>
#include <QPointingDevice>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QQuickWindow>
#include <QSignalSpy>
#include <QSurfaceFormat>
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

// Vue AVEC cache, page fixe et encre, sans QML : le test termine lui-même chaque redessin demandé (personne
// n'appelle notifyCacheUpdated hors de PageArea.qml) et compte les demandes.
struct CachedFixture
{
    QQuickWindow window;
    PageView* view = nullptr;
    actcraft::render::FixedPageItem* fixed = nullptr;
    QPointingDevice* stylus = nullptr;
};

bool openCached(CachedFixture& f, QPointingDevice* stylus)
{
    f.stylus = stylus;
    f.window.resize(1000, 600);
    f.view = new PageView;
    f.view->setParentItem(f.window.contentItem());
    f.view->setSize(QSizeF(1000, 600));
    auto* content = new QQuickItem;
    content->setSize(QSizeF(1024, 768));
    content->setParentItem(f.view);
    f.fixed = new actcraft::render::FixedPageItem;
    f.fixed->setSize(content->size());
    f.fixed->setParentItem(content);
    auto* ink = new actcraft::render::InkItem;
    ink->setSize(content->size());
    ink->setParentItem(content);
    f.view->setContent(content); // cache activé (défaut)
    f.view->setFixedPage(f.fixed);
    f.view->setInk(ink);
    f.fixed->setPage(actcraft::core::Page{});
    f.window.show();
    if (!QTest::qWaitForWindowExposed(&f.window) ||
        !QTest::qWaitFor([&] { return f.fixed->isReady(); }, 5000)) {
        return false;
    }
    for (int i = 0; i < 5 && !f.view->cacheReady(); ++i) {
        f.view->notifyCacheUpdated();
    }
    return f.view->cacheReady();
}

void cachedStroke(CachedFixture& f, QPointF from, QPointF to)
{
    const auto send = [&f](QEvent::Type type, QPointF scene, Qt::MouseButtons buttons) {
        QTabletEvent event(type, f.stylus, scene, f.window.mapToGlobal(scene), buttons ? 0.5 : 0.0, 0, 0, 0,
                           0, 0, Qt::NoModifier, Qt::LeftButton, buttons);
        QCoreApplication::sendEvent(&f.window, &event);
    };
    send(QEvent::TabletPress, from, Qt::LeftButton);
    for (int i = 1; i <= 4; ++i) {
        send(QEvent::TabletMove, from + (to - from) * i / 4.0, Qt::LeftButton);
    }
    send(QEvent::TabletRelease, to, Qt::NoButton);
}

// Lot 10 : vue avec cache ET couche d'attente (sœur du cache, jamais enfant de la page fixe) ; la page fixe
// passe en différé.
struct PendingFixture
{
    CachedFixture cached;
    actcraft::render::PendingLayerItem* layer = nullptr;
};

bool openPending(PendingFixture& f, QPointingDevice* stylus)
{
    if (!openCached(f.cached, stylus)) {
        return false;
    }
    f.layer = new actcraft::render::PendingLayerItem;
    f.layer->setSize(f.cached.fixed->size());
    f.layer->setParentItem(f.cached.fixed->parentItem());
    f.layer->setFixedPage(f.cached.fixed);
    f.cached.view->setPendingLayer(f.layer);
    return f.cached.fixed->deferredFolding();
}

// Trait horizontal de 21 points de (100, y) à (500, y), largeur 4.
actcraft::core::Stroke line(double y)
{
    actcraft::core::Stroke s;
    s.baseWidth = 4;
    s.color = 0xFF000000u;
    for (int i = 0; i <= 20; ++i) {
        s.points.push_back(actcraft::core::StrokePoint{100.0 + 20.0 * i, y, 0.5f});
    }
    return s;
}

const actcraft::core::Stroke& lastStroke(const DrawFixture& f)
{
    return std::get<actcraft::core::Stroke>(f.fixed->page().objects.back());
}

// Qt Quick retarde et compresse les TouchUpdate jusqu'à l'image suivante : avant un événement envoyé
// directement (stylet, annulation, saisie volée), laisser passer une image pour que le déplacement soit
// livré.
void settleTouch()
{
    QTest::qWait(50);
}

// Les traits des tests sont horizontaux : un point d'un autre trait (autre hauteur) s'y verrait.
bool onOneLine(const actcraft::core::Stroke& stroke)
{
    for (const actcraft::core::StrokePoint& point : stroke.points) {
        if (point.y != stroke.points.front().y) {
            return false;
        }
    }
    return true;
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

    void inkColorAndWidthAreClampedAndSignalled()
    {
        DrawFixture f;
        QVERIFY(openDrawing(f, m_stylus.get()));
        QCOMPARE(f.view->inkColor(), QColor(Qt::black));
        QCOMPARE(f.view->inkWidth(), kDefaultInkWidth);
        QSignalSpy changed(f.view, &PageView::penChanged);
        f.view->setInkColor(QColor(QStringLiteral("#E53935")));
        f.view->setInkWidth(0.0);
        QCOMPARE(f.view->inkWidth(), kMinInkWidth);
        f.view->setInkWidth(500.0);
        QCOMPARE(f.view->inkWidth(), kMaxInkWidth);
        f.view->setInkColor(QColor()); // invalide : ignorée
        QCOMPARE(f.view->inkColor(), QColor(QStringLiteral("#E53935")));
        QCOMPARE(changed.count(), 3);
        // Appliquées au stylo en place…
        tabletStroke(f, QPointF(300, 300), QPointF(500, 300));
        QTRY_COMPARE(f.fixed->objectCount(), 1);
        QCOMPARE(lastStroke(f).color, 0xFFE53935u);
        QCOMPARE(lastStroke(f).baseWidth, kMaxInkWidth);
        // … et gardées quand il est reconstruit (la page change).
        f.view->setFixedPage(nullptr);
        f.view->setFixedPage(f.fixed);
        tabletStroke(f, QPointF(300, 400), QPointF(500, 400));
        QTRY_COMPARE(f.fixed->objectCount(), 2);
        QCOMPARE(lastStroke(f).color, 0xFFE53935u);
        QCOMPARE(lastStroke(f).baseWidth, kMaxInkWidth);
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

    void fingerThenStylusMakeTwoSeparateStrokes()
    {
        DrawFixture f;
        QVERIFY(openDrawing(f, m_stylus.get()));
        QPointingDevice* screen = QTest::createTouchDevice();
        QTest::touchEvent(&f.window, screen).press(0, QPoint(300, 100));
        settleTouch();
        QTest::touchEvent(&f.window, screen).move(0, QPoint(350, 100));
        settleTouch();
        tablet(f, QEvent::TabletPress, QPointF(300, 300), Qt::LeftButton); // termine le trait du doigt
        QTest::touchEvent(&f.window, screen).move(0, QPoint(400, 100));
        settleTouch();
        tablet(f, QEvent::TabletMove, QPointF(350, 300), Qt::LeftButton);
        QTest::touchEvent(&f.window, screen).release(0, QPoint(400, 100));
        settleTouch();
        tablet(f, QEvent::TabletMove, QPointF(400, 300), Qt::LeftButton);
        tablet(f, QEvent::TabletRelease, QPointF(450, 300), Qt::NoButton);
        QTRY_COMPARE(f.fixed->objectCount(), 2);
        QTest::qWait(50);
        QCOMPARE(f.fixed->objectCount(), 2);
        const auto& finger = std::get<actcraft::core::Stroke>(f.fixed->page().objects[0]);
        const auto& stylus = std::get<actcraft::core::Stroke>(f.fixed->page().objects[1]);
        QCOMPARE(finger.points.size(), std::size_t{2}); // appui et premier déplacement du doigt
        QVERIFY(onOneLine(finger));
        QCOMPARE(stylus.points.size(), std::size_t{4}); // 300, 350, 400, 450 : aucun point du doigt
        QVERIFY(onOneLine(stylus));
    }

    void stylusStrokeIgnoresFingerUntilReleased()
    {
        DrawFixture f;
        QVERIFY(openDrawing(f, m_stylus.get()));
        QPointingDevice* screen = QTest::createTouchDevice();
        tablet(f, QEvent::TabletPress, QPointF(300, 300), Qt::LeftButton);
        tablet(f, QEvent::TabletMove, QPointF(350, 300), Qt::LeftButton);
        QTest::touchEvent(&f.window, screen).press(0, QPoint(300, 100)); // paume ou doigt : refusé
        settleTouch();
        QTest::touchEvent(&f.window, screen).move(0, QPoint(400, 100));
        settleTouch();
        tablet(f, QEvent::TabletMove, QPointF(400, 300), Qt::LeftButton);
        QTest::touchEvent(&f.window, screen).release(0, QPoint(400, 100));
        settleTouch();
        tablet(f, QEvent::TabletMove, QPointF(450, 300), Qt::LeftButton);
        tablet(f, QEvent::TabletRelease, QPointF(500, 300), Qt::NoButton);
        QTRY_COMPARE(f.fixed->objectCount(), 1);
        QTest::qWait(50);
        QCOMPARE(f.fixed->objectCount(), 1);
        QCOMPARE(lastStroke(f).points.size(), std::size_t{5}); // 300 à 500, d'un seul tenant
        QVERIFY(onOneLine(lastStroke(f)));
    }

    void fingerStrokeEndsOnCancelOrLostGrab()
    {
        DrawFixture f;
        QVERIFY(openDrawing(f, m_stylus.get()));
        QPointingDevice* screen = QTest::createTouchDevice();
        // Trait ordinaire : le relâché, à un autre point que le dernier déplacement, en fait partie.
        QTest::touchEvent(&f.window, screen).press(0, QPoint(300, 100));
        QTest::touchEvent(&f.window, screen).move(0, QPoint(350, 100));
        QTest::touchEvent(&f.window, screen).release(0, QPoint(400, 100));
        QTRY_COMPARE(f.fixed->objectCount(), 1);
        QCOMPARE(lastStroke(f).points.size(), std::size_t{3});
        // Annulation (système) : le trait se termine au dernier point connu, la suite ne dessine pas.
        QTest::touchEvent(&f.window, screen).press(0, QPoint(300, 200));
        QTest::touchEvent(&f.window, screen).move(0, QPoint(350, 200));
        settleTouch();
        QTouchEvent cancel(QEvent::TouchCancel, screen);
        QCoreApplication::sendEvent(f.view, &cancel);
        QTRY_COMPARE(f.fixed->objectCount(), 2);
        QTest::touchEvent(&f.window, screen).move(0, QPoint(400, 200));
        QTest::touchEvent(&f.window, screen).release(0, QPoint(400, 200));
        // Saisie volée (futur geste, Flickable) : idem.
        QTest::touchEvent(&f.window, screen).press(0, QPoint(300, 300));
        QTest::touchEvent(&f.window, screen).move(0, QPoint(350, 300));
        settleTouch();
        f.view->ungrabTouchPoints();
        QTRY_COMPARE(f.fixed->objectCount(), 3);
        QTest::touchEvent(&f.window, screen).move(0, QPoint(400, 300));
        QTest::touchEvent(&f.window, screen).release(0, QPoint(400, 300));
        QTest::qWait(50);
        QCOMPARE(f.fixed->objectCount(), 3);
        QCOMPARE(lastStroke(f).points.size(), std::size_t{2});
    }

    void disabledViewTakesNoStylus()
    {
        DrawFixture f;
        QVERIFY(openDrawing(f, m_stylus.get()));
        f.view->setEnabled(false);
        tabletStroke(f, QPointF(300, 300), QPointF(500, 300));
        QTest::qWait(50);
        QCOMPARE(f.fixed->objectCount(), 0);
        QCOMPARE(f.passedOn.count, 6);
        f.view->setEnabled(true);
        tabletStroke(f, QPointF(300, 300), QPointF(500, 300));
        QTRY_COMPARE(f.fixed->objectCount(), 1);
    }

    // Une barre de défilement posée sur la vue prend la souris et le doigt avant elle (Qt Quick livre à
    // l'élément du dessus) ; le stylet, capté par le filtre de la fenêtre, doit lui être laissé de même.
    void stylusLeavesAnInteractiveItemAboveTheViewAlone()
    {
        DrawFixture f;
        QVERIFY(openDrawing(f, m_stylus.get()));
        // Barre : au-dessus de la vue (empilée après elle), sur sa partie basse droite, prend le bouton
        // gauche.
        auto* bar = new QQuickItem;
        bar->setParentItem(f.window.contentItem());
        bar->setPosition(QPointF(600, 500));
        bar->setSize(QSizeF(200, 100));
        bar->setAcceptedMouseButtons(Qt::LeftButton);
        tabletStroke(f, QPointF(650, 550), QPointF(750, 550));
        QTest::qWait(50);
        QCOMPARE(f.fixed->objectCount(), 0);
        QCOMPARE(f.passedOn.count, 6);
        // Indicateur (cercle sous le pointeur) : au-dessus de la vue mais ne prend rien, le stylet dessine.
        auto* ring = new QQuickItem;
        ring->setParentItem(f.window.contentItem());
        ring->setPosition(QPointF(300, 0));
        ring->setSize(QSizeF(200, 600));
        tabletStroke(f, QPointF(350, 300), QPointF(450, 300));
        QTRY_COMPARE(f.fixed->objectCount(), 1);
        QCOMPARE(f.passedOn.count, 6);
        // Élément actif sous la vue, ou barre cachée : la vue est dessus, le stylet dessine.
        auto* under = new QQuickItem;
        under->setParentItem(f.window.contentItem());
        under->setZ(-1);
        under->setPosition(QPointF(200, 100));
        under->setSize(QSizeF(200, 100));
        under->setAcceptedMouseButtons(Qt::LeftButton);
        tabletStroke(f, QPointF(250, 150), QPointF(350, 150));
        QTRY_COMPARE(f.fixed->objectCount(), 2);
        bar->setVisible(false);
        tabletStroke(f, QPointF(650, 550), QPointF(750, 550));
        QTRY_COMPARE(f.fixed->objectCount(), 3);
        QCOMPARE(f.passedOn.count, 6);
        // Élément actif de même z déclaré avant la vue : dessous, le stylet dessine.
        auto* before = new QQuickItem;
        before->setParentItem(f.window.contentItem());
        before->stackBefore(f.view);
        before->setPosition(QPointF(500, 100));
        before->setSize(QSizeF(300, 100));
        before->setAcceptedMouseButtons(Qt::LeftButton);
        tabletStroke(f, QPointF(550, 150), QPointF(650, 150));
        QTRY_COMPARE(f.fixed->objectCount(), 4);
        QCOMPARE(f.passedOn.count, 6);
    }

    // Un gestionnaire de pointeur ajoute tous les boutons à acceptedMouseButtons() de son élément : seuls
    // comptent ceux de l'élément et les gestionnaires qui prennent l'appui (TapHandler, DragHandler,
    // PointHandler).
    void stylusIgnoresHandlersThatTakeNoPress()
    {
        QQmlEngine engine; // avant la fenêtre : détruit après les éléments qu'il a créés
        DrawFixture f;
        QVERIFY(openDrawing(f, m_stylus.get()));
        const auto make = [&](const QByteArray& qml, QPointF pos) -> QQuickItem* {
            QQmlComponent component(&engine);
            component.setData("import QtQuick\nItem { width: 200; height: 100\n" + qml + "\n}", QUrl());
            auto* item = qobject_cast<QQuickItem*>(component.create());
            if (item) {
                item->setParent(f.window.contentItem());
                item->setParentItem(f.window.contentItem());
                item->setPosition(pos);
            }
            return item;
        };
        // Survol seulement : au-dessus de la vue, ne prend pas l'appui, le stylet dessine.
        QVERIFY(make("HoverHandler {}", QPointF(300, 0)));
        tabletStroke(f, QPointF(350, 50), QPointF(450, 50));
        QTRY_COMPARE(f.fixed->objectCount(), 1);
        QCOMPARE(f.passedOn.count, 0);
        // Tape : prend l'appui, le stylet lui est laissé.
        QVERIFY(make("TapHandler {}", QPointF(300, 200)));
        tabletStroke(f, QPointF(350, 250), QPointF(450, 250));
        QTest::qWait(50);
        QCOMPARE(f.fixed->objectCount(), 1);
        QCOMPARE(f.passedOn.count, 6);
        // MouseArea (prend elle-même le bouton gauche) avec un survol : laissée aussi.
        QVERIFY(make("MouseArea { anchors.fill: parent; HoverHandler {} }", QPointF(300, 400)));
        tabletStroke(f, QPointF(350, 450), QPointF(450, 450));
        QTest::qWait(50);
        QCOMPARE(f.fixed->objectCount(), 1);
        QCOMPARE(f.passedOn.count, 12);
        // Limite connue (API publique seule) : un élément d'un autre type qui accepte le bouton gauche ET
        // porte un survol n'est pas reconnu, le stylet y dessine. Aucun élément de l'interface n'est dans ce
        // cas.
        QQuickItem* plain = make("HoverHandler {}", QPointF(550, 400));
        QVERIFY(plain);
        plain->setAcceptedMouseButtons(Qt::LeftButton);
        tabletStroke(f, QPointF(600, 450), QPointF(700, 450));
        QTRY_COMPARE(f.fixed->objectCount(), 2);
        QCOMPARE(f.passedOn.count, 12);
    }

    // Zoom, « Ajuster » ou défilement pendant un trait : la page bouge sous le stylet. Le trait se termine
    // avant, là où il était, versé à la page ; la suite du contact ne dessine plus (sinon le trait
    // sauterait).
    void transformChangeEndsTheStrokeWhereItWas()
    {
        DrawFixture f;
        QVERIFY(openDrawing(f, m_stylus.get()));
        const auto changes = {+[](PageView* v) { v->setZoom(2.0); }, +[](PageView* v) { v->fitPage(); }};
        int strokes = 0;
        for (const auto& change : changes) {
            tablet(f, QEvent::TabletPress, QPointF(300, 300), Qt::LeftButton);
            tablet(f, QEvent::TabletMove, QPointF(350, 300), Qt::LeftButton);
            tablet(f, QEvent::TabletMove, QPointF(400, 300), Qt::LeftButton);
            const QPointF last = f.view->viewTransform().viewToPage(f.view->mapFromScene(QPointF(400, 300)));
            change(f.view);
            ++strokes;
            QCOMPARE(f.fixed->objectCount(), strokes); // versé avant que la transformation change
            QCOMPARE(f.view->ink()->pointCount(), std::size_t{0});
            QCOMPARE(lastStroke(f).points.size(), std::size_t{3});
            QCOMPARE(lastStroke(f).points.back().x, last.x());
            tablet(f, QEvent::TabletMove, QPointF(450, 300), Qt::LeftButton);
            tablet(f, QEvent::TabletRelease, QPointF(500, 300), Qt::NoButton);
            QTest::qWait(50);
            QCOMPARE(f.fixed->objectCount(), strokes);
            QCOMPARE(lastStroke(f).points.size(), std::size_t{3});
        }
        // Le contact suivant dessine normalement.
        tabletStroke(f, QPointF(300, 400), QPointF(500, 400));
        QTRY_COMPARE(f.fixed->objectCount(), strokes + 1);
    }

    void pageSizeChangeSignalsScroll()
    {
        Fixture f;
        QVERIFY(open(f));
        f.view->setZoom(4.0);
        f.view->setPan(QPointF(kViewMargin, kViewMargin));
        const qreal before = f.view->scrollSizeX();
        QSignalSpy scrolled(f.view, &PageView::scrollChanged);
        auto* wider = new QQuickItem;
        wider->setSize(QSizeF(2048, 768));
        wider->setParentItem(f.view);
        f.view->setContent(wider); // ni le zoom ni le défilement ne bougent, les barres si
        QCOMPARE(f.view->pan(), QPointF(kViewMargin, kViewMargin));
        QVERIFY(f.view->scrollSizeX() < before);
        QCOMPARE(scrolled.count(), 1);
    }

    void scrollToKeepsFitWhenNothingMoves()
    {
        Fixture f;
        QVERIFY(open(f));
        f.view->scrollTo(0.0, 0.0); // page entière visible : une barre renvoie sa position, rien ne bouge
        f.view->setSize(QSizeF(640, 400));
        QVERIFY(qFuzzyCompare(f.view->zoom(), (400.0 - 64.0) / 768.0)); // toujours « Ajuster »
    }

    // Molette sur une page entière visible : rien ne bouge, « Ajuster » reste (F5 ou un redimensionnement
    // recadrent encore la page), comme pour une barre de défilement.
    void wheelKeepsFitWhenNothingMoves()
    {
        Fixture f;
        QVERIFY(open(f));
        wheel(f, QPointF(640, 400), QPoint(0, -120), Qt::NoModifier);
        wheel(f, QPointF(640, 400), QPoint(0, -120), Qt::ShiftModifier);
        f.view->setSize(QSizeF(640, 400));
        QVERIFY(qFuzzyCompare(f.view->zoom(), (400.0 - 64.0) / 768.0)); // toujours « Ajuster »
        // Vue si petite qu'« Ajuster » bute sur le zoom minimal : Ctrl+molette (dézoom) ne bouge rien.
        f.view->setSize(QSizeF(250, 200));
        QCOMPARE(f.view->zoom(), kMinZoom);
        const QPointF pan = f.view->pan();
        wheel(f, QPointF(125, 100), QPoint(0, -120), Qt::ControlModifier);
        QCOMPARE(f.view->zoom(), kMinZoom);
        QCOMPARE(f.view->pan(), pan);
        f.view->setSize(QSizeF(640, 400));
        QVERIFY(qFuzzyCompare(f.view->zoom(), (400.0 - 64.0) / 768.0)); // toujours « Ajuster »
    }

    // Une molette qui bouge vraiment la page quitte « Ajuster » : le redimensionnement ne recadre plus.
    void wheelThatMovesLeavesFit()
    {
        Fixture f;
        QVERIFY(open(f));
        // Défilement : « Ajuster » au zoom minimal sur une vue étroite, la page déborde en largeur.
        f.view->setSize(QSizeF(250, 200));
        QCOMPARE(f.view->zoom(), kMinZoom);
        const QPointF pan = f.view->pan();
        wheel(f, QPointF(125, 100), QPoint(0, -120), Qt::ShiftModifier);
        QVERIFY(f.view->pan() != pan);
        f.view->setSize(QSizeF(640, 400));
        QCOMPARE(f.view->zoom(), kMinZoom);
        // Zoom : Ctrl+molette depuis « Ajuster ».
        f.view->fitPage();
        const qreal fit = f.view->zoom();
        wheel(f, QPointF(320, 200), QPoint(0, 120), Qt::ControlModifier);
        const qreal zoomed = f.view->zoom();
        QVERIFY(zoomed > fit);
        f.view->setSize(QSizeF(1280, 800));
        QCOMPARE(f.view->zoom(), zoomed);
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

    // Avec un contrôleur, le stylo verse au document par la pile (ControllerSink) ; sans contrôleur, le trait
    // va comme avant dans la page interne de l'élément (tests de trait ci-dessus, inchangés).
    void strokeGoesToTheControllerDocument()
    {
        DrawFixture f;
        QVERIFY(openDrawing(f, m_stylus.get()));
        DocumentController controller;
        controller.setFixedPage(f.fixed);
        f.view->setDocumentController(&controller);
        QSignalSpy frozen(f.view, &PageView::strokeFrozen);
        tabletStroke(f, QPointF(300, 300), QPointF(500, 300));
        QTRY_COMPARE(controller.document().page(0).objects.size(), std::size_t{1});
        QCOMPARE(controller.document().page(0).objects[0].common.layer, actcraft::core::Layer::Top);
        QCOMPARE(f.fixed->objectCount(), 1);
        QCOMPARE(frozen.count(), 1);
        QVERIFY(controller.canUndo());
    }

    // Un redessin du cache par trait : contentChanged (insertion dans le cache) le demande, onStrokeFrozen
    // n'émet plus que strokeFrozen. Les deux ensemble en demanderaient trois (m_refreshAgain).
    void oneCacheRenderPerStroke()
    {
        CachedFixture f;
        QVERIFY(openCached(f, m_stylus.get()));
        DocumentController controller;
        controller.setFixedPage(f.fixed);
        f.view->setDocumentController(&controller);
        for (int i = 0; i < 5 && !f.view->cacheReady(); ++i) {
            f.view->notifyCacheUpdated(); // setSource du contrôleur a demandé un redessin
        }
        QVERIFY(f.view->cacheReady());
        QSignalSpy requested(f.view, &PageView::cacheRefreshRequested);
        cachedStroke(f, QPointF(300, 200), QPointF(500, 260));
        QTRY_COMPARE(controller.document().page(0).objects.size(), std::size_t{1});
        for (int i = 0; i < 5 && !f.view->cacheReady(); ++i) {
            f.view
                ->notifyCacheUpdated(); // fin de redessin, comme ShaderEffectSource.scheduledUpdateCompleted
        }
        QVERIFY(f.view->cacheReady());
        QCOMPARE(requested.count(), 1);
    }

    // Lot 10 (DA11) : avec une couche d'attente, un trait n'est versé au cache qu'après l'inactivité.
    void strokeWaitsAboveTheCacheUntilIdle()
    {
        PendingFixture f;
        QVERIFY(openPending(f, m_stylus.get()));
        QSignalSpy requested(f.cached.view, &PageView::cacheRefreshRequested);
        cachedStroke(f.cached, QPointF(300, 200), QPointF(500, 260));
        QTRY_COMPARE(f.cached.fixed->pendingCount(), 1);
        QCOMPARE(requested.count(), 0);
        QVERIFY(f.cached.view->cacheReady()); // l'affichage (cache + attente) est juste
        QVERIFY(f.cached.view->foldTimerActive());
        QTRY_COMPARE_WITH_TIMEOUT(requested.count(), 1, kCacheFoldIdleMs + 2000);
        QCOMPARE(f.cached.fixed->pendingCount(), 0); // versé par la demande elle-même
    }

    void inputPostponesTheFold()
    {
        PendingFixture f;
        QVERIFY(openPending(f, m_stylus.get()));
        QSignalSpy requested(f.cached.view, &PageView::cacheRefreshRequested);
        cachedStroke(f.cached, QPointF(300, 200), QPointF(500, 260));
        QTest::qWait(kCacheFoldIdleMs * 6 / 10);
        cachedStroke(f.cached, QPointF(300, 300), QPointF(500, 360));
        QTest::qWait(kCacheFoldIdleMs * 6 / 10);
        QCOMPARE(requested.count(), 0); // 1,2 délai depuis le premier trait, 0,6 depuis le second
        QTRY_COMPARE_WITH_TIMEOUT(requested.count(), 1, kCacheFoldIdleMs + 2000);
        QCOMPARE(f.cached.fixed->pendingCount(), 0);
    }

    // Molette et clavier repoussent aussi le versement (une entrée, quelle qu'elle soit).
    void wheelPostponesTheFold()
    {
        PendingFixture f;
        QVERIFY(openPending(f, m_stylus.get()));
        QSignalSpy requested(f.cached.view, &PageView::cacheRefreshRequested);
        f.cached.fixed->appendStroke(line(100));
        QVERIFY(f.cached.view->foldTimerActive());
        QTest::qWait(kCacheFoldIdleMs * 6 / 10);
        QWheelEvent event(QPointF(500, 300), f.cached.window.mapToGlobal(QPointF(500, 300)), QPoint(),
                          QPoint(0, -120), Qt::NoButton, Qt::NoModifier, Qt::NoScrollPhase, false);
        QCoreApplication::sendEvent(&f.cached.window, &event);
        QTest::qWait(kCacheFoldIdleMs * 6 / 10);
        QCOMPARE(requested.count(), 0); // 1,2 délai depuis le trait, 0,6 depuis la molette
        QTest::keyClick(&f.cached.window, Qt::Key_Shift);
        QTest::qWait(kCacheFoldIdleMs * 6 / 10);
        QCOMPARE(requested.count(), 0); // 0,6 délai depuis la touche
        QTRY_COMPARE_WITH_TIMEOUT(requested.count(), 1, kCacheFoldIdleMs + 2000);
        QCOMPARE(f.cached.fixed->pendingCount(), 0);
    }

    // Un survol (stylet tenu près du tableau, souris qui passe) ne repousse pas le versement.
    void hoverDoesNotPostponeTheFold()
    {
        PendingFixture f;
        QVERIFY(openPending(f, m_stylus.get()));
        QSignalSpy requested(f.cached.view, &PageView::cacheRefreshRequested);
        f.cached.fixed->appendStroke(line(100));
        QElapsedTimer clock;
        clock.start();
        while (requested.count() == 0 && clock.elapsed() < kCacheFoldIdleMs + 2000) {
            const QPointF at(400.0 + static_cast<double>(clock.elapsed() % 50), 300.0);
            QTabletEvent hover(QEvent::TabletMove, f.cached.stylus, at, f.cached.window.mapToGlobal(at), 0.0,
                               0, 0, 0, 0, 0, Qt::NoModifier, Qt::NoButton, Qt::NoButton);
            QCoreApplication::sendEvent(&f.cached.window, &hover);
            QMouseEvent move(QEvent::MouseMove, at, at, f.cached.window.mapToGlobal(at), Qt::NoButton,
                             Qt::NoButton, Qt::NoModifier);
            QCoreApplication::sendEvent(&f.cached.window, &move);
            QTest::qWait(20);
        }
        QCOMPARE(requested.count(), 1);                    // versé malgré les survols
        QVERIFY(clock.elapsed() < kCacheFoldIdleMs + 500); // et à l'heure, pas repoussé
        QCOMPARE(f.cached.fixed->pendingCount(), 0);
    }

    void zoomFoldsThePending()
    {
        PendingFixture f;
        QVERIFY(openPending(f, m_stylus.get()));
        cachedStroke(f.cached, QPointF(300, 200), QPointF(500, 260));
        QTRY_COMPARE(f.cached.fixed->pendingCount(), 1);
        QSignalSpy requested(f.cached.view, &PageView::cacheRefreshRequested);
        f.cached.view->setZoom(2.0);
        QCOMPARE(requested.count(), 1);
        QCOMPARE(f.cached.fixed->pendingCount(), 0);
        QVERIFY(!f.cached.view->foldTimerActive());
    }

    // Invariant d'échelle (revue de L10-T3) : tout setRasterScale de la page fixe est suivi d'un versement
    // dans le même tour de boucle, par chacun de ses chemins (zoom posé, ajustement, page redimensionnée, fin
    // d'un zoom à la molette). Versé hors de la demande de redessin (minuterie d'inactivité seule, fin du
    // redessin), la couche d'attente serait refaite à chaque échelle, hors de la texture redessinée.
    void everyRasterScaleChangeFoldsInTheSameTurn()
    {
        PendingFixture f;
        QVERIFY(openPending(f, m_stylus.get()));
        actcraft::render::FixedPageItem* fixed = f.cached.fixed;
        // Zoom posé.
        fixed->appendStroke(line(100));
        QCOMPARE(fixed->pendingCount(), 1);
        double scale = fixed->rasterScale();
        f.cached.view->setZoom(2.0);
        QVERIFY(!qFuzzyCompare(fixed->rasterScale(), scale));
        QCOMPARE(fixed->pendingCount(), 0);
        // Ajustement.
        fixed->appendStroke(line(120));
        QCOMPARE(fixed->pendingCount(), 1);
        scale = fixed->rasterScale();
        f.cached.view->fitPage();
        QVERIFY(!qFuzzyCompare(fixed->rasterScale(), scale));
        QCOMPARE(fixed->pendingCount(), 0);
        // Page redimensionnée : « Ajuster » suit.
        fixed->appendStroke(line(140));
        QCOMPARE(fixed->pendingCount(), 1);
        scale = fixed->rasterScale();
        f.cached.view->content()->setSize(QSizeF(2048, 1536));
        QVERIFY(!qFuzzyCompare(fixed->rasterScale(), scale));
        QCOMPARE(fixed->pendingCount(), 0);
        // Fin d'un zoom à la molette (minuterie du redessin, après le dernier cran).
        fixed->appendStroke(line(160));
        QCOMPARE(fixed->pendingCount(), 1);
        scale = fixed->rasterScale();
        const QPointF pos(500, 300);
        QWheelEvent event(pos, f.cached.window.mapToGlobal(pos), QPoint(), QPoint(0, 120), Qt::NoButton,
                          Qt::ControlModifier, Qt::NoScrollPhase, false);
        QCoreApplication::sendEvent(&f.cached.window, &event);
        QCOMPARE(fixed->pendingCount(), 1); // zoom en cours : l'échelle n'a pas bougé, rien n'est versé
        QElapsedTimer clock;
        clock.start();
        while (qFuzzyCompare(fixed->rasterScale(), scale) && clock.elapsed() < 3000) {
            QCoreApplication::processEvents(QEventLoop::AllEvents, 1);
        }
        QVERIFY(!qFuzzyCompare(fixed->rasterScale(), scale));
        QCOMPARE(fixed->pendingCount(), 0); // versé dans le tour même où l'échelle a changé
        QVERIFY(!f.cached.view->foldTimerActive());
    }

    void tooManyPendingFoldAtOnce()
    {
        PendingFixture f;
        QVERIFY(openPending(f, m_stylus.get()));
        QSignalSpy requested(f.cached.view, &PageView::cacheRefreshRequested);
        for (int i = 0; i < kCacheFoldMaxPending - 1; ++i) {
            f.cached.fixed->appendStroke(line(10.0 + i)); // page interne : insert, donc attente
        }
        QCOMPARE(requested.count(), 0);
        QCOMPARE(f.cached.fixed->pendingCount(), kCacheFoldMaxPending - 1);
        f.cached.fixed->appendStroke(line(400));
        QCOMPARE(requested.count(), 1);
        QCOMPARE(f.cached.fixed->pendingCount(), 0);
    }

    // Revue de L10-T4 : au plafond, la vue verse pendant pendingChanged (elle y est branchée la première) ;
    // un observateur branché après n'y voit que l'attente vide, objectCountChanged (émis avant) y voit le
    // plafond. Le banc relève donc les deux (Runner::play, pending_max).
    void theCapIsSeenOnObjectCountChangedOnly()
    {
        PendingFixture f;
        QVERIFY(openPending(f, m_stylus.get()));
        actcraft::render::FixedPageItem* watched = f.cached.fixed;
        int onPending = 0;
        int onCount = 0;
        QObject observer;
        QObject::connect(watched, &actcraft::render::FixedPageItem::pendingChanged, &observer,
                         [&onPending, watched] { onPending = qMax(onPending, watched->pendingCount()); });
        QObject::connect(watched, &actcraft::render::FixedPageItem::objectCountChanged, &observer,
                         [&onCount, watched] { onCount = qMax(onCount, watched->pendingCount()); });
        for (int i = 0; i < kCacheFoldMaxPending; ++i) {
            watched->appendStroke(line(10.0 + i));
        }
        QCOMPARE(watched->pendingCount(), 0);
        QCOMPARE(onPending, kCacheFoldMaxPending - 1);
        QCOMPARE(onCount, kCacheFoldMaxPending);
    }

    // Revue de L10-T4 : une autre page fixe posée sur la vue ; l'ancienne verse ce qui y attendait et quitte
    // le différé (plus personne ne le verserait). La couche regarde toujours l'ancienne : la nouvelle ne
    // diffère pas.
    void replacedFixedPageFoldsAndLeavesTheDeferral()
    {
        PendingFixture f;
        QVERIFY(openPending(f, m_stylus.get()));
        f.cached.fixed->appendStroke(line(100));
        QCOMPARE(f.cached.fixed->pendingCount(), 1);
        QSignalSpy requested(f.cached.view, &PageView::cacheRefreshRequested);
        auto* other = new actcraft::render::FixedPageItem;
        other->setSize(f.cached.fixed->size());
        other->setParentItem(f.cached.fixed->parentItem());
        f.cached.view->setFixedPage(other);
        QCOMPARE(f.cached.fixed->pendingCount(), 0);
        QVERIFY(!f.cached.fixed->deferredFolding());
        QCOMPARE(requested.count(), 1);
        QVERIFY(!other->deferredFolding());
        QVERIFY(!f.cached.view->foldTimerActive());
    }

    // Revue de L10-T4 : la couche d'attente détruite, ce qui attendait est versé et la page quitte le
    // différé.
    void destroyedPendingLayerEndsTheDeferral()
    {
        PendingFixture f;
        QVERIFY(openPending(f, m_stylus.get()));
        f.cached.fixed->appendStroke(line(100));
        QCOMPARE(f.cached.fixed->pendingCount(), 1);
        QSignalSpy requested(f.cached.view, &PageView::cacheRefreshRequested);
        QSignalSpy layerChanged(f.cached.view, &PageView::pendingLayerChanged);
        delete f.layer;
        f.layer = nullptr;
        QVERIFY(!f.cached.view->pendingLayer());
        QCOMPARE(layerChanged.count(), 1);
        QVERIFY(!f.cached.fixed->deferredFolding());
        QCOMPARE(f.cached.fixed->pendingCount(), 0);
        QCOMPARE(requested.count(), 1);
        QVERIFY(!f.cached.view->foldTimerActive());
        f.cached.fixed->appendStroke(line(200));
        QCOMPARE(f.cached.fixed->pendingCount(), 0); // plus rien n'attend
    }

    // Revue de L10-T4 : une couche qui regarde une autre page fixe (ou aucune encore : ordre des liaisons
    // QML) ne met pas la page de la vue en différé ; elle la suit quand sa page change.
    void pendingLayerOfAnotherPageDoesNotDefer()
    {
        CachedFixture f;
        QVERIFY(openCached(f, m_stylus.get()));
        auto* layer = new actcraft::render::PendingLayerItem;
        layer->setSize(f.fixed->size());
        layer->setParentItem(f.fixed->parentItem());
        f.view->setPendingLayer(layer);
        QVERIFY(!f.fixed->deferredFolding());
        layer->setFixedPage(f.fixed);
        QVERIFY(f.fixed->deferredFolding());
        f.fixed->appendStroke(line(100));
        QCOMPARE(f.fixed->pendingCount(), 1);
        auto* other = new actcraft::render::FixedPageItem;
        layer->setFixedPage(other);
        QVERIFY(!f.fixed->deferredFolding());
        QCOMPARE(f.fixed->pendingCount(), 0);
        QVERIFY(!f.view->foldTimerActive());
    }

    void withoutCacheNothingWaits()
    {
        PendingFixture f;
        QVERIFY(openPending(f, m_stylus.get()));
        f.cached.view->setCacheEnabled(false);
        QVERIFY(!f.cached.fixed->deferredFolding());
        f.cached.fixed->appendStroke(line(100));
        QCOMPARE(f.cached.fixed->pendingCount(), 0);
        f.cached.view->setCacheEnabled(true);
        QVERIFY(f.cached.fixed->deferredFolding());
    }

    void contentResizeRefitsAndEmitsScroll()
    {
        Fixture f;
        QVERIFY(open(f));
        f.view->fitPage();
        const qreal before = f.view->zoom();
        QSignalSpy scrolled(f.view, &PageView::scrollChanged);
        f.content->setSize(QSizeF(2048, 1536)); // même élément, page deux fois plus grande
        QVERIFY(scrolled.count() >= 1);
        QVERIFY(qAbs(f.view->zoom() - before / 2) < 1e-6); // « Ajuster » suit la nouvelle taille
    }

    void contentResizeRequestsTheCacheOnce()
    {
        Fixture f;
        QVERIFY(open(f));
        f.view->fitPage();
        QTRY_VERIFY(f.view->cacheReady());
        QSignalSpy requested(f.view, &PageView::cacheRefreshRequested);
        f.content->setSize(QSizeF(800, 600));
        QCOMPARE(requested.count(), 1);
    }

    void contentResizeOutsideFitClampsThePan()
    {
        Fixture f;
        QVERIFY(open(f));
        f.view->setZoom(2.0);
        f.view->scrollTo(1.0, 1.0); // en bas à droite
        QSignalSpy scrolled(f.view, &PageView::scrollChanged);
        f.content->setSize(QSizeF(512, 384)); // la page rapetisse : le défilement reste dans les bornes
        QVERIFY(scrolled.count() >= 1);
        QCOMPARE(f.view->zoom(), 2.0);
        QVERIFY(f.view->scrollPositionX() <= 1.0);
    }

    void settledZoomSetsTheRasterScale()
    {
        DrawFixture f;
        QVERIFY(openDrawing(f, m_stylus.get()));
        f.view->setZoom(2.0);
        const qreal dpr = f.window.effectiveDevicePixelRatio();
        QCOMPARE(f.fixed->rasterScale(), 2.0 * dpr);
        f.view->fitPage();
        QCOMPARE(f.fixed->rasterScale(), f.view->zoom() * dpr);
    }

    // Fluidité : pendant un zoom continu à la molette, la page fixe n'est pas refaite ; l'échelle n'est posée
    // qu'à la fin (minuterie), une seule fois.
    void continuousZoomSetsTheRasterScaleOnlyAtTheEnd()
    {
        DrawFixture f;
        QVERIFY(openDrawing(f, m_stylus.get()));
        f.view->setZoom(1.0);
        const double scale = f.fixed->rasterScale();
        const int rasterizations = f.fixed->rasterizationCount();
        const QPointF pos(500, 300);
        for (int i = 0; i < 4; ++i) {
            QWheelEvent event(pos, f.window.mapToGlobal(pos), QPoint(), QPoint(0, 120), Qt::NoButton,
                              Qt::ControlModifier, Qt::NoScrollPhase, false);
            QCoreApplication::sendEvent(&f.window, &event);
        }
        QVERIFY(f.view->zoom() > 1.0);
        QCOMPARE(f.fixed->rasterScale(), scale); // zoom en cours : rien n'a bougé
        QCOMPARE(f.fixed->rasterizationCount(), rasterizations);
        const qreal dpr = f.window.effectiveDevicePixelRatio();
        QTRY_COMPARE_WITH_TIMEOUT(f.fixed->rasterScale(), f.view->zoom() * dpr, 2000); // fin du zoom
    }

    // Un zoom à la molette est en cours quand la taille de la page change : le redessin dû au changement de
    // taille pose l'échelle du zoom courant, la fin de zoom n'est pas perdue (page nette, pas rastérisée à
    // l'ancienne échelle).
    void contentResizeDuringWheelZoomKeepsTheFinalRasterScale()
    {
        DrawFixture f;
        QVERIFY(openDrawing(f, m_stylus.get()));
        f.view->setZoom(1.0);
        const QPointF pos(500, 300);
        for (int i = 0; i < 3; ++i) {
            QWheelEvent event(pos, f.window.mapToGlobal(pos), QPoint(), QPoint(0, 120), Qt::NoButton,
                              Qt::ControlModifier, Qt::NoScrollPhase, false);
            QCoreApplication::sendEvent(&f.window, &event);
        }
        QVERIFY(f.view->zoom() > 1.0);
        f.view->content()->setSize(QSizeF(800, 600)); // avant la fin de la minuterie du zoom
        const qreal dpr = f.window.effectiveDevicePixelRatio();
        QTRY_COMPARE_WITH_TIMEOUT(f.fixed->rasterScale(), f.view->zoom() * dpr, 2000);
        QTest::qWait(300); // la minuterie, si elle courait encore, est passée : l'échelle est restée la bonne
        QCOMPARE(f.fixed->rasterScale(), f.view->zoom() * dpr);
    }

    // Fluidité : un trait figé n'ajoute qu'un nœud à la scène de la page affichée, même sur la page générée
    // du banc (300 objets) ; jamais une reconstruction de la page (relais par copie de page, ou identifiant
    // déjà pris par la page générée).
    void strokeOnTheGeneratedPageAddsOneNode()
    {
        CachedFixture f;
        QVERIFY(openCached(f, m_stylus.get()));
        DocumentController controller;
        controller.setFixedPage(f.fixed);
        f.view->setDocumentController(&controller);
        controller.loadGeneratedFirstPage();
        QTRY_VERIFY(f.fixed->isReady());
        f.window.grabWindow(); // scène des 300 objets construite
        const int rebuilds = f.fixed->rebuildCount();
        cachedStroke(f, QPointF(300, 200), QPointF(500, 260));
        QTRY_COMPARE(f.fixed->objectCount(), 301);
        f.window.grabWindow();
        QCOMPARE(f.fixed->rebuildCount(), rebuilds);
        QVERIFY(actcraft::core::hasUniqueIds(controller.document()));
    }

    // Contrôleur détruit avant la vue : le stylo ne garde aucune référence vers lui. Le trait va à la page
    // interne de la page fixe (détachée par le contrôleur), comme sans contrôleur : rien n'est perdu en
    // silence, rien n'est écrit dans un objet détruit.
    void strokeAfterTheControllerIsGoneGoesToTheInternalPage()
    {
        DrawFixture f;
        QVERIFY(openDrawing(f, m_stylus.get()));
        auto controller = std::make_unique<DocumentController>();
        controller->setFixedPage(f.fixed);
        f.view->setDocumentController(controller.get());
        controller.reset();
        QVERIFY(f.view->documentController() == nullptr);
        tabletStroke(f, QPointF(300, 300), QPointF(500, 300));
        QTRY_COMPARE(f.fixed->objectCount(), 1);
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

    // H10-9 (L10-T8) : le plafond de la texture du cache suit l'échantillonnage DU CACHE
    // (PageView.cacheSamples, lié à ShaderEffectSource.samples), jamais celui de la fenêtre. Lu par la
    // propriété, comme PageArea.qml.
    void cacheSamplesDefaultFollowsTheRenderer()
    {
        Fixture f;
        QVERIFY(open(f));
        QCOMPARE(f.view->property("cacheSamples").toInt(), 1); // tests en rendu logiciel : pas de MSAA
    }

    void multisampledCacheIsCappedAtHighZoom()
    {
        Fixture f; // fenêtre sans multi-échantillonnage : seul le cache l'est
        QVERIFY(open(f));
        QCOMPARE(f.window.effectiveDevicePixelRatio(), 1.0);
        f.view->setProperty("cacheSamples", 4);
        f.view->setZoom(8.0);
        QCOMPARE(f.view->cacheTextureSize(), QSize(3072, 2304));
        QCOMPARE(f.view->property("cacheSamples").toInt(), 4);
    }

    void multisampledCacheIsNeverBlurredAtHundredPercent()
    {
        Fixture f;
        QVERIFY(open(f));
        QCOMPARE(f.window.effectiveDevicePixelRatio(), 1.0);
        f.view->setProperty("cacheSamples", 4);
        f.content->setSize(QSizeF(3500, 2000)); // page plus grande que le plafond multi-échantillonné
        f.view->setZoom(1.0);
        QCOMPARE(f.view->cacheTextureSize(), QSize(3500, 2000));
        f.view->setZoom(8.0);
        QCOMPARE(f.view->cacheTextureSize(), QSize(3500, 2000)); // plancher : le côté à 100 %
    }

    void singleSampledCacheKeepsTheFullCapEvenIfTheWindowIsMultisampled()
    {
        Fixture f;
        QSurfaceFormat format = f.window.format();
        format.setSamples(4);
        f.window.setFormat(format);
        QVERIFY(open(f));
        f.view->setProperty("cacheSamples", 1);
        f.view->setZoom(8.0);
        QCOMPARE(f.view->cacheTextureSize(), QSize(4096, 3072));
    }
};

QTEST_MAIN(TstPageView)
#include "tst_pageview.moc"
