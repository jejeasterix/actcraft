#include "startup.h"
#include "support/renderhost.h"

#include <actcraft/render/fixedpageitem.h>
#include <actcraft/render/inkitem.h>
#include <actcraft/ui/documentcontroller.h>
#include <actcraft/ui/pageview.h>
#include <actcraft/ui/viewtransform.h>

#include <QElapsedTimer>
#include <QPointingDevice>
#include <QQmlApplicationEngine>
#include <QQuickWindow>
#include <QSignalSpy>
#include <QTabletEvent>
#include <QTest>
#include <QtQml/qqmlextensionplugin.h>

#include <memory>

Q_IMPORT_QML_PLUGIN(ActCraft_UiPlugin)

using namespace actcraft;

namespace {

struct App
{
    QQmlApplicationEngine engine;
    QQuickWindow* window = nullptr;
    ui::PageView* view = nullptr;
    render::FixedPageItem* fixed = nullptr;
    render::InkItem* ink = nullptr;
};

bool start(App& app, bool cacheEnabled)
{
    StartupOptions options;
    options.cacheEnabled = cacheEnabled;
    options.generatedPage = true;
    app.window = loadMainWindow(app.engine, options);
    if (!app.window || !QTest::qWaitForWindowExposed(app.window)) {
        return false;
    }
    app.view = app.window->findChild<ui::PageView*>(QStringLiteral("pageView"));
    app.fixed = app.window->findChild<render::FixedPageItem*>(QStringLiteral("fixedPage"));
    app.ink = app.window->findChild<render::InkItem*>(QStringLiteral("ink"));
    if (!app.view || !app.fixed || !app.ink) {
        return false;
    }
    return QTest::qWaitFor([&] { return app.fixed->isReady() && app.view->cacheReady(); }, 20000);
}

QColor pixelAtPage(App& app, QPointF pagePoint)
{
    const QImage image = app.window->grabWindow();
    const QPointF scene = app.view->pageToScene(pagePoint) * app.window->effectiveDevicePixelRatio();
    return image.pixelColor(scene.toPoint());
}

} // namespace

class TstUiDrawing : public QObject
{
    Q_OBJECT

private:
    std::unique_ptr<QPointingDevice> m_stylus;

    void post(App& app, QEvent::Type type, QPointF page, qreal pressure, Qt::MouseButton button,
              Qt::MouseButtons buttons)
    {
        const QPointF scene = app.view->pageToScene(page);
        QCoreApplication::postEvent(
            app.window, new QTabletEvent(type, m_stylus.get(), scene, app.window->mapToGlobal(scene),
                                         pressure, 0, 0, 0, 0, 0, Qt::NoModifier, button, buttons));
    }

    // Comme post, mais livré tout de suite : rien n'est synchronisé ni dessiné entre deux événements.
    void send(App& app, QEvent::Type type, QPointF page, qreal pressure, Qt::MouseButton button,
              Qt::MouseButtons buttons)
    {
        const QPointF scene = app.view->pageToScene(page);
        QTabletEvent event(type, m_stylus.get(), scene, app.window->mapToGlobal(scene), pressure, 0, 0, 0, 0,
                           0, Qt::NoModifier, button, buttons);
        QCoreApplication::sendEvent(app.window, &event);
    }

    // Trait horizontal de (300, y) à (500, y) ; `probeX` est la colonne de page où l'on lit le pixel.
    void strokeIsDrawnThenFrozen(bool cacheEnabled, qreal zoom = 0.0, qreal y = 670)
    {
        App app;
        QVERIFY(start(app, cacheEnabled));
        QCOMPARE(app.view->cacheEnabled(), cacheEnabled);
        if (zoom > 0.0) {
            app.view->setZoom(zoom);
            QTRY_VERIFY_WITH_TIMEOUT(app.view->cacheReady(), 20000);
        }
        const QPointF probe(350, y);
        QSignalSpy frozen(app.view, &ui::PageView::strokeFrozen);
        post(app, QEvent::TabletPress, QPointF(300, y), 0.7, Qt::LeftButton, Qt::LeftButton);
        for (int i = 1; i <= 100; ++i) {
            post(app, QEvent::TabletMove, QPointF(300 + 2.0 * i, y), 0.7, Qt::NoButton, Qt::LeftButton);
        }
        QTRY_VERIFY_WITH_TIMEOUT(app.ink->pointCount() >= 100, 3000);
        QVERIFY2(colorNear(pixelAtPage(app, probe), QColor(Qt::black), 60), "encre en cours");

        // Décision 10 : entre le relâché et le cache redessiné, aucune image ne doit perdre le trait
        // (encre vidée et trait versé à la page fixe dans le même tour de boucle).
        // Limite : grabWindow() synchronise et dessine sa propre image ; on vérifie l'état de la scène après
        // chaque lot d'événements traité, pas l'image réellement présentée à l'écran.
        QSignalSpy swapped(app.window, &QQuickWindow::frameSwapped);
        post(app, QEvent::TabletRelease, QPointF(500, y), 0.0, Qt::LeftButton, Qt::NoButton);
        QElapsedTimer clock;
        clock.start();
        int checkedFrames = 0;
        while (!(frozen.count() == 1 && app.view->cacheReady()) && clock.elapsed() < 3000) {
            QCoreApplication::processEvents(QEventLoop::AllEvents, 5);
            if (frozen.count() == 1 && !swapped.isEmpty()) {
                swapped.clear();
                ++checkedFrames;
                QVERIFY2(colorNear(pixelAtPage(app, probe), QColor(Qt::black), 60),
                         "trait absent entre le relâché et le cache prêt");
            }
        }
        QCOMPARE(frozen.count(), 1);
        QVERIFY(app.view->cacheReady());
        qInfo("images contrôlées entre relâché et cache prêt : %d", checkedFrames);
        QCOMPARE(app.ink->pointCount(), std::size_t{0});
        QCOMPARE(app.fixed->objectCount(), 301);
        QVERIFY2(colorNear(pixelAtPage(app, probe), QColor(Qt::black), 60), "trait figé");
    }

private slots:
    void initTestCase()
    {
        m_stylus = std::make_unique<QPointingDevice>(
            QStringLiteral("tst-ui-stylus"), 5150, QInputDevice::DeviceType::Stylus,
            QPointingDevice::PointerType::Pen,
            QInputDevice::Capability::Position | QInputDevice::Capability::Pressure, 1, 3);
    }

    void sansCacheOptionIsParsed()
    {
        QVERIFY(parseStartupOptions({QStringLiteral("actcraft")}).cacheEnabled);
        QVERIFY(
            !parseStartupOptions({QStringLiteral("actcraft"), QStringLiteral("--sans-cache")}).cacheEnabled);
    }

    void stylusStrokeWithCache() { strokeIsDrawnThenFrozen(true); }

    void stylusStrokeWithoutCache() { strokeIsDrawnThenFrozen(false); }

    // Zoom 200 % : l'encre et le trait figé tombent au même endroit (le pixel sondé reste noir des deux
    // côtés).
    void stylusStrokeZoomedWithCache() { strokeIsDrawnThenFrozen(true, 2.0, 384); }

    // Lot 10 : le trait reste visible à chaque image entre le relâché, son attente et son versement ; un seul
    // redessin du cache, après l'inactivité.
    void deferredStrokeStaysVisibleThroughTheFold()
    {
        App app;
        QVERIFY(start(app, true));
        const QPointF probe(350, 670);
        QSignalSpy requested(app.view, &ui::PageView::cacheRefreshRequested);
        post(app, QEvent::TabletPress, QPointF(300, 670), 0.7, Qt::LeftButton, Qt::LeftButton);
        for (int i = 1; i <= 100; ++i) {
            post(app, QEvent::TabletMove, QPointF(300 + 2.0 * i, 670), 0.7, Qt::NoButton, Qt::LeftButton);
        }
        post(app, QEvent::TabletRelease, QPointF(500, 670), 0.0, Qt::LeftButton, Qt::NoButton);
        QTRY_COMPARE_WITH_TIMEOUT(app.fixed->pendingCount(), 1, 3000);
        QCOMPARE(requested.count(), 0);
        QSignalSpy swapped(app.window, &QQuickWindow::frameSwapped);
        QElapsedTimer clock;
        clock.start();
        int checked = 0;
        int checkedAfterRequest = 0; // images contrôlées entre la demande de redessin et le cache prêt
        const auto checkFrame = [&] {
            swapped.clear();
            ++checked;
            if (requested.count() > 0) {
                ++checkedAfterRequest;
            }
            QVERIFY2(colorNear(pixelAtPage(app, probe), QColor(Qt::black), 60),
                     "trait absent entre le relâché et le versement achevé");
        };
        while (!(app.fixed->pendingCount() == 0 && app.view->cacheReady()) &&
               clock.elapsed() < ui::kCacheFoldIdleMs + 5000) {
            QCoreApplication::processEvents(QEventLoop::AllEvents, 5);
            if (!swapped.isEmpty()) {
                checkFrame();
            }
        }
        if (!swapped.isEmpty()) {
            checkFrame(); // image échangée dans le même tour que la fin du versement
        }
        QCOMPARE(app.fixed->pendingCount(), 0);
        QVERIFY(app.view->cacheReady());
        QVERIFY2(colorNear(pixelAtPage(app, probe), QColor(Qt::black), 60),
                 "trait absent après le versement");
        QCOMPARE(requested.count(), 1);
        QVERIFY2(checked >= 2, qPrintable(QStringLiteral("images contrôlées : %1").arg(checked)));
        QVERIFY2(checkedAfterRequest >= 1, "aucune image contrôlée pendant le versement");
    }

    void undoOfAPendingStrokeLeavesTheCacheAlone()
    {
        App app;
        QVERIFY(start(app, true));
        auto* controller =
            app.window->findChild<ui::DocumentController*>(QStringLiteral("documentController"));
        QVERIFY(controller);
        const QPointF probe(350, 670);
        post(app, QEvent::TabletPress, QPointF(300, 670), 0.7, Qt::LeftButton, Qt::LeftButton);
        for (int i = 1; i <= 100; ++i) {
            post(app, QEvent::TabletMove, QPointF(300 + 2.0 * i, 670), 0.7, Qt::NoButton, Qt::LeftButton);
        }
        post(app, QEvent::TabletRelease, QPointF(500, 670), 0.0, Qt::LeftButton, Qt::NoButton);
        QTRY_COMPARE_WITH_TIMEOUT(app.fixed->pendingCount(), 1, 3000);
        QSignalSpy requested(app.view, &ui::PageView::cacheRefreshRequested);
        const int rebuilds = app.fixed->rebuildCount();
        controller->undo();
        QCOMPARE(app.fixed->pendingCount(), 0);
        QCOMPARE(requested.count(), 0);
        QTest::qWait(50);
        QCOMPARE(app.fixed->rebuildCount(), rebuilds);
        QVERIFY2(!colorNear(pixelAtPage(app, probe), QColor(Qt::black), 60), "trait annulé encore visible");
    }

    // Lot 10 (revue de L10-T3) : un trait inséré entre une demande de versement et la synchronisation
    // suivante attend au-dessus du cache et reste dessiné. Versé à la fin de ce redessin (après la photo du
    // cache) au lieu de la demande, il ne serait plus dessiné par personne, ni le trait d'avant.
    void strokeInsertedBetweenFoldRequestAndSyncStaysVisible()
    {
        App app;
        QVERIFY(start(app, true));
        const QPointF first(350, 670);
        const QPointF second(350, 640);
        QVERIFY2(!colorNear(pixelAtPage(app, second), QColor(Qt::black), 60), "point sondé déjà noir");
        post(app, QEvent::TabletPress, QPointF(300, 670), 0.7, Qt::LeftButton, Qt::LeftButton);
        for (int i = 1; i <= 100; ++i) {
            post(app, QEvent::TabletMove, QPointF(300 + 2.0 * i, 670), 0.7, Qt::NoButton, Qt::LeftButton);
        }
        post(app, QEvent::TabletRelease, QPointF(500, 670), 0.0, Qt::LeftButton, Qt::NoButton);
        QTRY_COMPARE_WITH_TIMEOUT(app.fixed->pendingCount(), 1, 3000);
        QSignalSpy requested(app.view, &ui::PageView::cacheRefreshRequested);
        app.view->requestCacheRefresh(); // demande de versement : le premier trait passe au cache
        QCOMPARE(app.fixed->pendingCount(), 0);
        // Second trait, livré avant toute synchronisation (événements envoyés, pas postés).
        send(app, QEvent::TabletPress, QPointF(300, 640), 0.7, Qt::LeftButton, Qt::LeftButton);
        for (int i = 1; i <= 100; ++i) {
            send(app, QEvent::TabletMove, QPointF(300 + 2.0 * i, 640), 0.7, Qt::NoButton, Qt::LeftButton);
        }
        send(app, QEvent::TabletRelease, QPointF(500, 640), 0.0, Qt::LeftButton, Qt::NoButton);
        QCOMPARE(app.fixed->pendingCount(), 1);
        QTRY_VERIFY_WITH_TIMEOUT(app.view->cacheReady(), 5000);
        QCOMPARE(requested.count(), 1);
        QCOMPARE(app.fixed->pendingCount(), 1); // il attend le prochain versement
        QVERIFY2(colorNear(pixelAtPage(app, first), QColor(Qt::black), 60), "trait versé absent du cache");
        QVERIFY2(colorNear(pixelAtPage(app, second), QColor(Qt::black), 60),
                 "trait inséré pendant la demande absent");
    }

    void mouseLeftButtonDraws()
    {
        App app;
        QVERIFY(start(app, true));
        const QPoint a = app.view->pageToScene(QPointF(300, 640)).toPoint();
        const QPoint b = app.view->pageToScene(QPointF(500, 640)).toPoint();
        QTest::mousePress(app.window, Qt::LeftButton, Qt::NoModifier, a);
        for (int i = 1; i <= 10; ++i) {
            QTest::mouseMove(app.window, a + (b - a) * i / 10);
        }
        QTest::mouseRelease(app.window, Qt::LeftButton, Qt::NoModifier, b);
        QTRY_COMPARE_WITH_TIMEOUT(app.fixed->objectCount(), 301, 3000);
    }
};

QTEST_MAIN(TstUiDrawing)
#include "tst_ui_drawing.moc"
