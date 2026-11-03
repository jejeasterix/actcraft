#include "clockcontroller.h"
#include "support/lot7registry.h"
#include "support/presentationharness.h"

#include <actcraft/ui/documentcontroller.h>

#include <QSignalSpy>
#include <QStringList>
#include <QVariantMap>
#include <QtTest>

namespace {

using actcraft::ui::ClockController;
using actcraft::ui::DocumentController;
using presentationharness::MainWindow;
using presentationharness::Pointer;

DocumentController* controllerOf(const MainWindow& main)
{
    return main.window() ? main.window()->findChild<DocumentController*>(QStringLiteral("documentController"))
                         : nullptr;
}

ClockController* clockOf(const MainWindow& main)
{
    return main.window() ? main.window()->findChild<ClockController*>(QStringLiteral("clock")) : nullptr;
}

QPointF centreOf(const QQuickItem* item)
{
    return item->mapToScene(QPointF(item->width() / 2, item->height() / 2));
}

// Appui bref au stylet, comme Qt : l'événement de tablette part non accepté ; si la fenêtre ne l'accepte pas
// (pas de gestionnaire de tablette sous le point), Qt le rejoue en souris
// (Qt::AA_SynthesizeMouseForUnhandledTabletEvents, actif par défaut). Faux si la fenêtre a pris un événement
// : Qt ne rejouerait rien.
bool stylusTap(QQuickWindow* window, QPointF at)
{
    bool replayed = true;
    for (const QEvent::Type type : {QEvent::TabletPress, QEvent::TabletRelease}) {
        const bool press = type == QEvent::TabletPress;
        QTabletEvent event(type, presentationharness::stylusDevice(), at, window->mapToGlobal(at),
                           press ? 0.5 : 0.0, 0, 0, 0, 0, 0, Qt::NoModifier, Qt::LeftButton,
                           press ? Qt::LeftButton : Qt::NoButton);
        event.setAccepted(false); // comme QGuiApplication avant l'envoi
        QCoreApplication::sendEvent(window, &event);
        if (event.isAccepted()) {
            replayed = false;
            continue;
        }
        if (press) {
            QTest::mousePress(window, Qt::LeftButton, Qt::NoModifier, at.toPoint());
        } else {
            QTest::mouseRelease(window, Qt::LeftButton, Qt::NoModifier, at.toPoint());
        }
    }
    QCoreApplication::processEvents();
    return replayed;
}

// Ouvre le panneau par sa commande et le pose, sa barre de titre centrée sur le point de page (400, 200).
QQuickItem* openClock(const MainWindow& main)
{
    if (!main.run(QStringLiteral("tool.clock"))) {
        return nullptr;
    }
    QQuickItem* panel = main.item(QStringLiteral("clockRoot"));
    QQuickItem* layer = main.item(QStringLiteral("presentationLayer"));
    if (!panel || !layer || !panel->isVisible()) {
        return nullptr;
    }
    panel->setPosition(layer->mapFromScene(main.pageView()->pageToScene(QPointF(400, 200)) -
                                           QPointF(panel->width() / 2, 22)));
    return panel;
}

// Tout le fichier dans cet espace de noms anonyme (exécutable unique : aucun nom global hors du registre).
class TstUiClock : public QObject
{
    Q_OBJECT

private slots:
    // H7-14, F-290 : les quatre actions de fin de la spécification, dans son ordre : « aucune », « page
    // suivante », « page précédente », « rétablir la page » (commande page.revert du lot 3, L7-T34). Remplace
    // endActionsAreTheThreeOfThisLot (revue M4). La liste de la boîte est celle du contrôleur.
    void endActionsAreTheFourOfTheSpecification()
    {
        MainWindow main;
        QVERIFY(main.window() != nullptr);
        QVERIFY(openClock(main) != nullptr);
        ClockController* clock = clockOf(main);
        QVERIFY(clock != nullptr);

        QStringList commands;
        QStringList labels;
        for (const QVariant& entry : clock->endActions()) {
            const QVariantMap map = entry.toMap();
            commands << map.value(QStringLiteral("command")).toString();
            labels << map.value(QStringLiteral("label")).toString();
        }
        QCOMPARE(commands, (QStringList{QString(), QStringLiteral("page.next"),
                                        QStringLiteral("page.previous"), QStringLiteral("page.revert")}));
        QCOMPARE(labels,
                 (QStringList{QStringLiteral("Aucune"), QStringLiteral("Page suivante"),
                              QStringLiteral("Page précédente"), QStringLiteral("Rétablir la page")}));

        // « rétablir la page » est acceptée ; une action hors liste est refusée, l'action en cours reste.
        clock->setEndAction(QStringLiteral("page.revert"));
        QCOMPARE(clock->endAction(), QStringLiteral("page.revert"));
        clock->setEndAction(QStringLiteral("file.quit"));
        QCOMPARE(clock->endAction(), QStringLiteral("page.revert"));

        // La boîte offre les mêmes quatre choix.
        QVERIFY(main.run(QStringLiteral("tool.clock")));
        QQuickItem* settings = main.item(QStringLiteral("clockSettingsButton"));
        QVERIFY(settings != nullptr);
        QTest::mouseClick(main.window(), Qt::LeftButton, Qt::NoModifier, centreOf(settings).toPoint());
        QCoreApplication::processEvents();
        QQuickItem* combo = main.item(QStringLiteral("countdownEndAction"));
        QVERIFY(combo != nullptr);
        QCOMPARE(combo->property("count").toInt(), 4);
    }

    // F-290 : « rétablir la page » à l'échéance : la page reprend son état d'ouverture (document neuf :
    // vierge) par la commande page.revert du lot 3, une seule fois ; un annuler rend le trait.
    void countdownCanRevertThePage()
    {
        MainWindow main;
        DocumentController* controller = controllerOf(main);
        QVERIFY(controller != nullptr);
        QVERIFY(openClock(main) != nullptr);
        ClockController* clock = clockOf(main);
        QVERIFY(clock != nullptr);
        QSignalSpy ended(clock, &ClockController::countdownEnded);
        const int before = main.pageObjectCount();
        presentationharness::strokeAt(main.window(), presentationharness::Pointer::Stylus,
                                      main.pageView()->pageToScene(QPointF(40, 700)),
                                      main.pageView()->pageToScene(QPointF(140, 700)));
        QTRY_COMPARE(main.pageObjectCount(), before + 1);

        QVERIFY(clock->applyCountdownText(QStringLiteral("00:00:02")));
        clock->setEndAction(QStringLiteral("page.revert"));
        QCOMPARE(clock->endAction(), QStringLiteral("page.revert"));
        clock->startCountdown();
        clock->setClockNowOffsetMs(2500);
        QTRY_COMPARE(main.pageObjectCount(), before);
        QCOMPARE(controller->pageCount(), 1);
        QCOMPARE(ended.count(), 1);

        QVERIFY(main.run(QStringLiteral("edit.undo")));
        QTRY_COMPARE(main.pageObjectCount(), before + 1);
    }

    // 3 s, « page suivante » : à l'échéance la page 2 existe et est affichée ; une seule fin (le temps est
    // injecté par clockNowOffsetMs, aucune attente de plusieurs secondes).
    void countdownEndGoesToNextPage()
    {
        MainWindow main;
        DocumentController* controller = controllerOf(main);
        QVERIFY(controller != nullptr);
        QVERIFY(openClock(main) != nullptr);
        ClockController* clock = clockOf(main);
        QVERIFY(clock != nullptr);
        QSignalSpy ended(clock, &ClockController::countdownEnded);

        QVERIFY(clock->applyCountdownText(QStringLiteral("00:00:03")));
        clock->setEndAction(QStringLiteral("page.next"));
        clock->startCountdown();
        QCOMPARE(clock->remainingText(), QStringLiteral("00:00:03"));
        clock->setClockNowOffsetMs(1000);
        QTRY_COMPARE(clock->remainingText(), QStringLiteral("00:00:02"));
        QCOMPARE(controller->pageCount(), 1);

        clock->setClockNowOffsetMs(3500);
        QTRY_COMPARE(controller->pageCount(), 2);
        QCOMPARE(controller->currentIndex(), 1);
        QCOMPARE(main.item(QStringLiteral("pageIndicator"))->property("text").toString(),
                 QStringLiteral("2 / 2"));
        QCOMPARE(clock->remainingText(), QStringLiteral("00:00:00"));

        clock->setClockNowOffsetMs(9000);
        QTest::qWait(600);
        QCOMPARE(controller->pageCount(), 2);
        QCOMPARE(ended.count(), 1);
    }

    // Répéter + page suivante : une page vierge de plus à chaque fin.
    void repeatAddsBlankPages()
    {
        MainWindow main;
        DocumentController* controller = controllerOf(main);
        QVERIFY(controller != nullptr);
        QVERIFY(openClock(main) != nullptr);
        ClockController* clock = clockOf(main);
        QVERIFY(clock != nullptr);

        QVERIFY(clock->applyCountdownText(QStringLiteral("00:00:02")));
        clock->setEndAction(QStringLiteral("page.next"));
        clock->setRepeat(true);
        clock->startCountdown();
        clock->setClockNowOffsetMs(2100);
        QTRY_COMPARE(controller->pageCount(), 2);
        clock->setClockNowOffsetMs(4100);
        QTRY_COMPARE(controller->pageCount(), 3);
        QCOMPARE(controller->currentIndex(), 2);
    }

    // « Aucune » ne fait rien ; « page précédente » revient d'une page.
    void noneDoesNothingAndPreviousGoesBack()
    {
        MainWindow main;
        DocumentController* controller = controllerOf(main);
        QVERIFY(controller != nullptr);
        QVERIFY(openClock(main) != nullptr);
        ClockController* clock = clockOf(main);
        QVERIFY(clock != nullptr);
        QSignalSpy ended(clock, &ClockController::countdownEnded);

        QVERIFY(clock->applyCountdownText(QStringLiteral("00:00:01")));
        clock->startCountdown();
        clock->setClockNowOffsetMs(1500);
        QTRY_COMPARE(ended.count(), 1);
        QCOMPARE(controller->pageCount(), 1);

        QVERIFY(main.run(QStringLiteral("page.next")));
        QCOMPARE(controller->currentIndex(), 1);
        clock->setEndAction(QStringLiteral("page.previous"));
        clock->startCountdown();
        clock->setClockNowOffsetMs(3000);
        QTRY_COMPARE(controller->currentIndex(), 0);
        QCOMPARE(controller->pageCount(), 2);
    }

    // La pause fige le compte ; la reprise le relance ; une saisie invalide est refusée.
    void pauseFreezesAndInvalidTextIsRefused()
    {
        ClockController clock;
        QVERIFY(!clock.applyCountdownText(QStringLiteral("xx")));
        QVERIFY(clock.applyCountdownText(QStringLiteral("00:01:00")));
        QCOMPARE(clock.countdownSeconds(), 60);
        clock.startCountdown();
        clock.setClockNowOffsetMs(10'000);
        clock.togglePause();
        QVERIFY(clock.paused());
        clock.setClockNowOffsetMs(40'000);
        QCOMPARE(clock.remainingText(), QStringLiteral("00:00:50"));
        clock.togglePause();
        QVERIFY(!clock.paused());
        clock.setClockNowOffsetMs(50'000);
        QCOMPARE(clock.remainingText(), QStringLiteral("00:00:40"));
    }

    void clockBlocksTheStylus_data() { presentationharness::addPointerRows(); }

    // Panneau ouvert, il couvre la page quel que soit le périphérique : le bouton de pause bascule la pause
    // sans tracer, un coin agrandit le panneau en gardant ses proportions sans tracer, la barre déplace sans
    // tracer, le corps retient le trait ; hors du panneau, un trait est versé. À la souris, le clic droit sur
    // le panneau n'ouvre pas le menu de la page, hors du panneau il l'ouvre.
    void clockBlocksTheStylus()
    {
        QFETCH(Pointer, pointer);
        MainWindow main;
        QVERIFY(main.window() != nullptr);
        QQuickItem* panel = openClock(main);
        QVERIFY(panel != nullptr);
        ClockController* clock = clockOf(main);
        QVERIFY(clock != nullptr);
        QQuickItem* pause = main.item(QStringLiteral("clockPauseButton"));
        QQuickItem* corner = main.item(QStringLiteral("clockCornerBottomRight"));
        QVERIFY(pause != nullptr && corner != nullptr);
        const int before = main.pageObjectCount();
        QVERIFY(!clock->paused());

        // Bouton de pause : la pause bascule, aucun trait.
        const QPointF button = centreOf(pause);
        if (pointer == Pointer::Stylus) {
            QVERIFY2(stylusTap(main.window(), button), "tablette non acceptée : rejouée en souris par Qt");
        } else {
            presentationharness::strokeAt(main.window(), pointer, button - QPointF(6, 0),
                                          button + QPointF(6, 0), 3);
        }
        QTRY_VERIFY(clock->paused());
        QTest::qWait(50);
        QCOMPARE(main.pageObjectCount(), before);

        // Coin : le panneau grandit en gardant ses proportions, aucun trait.
        const qreal width = panel->width();
        const qreal height = panel->height();
        const QPointF grip = centreOf(corner);
        presentationharness::strokeAt(main.window(), pointer, grip, grip + QPointF(60, 45));
        QTest::qWait(50);
        QCOMPARE(main.pageObjectCount(), before);
        QVERIFY2(panel->width() > width + 20, "panneau agrandi par son coin");
        QVERIFY2(qAbs(panel->width() / panel->height() - width / height) < 0.01, "proportions gardées");

        // Barre de titre : le panneau se déplace, aucun trait.
        const QPointF start = panel->position();
        const QPointF bar = panel->mapToScene(QPointF(100, 22));
        presentationharness::strokeAt(main.window(), pointer, bar, bar + QPointF(-60, 40));
        QTest::qWait(50);
        QCOMPARE(main.pageObjectCount(), before);
        QVERIFY2(panel->position() != start, "panneau déplacé par sa barre");

        // Corps, sans gestionnaire : seule la couverture retient le trait.
        const QPointF body = panel->mapToScene(QPointF(30, 70));
        presentationharness::strokeAt(main.window(), pointer, body, body + QPointF(100, 0));
        QTest::qWait(50);
        QCOMPARE(main.pageObjectCount(), before);

        if (pointer == Pointer::Mouse) {
            // Le clic droit, que la vue teste par sa couverture (menu de la page), discrimine.
            QSignalSpy menu(main.pageView(), &actcraft::ui::PageView::contextMenuRequested);
            QVERIFY(menu.isValid());
            QTest::mouseClick(main.window(), Qt::RightButton, Qt::NoModifier, body.toPoint());
            QTest::qWait(50);
            QCOMPARE(menu.count(), 0);
            QTest::mouseClick(main.window(), Qt::RightButton, Qt::NoModifier,
                              main.pageView()->pageToScene(QPointF(40, 700)).toPoint());
            QTRY_COMPARE(menu.count(), 1);
            // Lot 3 : le clic droit ouvre le menu de la page. Échap, raccourci de « Sélectionner »
            // (commands.json), ne le ferme pas ici et changerait l'outil (consigné) ; ouvert, le menu
            // prendrait l'appui du trait : un clic hors du menu, sur le panneau, le ferme.
            QObject* pageMenu = main.window()->findChild<QObject*>(QStringLiteral("pageContextMenu"));
            QVERIFY(pageMenu);
            // Lot 4 : le menu de la page (16 entrées) couvre désormais le corps du panneau ; le clic de
            // fermeture va sur la barre de titre, hors du menu (sans glisser, il ne déplace pas le panneau).
            const QPointF outside = panel->mapToScene(QPointF(100, 22));
            auto* menuContent = pageMenu->property("contentItem").value<QQuickItem*>();
            QVERIFY(menuContent);
            QQuickItem* menuBox = menuContent->parentItem() ? menuContent->parentItem() : menuContent;
            QVERIFY2(
                !menuBox->mapRectToScene(QRectF(0, 0, menuBox->width(), menuBox->height())).contains(outside),
                "point de fermeture hors du menu");
            QTest::mouseClick(main.window(), Qt::LeftButton, Qt::NoModifier, outside.toPoint());
            QTRY_VERIFY(!pageMenu->property("visible").toBool());
        }

        // Hors du panneau : un trait est versé.
        presentationharness::strokeAt(main.window(), pointer, main.pageView()->pageToScene(QPointF(40, 700)),
                                      main.pageView()->pageToScene(QPointF(140, 700)));
        QTRY_COMPARE(main.pageObjectCount(), before + 1);
    }

    // Revue M7 : un coin haut tiré de 300 px vers l'extérieur, panneau posé à 10 px du bord haut. Le panneau
    // grandit en gardant ses proportions et son coin opposé, mais ne sort pas de la couche : son bord haut
    // (barre de titre, croix) reste atteignable. Sans borne, l'échelle 1,43 le ferait monter de 137 px au-dessus.
    void cornerGripsKeepThePanelInside()
    {
        MainWindow main;
        QVERIFY(main.window() != nullptr);
        QQuickItem* panel = openClock(main);
        QVERIFY(panel != nullptr);
        QQuickItem* layer = panel->parentItem();
        QVERIFY(layer != nullptr);
        for (const QString& name : {QStringLiteral("clockCornerTopLeft"), QStringLiteral("clockCornerTopRight")}) {
            QQuickItem* corner = presentationharness::findItem(panel, name);
            QVERIFY2(corner != nullptr, qPrintable(name));
            const bool left = name.endsWith(QStringLiteral("Left"));
            panel->setSize(QSizeF(280, 340));
            panel->setPosition(QPointF(left ? layer->width() - 280 - 40 : 40, 10));
            const QRectF before(panel->position(), panel->size());
            const QPointF grip = centreOf(corner);
            presentationharness::strokeAt(main.window(), Pointer::Mouse, grip,
                                          grip + QPointF(left ? -300 : 300, 0));
            const QRectF after(panel->position(), panel->size());
            QVERIFY2(after.width() > before.width() + 1, qPrintable(name + QStringLiteral(" : agrandi")));
            QVERIFY2(qAbs(after.width() / after.height() - before.width() / before.height()) < 0.01,
                     qPrintable(name + QStringLiteral(" : proportions gardées")));
            QVERIFY2(qAbs(after.bottom() - before.bottom()) < 0.5,
                     qPrintable(name + QStringLiteral(" : bord bas fixe")));
            QVERIFY2(qAbs(left ? after.right() - before.right() : after.left() - before.left()) < 0.5,
                     qPrintable(name + QStringLiteral(" : bord opposé fixe")));
            QVERIFY2(after.top() >= -0.5 && after.left() >= -0.5 && after.right() <= layer->width() + 0.5,
                     qPrintable(name + QStringLiteral(" : dans la couche, ") + QString::number(after.top())));
        }
    }
};

} // namespace

ACTCRAFT_LOT7_TEST("clock", TstUiClock)
#include "tst_ui_clock.moc"
