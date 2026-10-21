#include "support/lot7registry.h"
#include "support/presentationharness.h"
#include "tickercontroller.h"

#include <actcraft/features/ticker.h>
#include <actcraft/ui/pageview.h>

#include <QImage>
#include <QSettings>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QtTest>

namespace {

// Tout le fichier dans cet espace de noms anonyme (exécutable unique : aucun nom global hors du registre).
using actcraft::ui::TickerController;
using presentationharness::MainWindow;
using presentationharness::Pointer;

TickerController* tickerOf(const MainWindow& main)
{
    return main.window() ? main.window()->findChild<TickerController*>(QStringLiteral("ticker")) : nullptr;
}

// Cherché dans la racine du message : main.item rend le premier élément de ce nom dans tout l'arbre.
QQuickItem* inTicker(const MainWindow& main, const QString& name)
{
    return presentationharness::findItem(main.item(QStringLiteral("tickerRoot")), name);
}

QRectF sceneRect(const QQuickItem* item)
{
    return item->mapRectToScene(QRectF(0, 0, item->width(), item->height()));
}

// Image de la bande telle que la fenêtre la montre (position du texte posée par le fil de rendu comprise).
QImage shot(const MainWindow& main, const QQuickItem* band)
{
    const qreal ratio = main.window()->devicePixelRatio();
    const QRectF zone = sceneRect(band);
    return main.window()->grabWindow().copy(QRectF(zone.topLeft() * ratio, zone.size() * ratio).toRect());
}

// Point de la fenêtre à la hauteur sceneY, à l'abscisse de page pageX ; faux s'il n'est pas sur la page.
bool onPage(const MainWindow& main, qreal pageX, qreal sceneY, QPointF* scene)
{
    const QPointF page = main.pageView()->sceneToPage(QPointF(main.window()->width() / 2.0, sceneY));
    *scene = main.pageView()->pageToScene(QPointF(pageX, page.y()));
    return page.y() > 20 && page.y() < 748;
}

// Trait au stylet comme Qt : chaque événement de tablette part non accepté ; si la fenêtre ne l'accepte pas
// (le filtre de la vue ne prend pas un appui couvert), Qt le rejoue en souris
// (Qt::AA_SynthesizeMouseForUnhandledTabletEvents, actif par défaut), et seulement alors.
void stylusStroke(QQuickWindow* window, QPointF from, QPointF to, int steps = 8)
{
    const auto send = [window](QEvent::Type type, QPointF point) {
        const bool release = type == QEvent::TabletRelease;
        const Qt::MouseButtons buttons = release ? Qt::NoButton : Qt::LeftButton;
        const Qt::MouseButton changed = type == QEvent::TabletMove ? Qt::NoButton : Qt::LeftButton;
        QTabletEvent event(type, presentationharness::stylusDevice(), point, window->mapToGlobal(point),
                           release ? 0.0 : 0.5, 0, 0, 0, 0, 0, Qt::NoModifier, changed, buttons);
        event.setAccepted(false); // comme QGuiApplication avant l'envoi
        QCoreApplication::sendEvent(window, &event);
        if (event.isAccepted()) {
            return;
        }
        if (type == QEvent::TabletPress) {
            QTest::mousePress(window, Qt::LeftButton, Qt::NoModifier, point.toPoint());
        } else if (type == QEvent::TabletMove) {
            QTest::mouseMove(window, point.toPoint());
        } else {
            QTest::mouseRelease(window, Qt::LeftButton, Qt::NoModifier, point.toPoint());
        }
    };
    send(QEvent::TabletPress, from);
    for (int i = 1; i <= steps; ++i) {
        send(QEvent::TabletMove, from + (to - from) * (static_cast<qreal>(i) / steps));
    }
    send(QEvent::TabletRelease, to);
    QCoreApplication::processEvents();
}

void strokeWith(const MainWindow& main, Pointer pointer, QPointF from, QPointF to)
{
    if (pointer == Pointer::Stylus) {
        stylusStroke(main.window(), from, to);
    } else {
        presentationharness::strokeAt(main.window(), pointer, from, to);
    }
}

// Message en boucle lancé par sa commande, comme le menu ou Ctrl+Maj+T.
bool startLoop(const MainWindow& main, TickerController* ticker, int position)
{
    ticker->setLoop(true);
    ticker->setPosition(position);
    return main.run(QStringLiteral("tool.ticker")) && ticker->running();
}

class TstUiTicker : public QObject
{
    Q_OBJECT

private slots:
    // Réglages par défaut : ceux de la logique au premier lancement ; keepAsDefault() les garde
    // (QSettings, tools/ticker) pour un contrôleur recréé ; un simple réglage ne garde rien.
    void tickerSettingsKeptInTheSettings()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        QSettings::setDefaultFormat(QSettings::IniFormat);
        QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, dir.path());
        const actcraft::features::TickerSettings defaults;
        {
            TickerController ticker;
            QCOMPARE(ticker.text(), defaults.text);
            QCOMPARE(ticker.speed(), defaults.speed);
            ticker.setText(QStringLiteral("Essai"));
        }
        {
            TickerController ticker;
            QCOMPARE(ticker.text(), defaults.text);
            ticker.setText(QStringLiteral("Bonjour"));
            ticker.setSpeed(99);
            ticker.setPosition(TickerController::Bottom);
            ticker.setLoop(true);
            QCOMPARE(ticker.speed(), 10);
            ticker.keepAsDefault();
        }
        QVERIFY(QSettings().contains(QStringLiteral("tools/ticker/message")));
        TickerController ticker;
        QCOMPARE(ticker.text(), QStringLiteral("Bonjour"));
        QCOMPARE(ticker.speed(), 10);
        QCOMPARE(ticker.position(), static_cast<int>(TickerController::Bottom));
        QVERIFY(ticker.loop());
    }

    // Surcouche : aucune demande de redessin du cache pendant 1 s de défilement en boucle ; un trait au
    // stylet sous la bande pendant le défilement est versé normalement, et le message continue.
    void tickerNeverRefreshesTheCache()
    {
        MainWindow main;
        QVERIFY(main.window() != nullptr);
        TickerController* ticker = tickerOf(main);
        QVERIFY(ticker != nullptr);
        QSignalSpy spy(main.pageView(), &actcraft::ui::PageView::cacheRefreshRequested);
        QVERIFY(spy.isValid());
        const int before = main.pageObjectCount();

        QVERIFY(startLoop(main, ticker, TickerController::Center));
        QQuickItem* band = inTicker(main, QStringLiteral("tickerBand"));
        QVERIFY(band != nullptr && band->isVisible());
        QTest::qWait(1000);
        QCOMPARE(spy.count(), 0);

        QPointF from;
        QPointF to;
        const qreal below = sceneRect(band).bottom() + 80;
        QVERIFY(onPage(main, 300, below, &from) && onPage(main, 700, below, &to));
        stylusStroke(main.window(), from, to);
        QTRY_COMPARE_WITH_TIMEOUT(main.pageObjectCount(), before + 1, 5000);
        QVERIFY(ticker->running());
    }

    // Le texte bouge sur le fil de rendu (XAnimator), jamais par le fil de l'interface : la bande montrée
    // change d'une image à l'autre, et l'abscisse du texte côté QML ne change pas pendant le passage. Une
    // minuterie ou une animation de propriété réécrirait x à chaque image (rouge) ; un texte immobile
    // laisserait la bande identique (rouge).
    void tickerMovesOnTheRenderThread()
    {
        MainWindow main;
        QVERIFY(main.window() != nullptr);
        TickerController* ticker = tickerOf(main);
        QVERIFY(ticker != nullptr);
        QVERIFY(startLoop(main, ticker, TickerController::Top));
        QQuickItem* band = inTicker(main, QStringLiteral("tickerBand"));
        QQuickItem* message = inTicker(main, QStringLiteral("tickerMessage"));
        QVERIFY(band != nullptr && message != nullptr);
        QTRY_VERIFY(band->isVisible());

        QSignalSpy moved(message, &QQuickItem::xChanged);
        QVERIFY(moved.isValid());
        QTest::qWait(200);
        const QImage first = shot(main, band);
        QTest::qWait(400);
        const QImage second = shot(main, band);
        QVERIFY(!first.isNull() && first.size() == second.size());
        QVERIFY2(first != second, "la bande n'a pas changé : le texte ne défile pas");
        QCOMPARE(moved.count(), 0);
        QVERIFY(ticker->running());
    }

    // Un seul passage : à la fin de l'animation, le message s'arrête et la bande disparaît.
    void singlePassHidesTheBand()
    {
        MainWindow main;
        QVERIFY(main.window() != nullptr);
        TickerController* ticker = tickerOf(main);
        QVERIFY(ticker != nullptr);
        ticker->setLoop(false);
        ticker->setSpeed(10);
        QVERIFY(main.run(QStringLiteral("tool.ticker")));
        QQuickItem* band = inTicker(main, QStringLiteral("tickerBand"));
        QQuickItem* message = inTicker(main, QStringLiteral("tickerMessage"));
        QVERIFY(band != nullptr && message != nullptr);
        QVERIFY(ticker->running());
        QTRY_VERIFY(band->isVisible());
        const int expected = ticker->durationMs(band->width(), message->width());
        QTest::qWait(expected / 2);
        QVERIFY(ticker->running());
        QTRY_VERIFY_WITH_TIMEOUT(!ticker->running(), expected + 3000);
        QVERIFY(!band->isVisible());
    }

    // Clic droit sur le message en boucle : il s'arrête, la bande disparaît.
    void rightClickStopsTheLoop()
    {
        MainWindow main;
        QVERIFY(main.window() != nullptr);
        TickerController* ticker = tickerOf(main);
        QVERIFY(ticker != nullptr);
        QVERIFY(startLoop(main, ticker, TickerController::Center));
        QQuickItem* band = inTicker(main, QStringLiteral("tickerBand"));
        QVERIFY(band != nullptr);
        QTRY_VERIFY(band->isVisible());
        QTest::qWait(300);
        QVERIFY(ticker->running());
        QTest::mouseClick(main.window(), Qt::RightButton, Qt::NoModifier, sceneRect(band).center().toPoint());
        QTRY_VERIFY(!ticker->running());
        QVERIFY(!band->isVisible());
    }

    // Pause : le texte reste où il était (la bande ne change plus) ; reprise : il repart de là.
    void pauseHoldsTheMessage()
    {
        MainWindow main;
        QVERIFY(main.window() != nullptr);
        TickerController* ticker = tickerOf(main);
        QVERIFY(ticker != nullptr);
        QVERIFY(startLoop(main, ticker, TickerController::Top));
        QQuickItem* band = inTicker(main, QStringLiteral("tickerBand"));
        QQuickItem* message = inTicker(main, QStringLiteral("tickerMessage"));
        QVERIFY(band != nullptr && message != nullptr);
        QTRY_VERIFY(band->isVisible());
        QTest::qWait(400);

        ticker->togglePause();
        QVERIFY(ticker->paused());
        QTest::qWait(100);
        QVERIFY2(message->x() < band->width() - 10, qPrintable(QString::number(message->x())));
        const QImage held = shot(main, band);
        QTest::qWait(300);
        QCOMPARE(shot(main, band), held);

        ticker->togglePause();
        QVERIFY(!ticker->paused());
        QTest::qWait(300);
        QVERIFY(shot(main, band) != held);
        QVERIFY(ticker->running());
    }

    // Le message est du texte brut : une balise saisie s'affiche telle quelle, ombre comprise.
    void messageIsPlainText()
    {
        MainWindow main;
        QVERIFY(main.window() != nullptr);
        TickerController* ticker = tickerOf(main);
        QVERIFY(ticker != nullptr);
        const QString raw = QStringLiteral("<b>gras</b> &amp; <img src=\"x.png\">");
        ticker->setText(raw);
        ticker->setShadowOffset(3);
        QVERIFY(main.run(QStringLiteral("tool.ticker")));
        for (const QString& name : {QStringLiteral("tickerText"), QStringLiteral("tickerShadow")}) {
            QQuickItem* text = inTicker(main, name);
            QVERIFY2(text != nullptr, qPrintable(name));
            QCOMPARE(text->property("textFormat").toInt(), static_cast<int>(Qt::PlainText));
            QCOMPARE(text->property("text").toString(), raw);
        }
    }

    void tickerBlocksTheStylus_data() { presentationharness::addPointerRows(); }

    // La bande est une couverture (addInputCover, limitée à la bande) : rien n'est versé dessous, quel que
    // soit le périphérique, et le message continue ; sous la bande, le même trait est versé. Souris : le clic
    // droit hors de la bande ouvre le menu de la page (la couverture ne dépasse pas la bande), sur la bande
    // il n'ouvre rien et arrête le message (effet du gestionnaire).
    void tickerBlocksTheStylus()
    {
        QFETCH(Pointer, pointer);
        MainWindow main;
        QVERIFY(main.window() != nullptr);
        TickerController* ticker = tickerOf(main);
        QVERIFY(ticker != nullptr);
        QSignalSpy menu(main.pageView(), &actcraft::ui::PageView::contextMenuRequested);
        QVERIFY(menu.isValid());
        const int before = main.pageObjectCount();
        QVERIFY(startLoop(main, ticker, TickerController::Center));
        QQuickItem* band = inTicker(main, QStringLiteral("tickerBand"));
        QVERIFY(band != nullptr);
        QTRY_VERIFY(band->isVisible());
        const QRectF zone = sceneRect(band);

        // Sur la bande : aucun trait, le message continue.
        QPointF from;
        QPointF to;
        QVERIFY(onPage(main, 300, zone.center().y(), &from) && onPage(main, 700, zone.center().y(), &to));
        QVERIFY(zone.contains(from) && zone.contains(to));
        strokeWith(main, pointer, from, to);
        QTest::qWait(150);
        QCOMPARE(main.pageObjectCount(), before);
        QVERIFY(ticker->running());

        // Sous la bande : le même trait est versé, le message continue.
        QPointF belowFrom;
        QPointF belowTo;
        const qreal below = zone.bottom() + 80;
        QVERIFY(onPage(main, 300, below, &belowFrom) && onPage(main, 700, below, &belowTo));
        strokeWith(main, pointer, belowFrom, belowTo);
        QTRY_COMPARE_WITH_TIMEOUT(main.pageObjectCount(), before + 1, 5000);
        QVERIFY(ticker->running());
        QCOMPARE(menu.count(), 0);

        if (pointer != Pointer::Mouse) {
            return;
        }
        // Clic droit hors de la bande : la vue le reçoit (demande du menu de la page, talon du socle qui
        // n'ouvre rien), le message continue.
        QTest::mouseClick(main.window(), Qt::RightButton, Qt::NoModifier, belowFrom.toPoint());
        QTRY_COMPARE(menu.count(), 1);
        QVERIFY(ticker->running());

        // Clic droit sur la bande : aucun menu de la page, le message s'arrête.
        QTest::mouseClick(main.window(), Qt::RightButton, Qt::NoModifier, zone.center().toPoint());
        QTRY_VERIFY(!ticker->running());
        QCOMPARE(menu.count(), 1);
    }
};

} // namespace

ACTCRAFT_LOT7_TEST("ticker", TstUiTicker)
#include "tst_ui_ticker.moc"
