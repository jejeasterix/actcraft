#include <actcraft/core/document.h>
#include <actcraft/core/model.h>
#include <actcraft/render/fixedpageitem.h>
#include <actcraft/render/inkitem.h>
#include <actcraft/ui/documentcontroller.h>
#include <actcraft/ui/pageview.h>

#include <QPointingDevice>
#include <QQmlComponent>
#include <QQmlEngine>
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
};

QTEST_MAIN(TstPageView)
#include "tst_pageview.moc"
