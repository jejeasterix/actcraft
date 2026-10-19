#include "startup.h"
#include "support/renderhost.h"

#include <actcraft/render/fixedpageitem.h>
#include <actcraft/render/inkitem.h>
#include <actcraft/tools/tool.h>
#include <actcraft/ui/documentcontroller.h>
#include <actcraft/ui/pageview.h>
#include <actcraft/ui/viewtransform.h>

#include <QDir>
#include <QElapsedTimer>
#include <QKeyEvent>
#include <QPointingDevice>
#include <QQmlApplicationEngine>
#include <QQuickWindow>
#include <QSettings>
#include <QSignalSpy>
#include <QTabletEvent>
#include <QTemporaryDir>
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
    QPointingDevice* m_touch = nullptr;
    QTemporaryDir m_settingsDir;

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

    // Trait du stylet de (300, y) à (500, y) : appui, 100 mouvements, relâché, postés.
    void postStroke(App& app, qreal y)
    {
        post(app, QEvent::TabletPress, QPointF(300, y), 0.7, Qt::LeftButton, Qt::LeftButton);
        for (int i = 1; i <= 100; ++i) {
            post(app, QEvent::TabletMove, QPointF(300 + 2.0 * i, y), 0.7, Qt::NoButton, Qt::LeftButton);
        }
        post(app, QEvent::TabletRelease, QPointF(500, y), 0.0, Qt::LeftButton, Qt::NoButton);
    }

    // Le même trait, livré tout de suite (send).
    void sendStroke(App& app, qreal y)
    {
        send(app, QEvent::TabletPress, QPointF(300, y), 0.7, Qt::LeftButton, Qt::LeftButton);
        for (int i = 1; i <= 100; ++i) {
            send(app, QEvent::TabletMove, QPointF(300 + 2.0 * i, y), 0.7, Qt::NoButton, Qt::LeftButton);
        }
        send(app, QEvent::TabletRelease, QPointF(500, y), 0.0, Qt::LeftButton, Qt::NoButton);
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
        // Main.qml porte ToolHost (réglages par outil) : jamais le profil réel.
        QVERIFY(m_settingsDir.isValid());
        QSettings::setDefaultFormat(QSettings::IniFormat);
        QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, m_settingsDir.path());
        const QSettings probe;
        QVERIFY2(QDir::fromNativeSeparators(probe.fileName()).startsWith(m_settingsDir.path()),
                 qPrintable(probe.fileName()));
        m_touch = QTest::createTouchDevice();
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
        postStroke(app, 670);
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
        postStroke(app, 670);
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
        postStroke(app, 670);
        QTRY_COMPARE_WITH_TIMEOUT(app.fixed->pendingCount(), 1, 3000);
        QSignalSpy requested(app.view, &ui::PageView::cacheRefreshRequested);
        app.view->requestCacheRefresh(); // demande de versement : le premier trait passe au cache
        QCOMPARE(app.fixed->pendingCount(), 0);
        // Second trait, livré avant toute synchronisation (événements envoyés, pas postés).
        sendStroke(app, 640);
        QCOMPARE(app.fixed->pendingCount(), 1);
        // Le redessin demandé dure autant que le délai d'inactivité en debug et rendu logiciel (279 à 520 ms
        // relevés par la fumée du banc) : sans entrée, la minuterie verserait le second trait avant la fin.
        // Une touche (KeyPress, vue par le filtre de la fenêtre : elle repousse le versement, sans rien
        // prendre) toutes les 100 ms et après chaque image, jusqu'au cache prêt. Après chaque image : le
        // redessin bloque le fil principal, la minuterie échue partirait dans le même passage de la boucle.
        QElapsedTimer clock;
        clock.start();
        QElapsedTimer sinceKey;
        sinceKey.start();
        const auto pressShift = [&app, &sinceKey] {
            QKeyEvent press(QEvent::KeyPress, Qt::Key_Shift, Qt::NoModifier);
            QCoreApplication::sendEvent(app.window, &press);
            QKeyEvent release(QEvent::KeyRelease, Qt::Key_Shift, Qt::NoModifier);
            QCoreApplication::sendEvent(app.window, &release);
            sinceKey.restart();
        };
        const auto afterFrame =
            QObject::connect(app.window, &QQuickWindow::frameSwapped, app.window, pressShift);
        while (!app.view->cacheReady() && clock.elapsed() < 5000) {
            QCoreApplication::processEvents(QEventLoop::AllEvents, 5);
            if (sinceKey.elapsed() >= 100) {
                pressShift();
            }
        }
        QObject::disconnect(afterFrame);
        QVERIFY(app.view->cacheReady());
        QCOMPARE(requested.count(), 1);
        QCOMPARE(app.fixed->pendingCount(), 1); // il attend le prochain versement
        QVERIFY2(colorNear(pixelAtPage(app, first), QColor(Qt::black), 60), "trait versé absent du cache");
        QVERIFY2(colorNear(pixelAtPage(app, second), QColor(Qt::black), 60),
                 "trait inséré pendant la demande absent");
    }

    // Revue de clôture du lot 10 : un trait qui sort de la page vers la marge (vue « Ajuster ») n'y est pas
    // dessiné par la couche d'attente, rognée à la page comme le cache (sourceRect vide : le rectangle de la
    // page). Le pixel de la marge sous le trait reste le fond de la marge pendant l'attente et après le
    // versement : rien n'y apparaît ni n'y disparaît. L'encre en cours (InkLayer, non rognée depuis le lot 2)
    // n'est pas changée : elle déborde sur la marge pendant le tracé (relevé : noir sur la marge), comme au
    // lot 2 ; son pixel est relevé (qInfo), pas exigé.
    void pendingStrokeIsClippedToThePage()
    {
        App app;
        QVERIFY(start(app, true));
        const double zoom = app.view->zoom();
        const double pageWidth = app.fixed->width();
        const qreal y = 400;
        // 14 pixels de vue à droite de la page : dans la marge d'« Ajuster » (kViewMargin = 32 au moins).
        const QPointF outside(pageWidth + 14 / zoom, y);
        const QColor margin = pixelAtPage(app, outside);
        QVERIFY2(!colorNear(margin, QColor(Qt::black), 60), "marge déjà noire");
        const QPointF from(pageWidth - 120, y);
        const QPointF to(pageWidth + 28 / zoom, y);
        post(app, QEvent::TabletPress, from, 0.7, Qt::LeftButton, Qt::LeftButton);
        for (int i = 1; i <= 50; ++i) {
            post(app, QEvent::TabletMove, from + (to - from) * (i / 50.0), 0.7, Qt::NoButton, Qt::LeftButton);
        }
        QTRY_VERIFY_WITH_TIMEOUT(app.ink->pointCount() >= 50, 3000);
        const QColor inking = pixelAtPage(app, outside);
        qInfo("encre en cours sur la marge : %s (fond de la marge %s)", qPrintable(inking.name()),
              qPrintable(margin.name()));
        post(app, QEvent::TabletRelease, to, 0.0, Qt::LeftButton, Qt::NoButton);
        QTRY_COMPARE_WITH_TIMEOUT(app.fixed->pendingCount(), 1, 3000);
        const QColor waiting = pixelAtPage(app, outside);
        QVERIFY2(
            colorNear(waiting, margin, 8),
            qPrintable(
                QStringLiteral("en attente : %1 sur la marge (fond %2)").arg(waiting.name(), margin.name())));
        QTRY_VERIFY_WITH_TIMEOUT(app.fixed->pendingCount() == 0 && app.view->cacheReady(),
                                 ui::kCacheFoldIdleMs + 5000);
        const QColor folded = pixelAtPage(app, outside);
        QVERIFY2(colorNear(folded, margin, 8),
                 qPrintable(
                     QStringLiteral("versé : %1 sur la marge (fond %2)").arg(folded.name(), margin.name())));
        QVERIFY2(colorNear(pixelAtPage(app, QPointF(pageWidth - 60, y)), QColor(Qt::black), 60),
                 "trait absent de la page");
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

    // S3 tenu avec ToolHost posé (Main.qml) : un doigt trace avec l'outil stylo de l'utilisateur.
    void fingerStrokeStillDraws()
    {
        App app;
        QVERIFY(start(app, true));
        QVERIFY(app.view->tools());
        QVERIFY(app.view->currentTool());
        QCOMPARE(app.view->currentTool()->id(), QStringLiteral("pen"));
        const QPoint a = app.view->pageToScene(QPointF(300, 600)).toPoint();
        const QPoint b = app.view->pageToScene(QPointF(500, 600)).toPoint();
        QTest::touchEvent(app.window, m_touch).press(0, a);
        for (int i = 1; i <= 10; ++i) {
            QTest::touchEvent(app.window, m_touch).move(0, a + (b - a) * i / 10);
        }
        QTest::touchEvent(app.window, m_touch).release(0, b);
        QTRY_COMPARE_WITH_TIMEOUT(app.fixed->objectCount(), 301, 3000);
    }
};

QTEST_MAIN(TstUiDrawing)
#include "tst_ui_drawing.moc"
