#include "support/lot7registry.h"
#include "support/presentationharness.h"

#include <QtTest>

namespace {

using presentationharness::MainWindow;
using presentationharness::Pointer;

// Tout le fichier dans cet espace de noms anonyme (exécutable unique : aucun nom global hors du registre).
QObject* zoomOf(const MainWindow& main)
{
    return main.window() ? main.window()->findChild<QObject*>(QStringLiteral("pageZoom")) : nullptr;
}

QPointF inWindow(const MainWindow& main, QPointF pagePoint)
{
    return main.pageView()->pageToScene(pagePoint);
}

// Empreinte du document, lue par la racine de la surcouche (jamais par le contrôleur de zoom).
QString fingerprintOf(const MainWindow& main)
{
    QQuickItem* root = main.item(QStringLiteral("pageZoomRoot"));
    const auto controller = root ? root->property("documentController").value<QObject*>() : nullptr;
    return controller ? controller->property("fingerprint").toString() : QString();
}

void pressAt(QQuickWindow* window, Pointer pointer, QPointF point, Qt::MouseButton button = Qt::LeftButton)
{
    switch (pointer) {
    case Pointer::Stylus:
        presentationharness::sendTablet(window, QEvent::TabletPress, point);
        break;
    case Pointer::Finger:
        QTest::touchEvent(window, presentationharness::touchDevice()).press(0, point.toPoint());
        presentationharness::settleTouch();
        break;
    case Pointer::Mouse:
        QTest::mousePress(window, button, Qt::NoModifier, point.toPoint());
        break;
    }
}

void releaseAt(QQuickWindow* window, Pointer pointer, QPointF point, Qt::MouseButton button = Qt::LeftButton)
{
    switch (pointer) {
    case Pointer::Stylus:
        presentationharness::sendTablet(window, QEvent::TabletRelease, point);
        break;
    case Pointer::Finger:
        QTest::touchEvent(window, presentationharness::touchDevice()).release(0, point.toPoint());
        presentationharness::settleTouch();
        break;
    case Pointer::Mouse:
        QTest::mouseRelease(window, button, Qt::NoModifier, point.toPoint());
        break;
    }
    QCoreApplication::processEvents();
}

constexpr int kHoldMs = 600;

class TstUiPageZoom : public QObject
{
    Q_OBJECT

private slots:
    // Aucune demande de redessin pendant 600 ms de maintien ; exactement une après le relâché (la fin de zoom
    // de la vue).
    void pageZoomRefreshesTheCacheOnceAtRelease()
    {
        MainWindow main;
        QVERIFY(main.window() != nullptr);
        QVERIFY(zoomOf(main) != nullptr);
        QVERIFY(main.run(QStringLiteral("view.page-zoom")));
        QVERIFY(zoomOf(main)->property("active").toBool());
        const qreal before = main.pageView()->zoom();
        QSignalSpy spy(main.pageView(), &actcraft::ui::PageView::cacheRefreshRequested);
        QVERIFY(spy.isValid());

        const QPointF center = inWindow(main, QPointF(512, 384));
        QElapsedTimer held;
        held.start();
        pressAt(main.window(), Pointer::Mouse, center);
        // Maintien d'au moins 600 ms, prolongé tant que le zoom n'a pas pris 20 % (machine chargée).
        QTRY_VERIFY2(held.elapsed() >= kHoldMs && main.pageView()->zoom() > before * 1.2,
                     qPrintable(QString::number(main.pageView()->zoom())));
        QCOMPARE(spy.count(), 0);
        releaseAt(main.window(), Pointer::Mouse, center);

        QTRY_COMPARE_WITH_TIMEOUT(spy.count(), 1, 3000);
        QTest::qWait(500);
        QCOMPARE(spy.count(), 1);
    }

    // Zoom, panoramique et dézoom n'écrivent rien : empreinte et objets de la page identiques.
    void pageZoomLeavesTheDocumentUnchanged()
    {
        MainWindow main;
        QVERIFY(main.window() != nullptr);
        QVERIFY(main.run(QStringLiteral("view.page-zoom")));
        const QString fingerprint = fingerprintOf(main);
        QVERIFY(!fingerprint.isEmpty());
        const int before = main.pageObjectCount();
        const qreal startZoom = main.pageView()->zoom();

        const QPointF center = inWindow(main, QPointF(512, 384));
        pressAt(main.window(), Pointer::Mouse, center);
        QTRY_VERIFY(main.pageView()->zoom() > startZoom * 1.1);
        releaseAt(main.window(), Pointer::Mouse, center);
        const qreal zoomed = main.pageView()->zoom();
        QVERIFY(zoomed > startZoom * 1.1);

        // Chaque appui suivant loin du précédent (au-delà de kDoubleClickPixels) : aucun ne compte comme un
        // double appui, qui remettrait la page à l'écran et passerait pour un panoramique ou un dézoom.
        const QPointF panFrom = center + QPointF(-120, 0);
        const QPointF panBefore = main.pageView()->pan();
        presentationharness::strokeAt(main.window(), Pointer::Mouse, panFrom, panFrom + QPointF(-90, -60));
        QTRY_VERIFY(main.pageView()->pan() != panBefore);
        QCOMPARE(main.pageView()->zoom(), zoomed);

        // Relecture du lot 7 (M10) : le bouton droit tenu dézoome peu à peu, tant qu'il est tenu (un double
        // appui ne ferait qu'un saut). Mutant : sens du bouton droit ignoré (`press(…, false)`) → le zoom
        // grandit ; sous l'ancien test (dézoom au même point que l'appui d'avant), ce mutant passait.
        const QPointF outAt = center + QPointF(120, 0);
        pressAt(main.window(), Pointer::Mouse, outAt, Qt::RightButton);
        QTRY_VERIFY2(main.pageView()->zoom() < zoomed * 0.95,
                     qPrintable(QString::number(main.pageView()->zoom())));
        const qreal during = main.pageView()->zoom();
        QTRY_VERIFY2(main.pageView()->zoom() < during * 0.95,
                     qPrintable(QString::number(main.pageView()->zoom())));
        releaseAt(main.window(), Pointer::Mouse, outAt, Qt::RightButton);
        QVERIFY(main.pageView()->zoom() < during);

        QTest::qWait(300);
        QCOMPARE(main.pageObjectCount(), before);
        QCOMPARE(fingerprintOf(main), fingerprint);
    }

    void pageZoomBlocksTheStylus_data() { presentationharness::addPointerRows(); }

    // La surcouche couvre la page : un appui tenu zoome (effet du gestionnaire lu) et ne verse aucun trait,
    // quel que soit le périphérique ; à la souris, le bouton droit n'ouvre pas non plus le menu contextuel.
    void pageZoomBlocksTheStylus()
    {
        QFETCH(Pointer, pointer);
        MainWindow main;
        QVERIFY(main.window() != nullptr);
        QVERIFY(main.run(QStringLiteral("view.page-zoom")));
        const QString fingerprint = fingerprintOf(main);
        const int before = main.pageObjectCount();
        const qreal startZoom = main.pageView()->zoom();
        QSignalSpy menu(main.pageView(), &actcraft::ui::PageView::contextMenuRequested);
        QVERIFY(menu.isValid());

        const QPointF center = inWindow(main, QPointF(512, 384));
        pressAt(main.window(), pointer, center);
        QTest::qWait(kHoldMs);
        const qreal held = main.pageView()->zoom();
        releaseAt(main.window(), pointer, center);
        QVERIFY2(held > startZoom * 1.2, qPrintable(QString::number(held)));

        // Un trait complet (appui, déplacements, relâché) fait un panoramique, jamais un trait de la page.
        const QPointF panBefore = main.pageView()->pan();
        presentationharness::strokeAt(main.window(), pointer, center, center + QPointF(-120, -80));
        QTest::qWait(100);
        QVERIFY(main.pageView()->pan() != panBefore);

        if (pointer == Pointer::Mouse) {
            pressAt(main.window(), pointer, center, Qt::RightButton);
            QTest::qWait(150);
            releaseAt(main.window(), pointer, center, Qt::RightButton);
            QCOMPARE(menu.count(), 0);
        }
        QTest::qWait(200);
        QCOMPARE(main.pageObjectCount(), before);
        QCOMPARE(fingerprintOf(main), fingerprint);
        QCOMPARE(menu.count(), 0);
    }

    // Appui long droit : pris par la couverture du zoom (test direct au clic droit), il dézoome ; sans le
    // mode, le même clic droit demande bien le menu contextuel (le signal est observable).
    void rightHoldIsTakenByTheCover()
    {
        MainWindow main;
        QVERIFY(main.window() != nullptr);
        QSignalSpy menu(main.pageView(), &actcraft::ui::PageView::contextMenuRequested);
        QVERIFY(menu.isValid());
        const QPointF center = inWindow(main, QPointF(512, 384));

        QTest::mouseClick(main.window(), Qt::RightButton, Qt::NoModifier, center.toPoint());
        QCOMPARE(menu.count(), 1);
        // Lot 3 : le clic droit ouvre le menu de la page. Échap, raccourci de « Sélectionner »
        // (commands.json), ne le ferme pas ici et changerait l'outil (consigné) ; ouvert, le menu
        // prendrait l'appui droit tenu qui suit : un clic hors du menu le ferme.
        QObject* pageMenu = main.window()->findChild<QObject*>(QStringLiteral("pageContextMenu"));
        QVERIFY(pageMenu);
        QTest::mouseClick(main.window(), Qt::LeftButton, Qt::NoModifier,
                          inWindow(main, QPointF(100, 100)).toPoint());
        QTRY_VERIFY(!pageMenu->property("visible").toBool());

        QVERIFY(main.run(QStringLiteral("view.page-zoom")));
        const int before = main.pageObjectCount();
        const qreal startZoom = main.pageView()->zoom();
        pressAt(main.window(), Pointer::Mouse, center, Qt::RightButton);
        QTest::qWait(kHoldMs);
        const qreal held = main.pageView()->zoom();
        releaseAt(main.window(), Pointer::Mouse, center, Qt::RightButton);
        QVERIFY2(held < startZoom * 0.8, qPrintable(QString::number(held)));
        QCOMPARE(menu.count(), 1); // aucun menu de plus
        QCOMPARE(main.pageObjectCount(), before);
    }

    // Deux appuis rapprochés reviennent à « Ajuster ».
    void doubleClickFitsThePage()
    {
        MainWindow main;
        QVERIFY(main.window() != nullptr);
        QVERIFY(main.run(QStringLiteral("view.page-zoom")));
        const qreal fitZoom = main.pageView()->zoom();
        const QPointF center = inWindow(main, QPointF(512, 384));

        pressAt(main.window(), Pointer::Mouse, center);
        QTest::qWait(500);
        releaseAt(main.window(), Pointer::Mouse, center);
        QVERIFY(main.pageView()->zoom() > fitZoom * 1.2);

        QTest::mouseClick(main.window(), Qt::LeftButton, Qt::NoModifier, center.toPoint());
        QTest::mouseClick(main.window(), Qt::LeftButton, Qt::NoModifier, center.toPoint());
        QTRY_VERIFY_WITH_TIMEOUT(qFuzzyCompare(main.pageView()->zoom(), fitZoom), 3000);
    }
};

} // namespace

ACTCRAFT_LOT7_TEST("pagezoom", TstUiPageZoom)
#include "tst_ui_pagezoom.moc"
