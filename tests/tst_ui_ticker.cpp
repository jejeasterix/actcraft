#include "support/lot7registry.h"
#include "support/presentationharness.h"
#include "tickercontroller.h"

#include <actcraft/features/ticker.h>
#include <actcraft/ui/pageview.h>

#include <QElapsedTimer>
#include <QFile>
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

// Trait au stylet dans la zone de la bande, à sa hauteur : vrai s'il est versé (nombre d'objets + 1).
bool strokeVersedAt(const MainWindow& main, const QRectF& zone, int before)
{
    QPointF from;
    QPointF to;
    if (!onPage(main, 300, zone.center().y(), &from) || !onPage(main, 700, zone.center().y(), &to) ||
        !zone.contains(from)) {
        return false;
    }
    stylusStroke(main.window(), from, to);
    return QTest::qWaitFor([&] { return main.pageObjectCount() == before + 1; }, 5000);
}

// Centre d'un élément, dans la fenêtre.
QPointF centreOf(const QQuickItem* item)
{
    return item->mapToScene(QPointF(item->width() / 2.0, item->height() / 2.0));
}

// Clic de souris au centre d'un élément (bouton de la fenêtre d'édition).
void clickItem(const MainWindow& main, const QQuickItem* item)
{
    QTest::mouseClick(main.window(), Qt::LeftButton, Qt::NoModifier, centreOf(item).toPoint());
}

// Fenêtre d'édition ouverte par la commande tool.ticker (une racine : le nom est unique).
QQuickItem* openDialog(const MainWindow& main)
{
    if (!main.run(QStringLiteral("tool.ticker"))) {
        return nullptr;
    }
    QQuickItem* dialog = main.item(QStringLiteral("tickerDialog"));
    return dialog && dialog->isVisible() ? dialog : nullptr;
}

// Élément nommé dans la fenêtre d'édition.
QQuickItem* inDialog(QQuickItem* dialog, const QString& name)
{
    return presentationharness::findItem(dialog, name);
}

void writeFile(const QString& path, const QByteArray& data)
{
    QFile file(path);
    QVERIFY(file.open(QIODevice::WriteOnly));
    QCOMPARE(file.write(data), data.size());
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

    // Réglages relus : mêmes contrôles que les modificateurs (taille de police positive, couleurs valides,
    // ombre positive ou nulle, vitesse bornée) ; une valeur refusée garde celle de la logique.
    void storedSettingsAreChecked()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        QSettings::setDefaultFormat(QSettings::IniFormat);
        QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, dir.path());
        actcraft::features::TickerSettings bad;
        bad.text = QStringLiteral("Altéré");
        bad.fontPointSize = 0;
        bad.shadowOffset = -3;
        bad.speed = 40;
        QSettings().setValue(QStringLiteral("tools/ticker/message"),
                             QString::fromUtf8(actcraft::features::tickerToJson(bad)));
        const QByteArray stored =
            QSettings().value(QStringLiteral("tools/ticker/message")).toString().toUtf8();
        QVERIFY(actcraft::features::tickerFromJson(stored).has_value());

        const actcraft::features::TickerSettings defaults;
        TickerController ticker;
        QCOMPARE(ticker.text(), QStringLiteral("Altéré")); // le bloc est bien relu
        QCOMPARE(ticker.fontPointSize(), defaults.fontPointSize);
        QCOMPARE(ticker.shadowOffset(), 0);
        QCOMPARE(ticker.speed(), 10);
        QVERIFY(ticker.textColor().isValid() && ticker.shadowColor().isValid() &&
                ticker.backgroundColor().isValid());
    }

    // Surcouche : aucune demande de redessin du cache pendant plus d'un passage en boucle (retour de boucle
    // compris) ni à l'arrêt ; un trait au stylet sous la bande pendant le défilement est versé normalement,
    // et le message continue.
    void tickerNeverRefreshesTheCache()
    {
        MainWindow main;
        QVERIFY(main.window() != nullptr);
        TickerController* ticker = tickerOf(main);
        QVERIFY(ticker != nullptr);
        QSignalSpy spy(main.pageView(), &actcraft::ui::PageView::cacheRefreshRequested);
        QVERIFY(spy.isValid());
        const int before = main.pageObjectCount();

        ticker->setSpeed(10);
        QVERIFY(startLoop(main, ticker, TickerController::Center));
        QQuickItem* band = inTicker(main, QStringLiteral("tickerBand"));
        QQuickItem* message = inTicker(main, QStringLiteral("tickerMessage"));
        QVERIFY(band != nullptr && message != nullptr && band->isVisible());
        const int pass = ticker->durationMs(band->width(), message->width());
        QTest::qWait(pass + 600);
        QVERIFY(ticker->running()); // un passage fini, la boucle a repris
        QTest::mouseClick(main.window(), Qt::RightButton, Qt::NoModifier, sceneRect(band).center().toPoint());
        QTRY_VERIFY(!ticker->running());
        QTest::qWait(200);
        QCOMPARE(spy.count(), 0);

        QVERIFY(main.run(QStringLiteral("tool.ticker")));
        QVERIFY(ticker->running());
        QTest::qWait(200);

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

    // Un seul passage : à la fin de l'animation, le message s'arrête et la bande disparaît. La vitesse
    // changée en cours de passage vaut aussitôt : le passage repart de sa place, et le reste dure distance /
    // vitesse nouvelle. Après la fin, la zone de l'ancienne bande reçoit de nouveau les traits.
    void singlePassHidesTheBand()
    {
        MainWindow main;
        QVERIFY(main.window() != nullptr);
        TickerController* ticker = tickerOf(main);
        QVERIFY(ticker != nullptr);
        const int before = main.pageObjectCount();
        ticker->setLoop(false);
        ticker->setPosition(TickerController::Center);
        ticker->setSpeed(4);
        QVERIFY(main.run(QStringLiteral("tool.ticker")));
        QQuickItem* band = inTicker(main, QStringLiteral("tickerBand"));
        QQuickItem* message = inTicker(main, QStringLiteral("tickerMessage"));
        QVERIFY(band != nullptr && message != nullptr);
        QVERIFY(ticker->running());
        QTRY_VERIFY(band->isVisible());
        QTest::qWait(500);

        ticker->setSpeed(10);
        QElapsedTimer clock;
        clock.start();
        const qreal at = message->x(); // place écrite par l'animateur arrêté
        const qint64 expected = qRound64((at + message->width()) / ticker->pixelsPerSecond() * 1000.0);
        QTRY_VERIFY_WITH_TIMEOUT(!ticker->running(), static_cast<int>(3 * expected + 3000));
        const qint64 took = clock.elapsed();
        QVERIFY2(
            took >= expected * 7 / 10 - 200 && took <= expected * 13 / 10 + 500,
            qPrintable(
                QStringLiteral("reste du passage : %1 ms pour %2 ms attendues").arg(took).arg(expected)));
        QVERIFY2(at < band->width() - 20, qPrintable(QString::number(at)));
        QVERIFY(!band->isVisible());
        QVERIFY(strokeVersedAt(main, sceneRect(band), before));
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
        const int before = main.pageObjectCount();
        QTest::mouseClick(main.window(), Qt::RightButton, Qt::NoModifier, sceneRect(band).center().toPoint());
        QTRY_VERIFY(!ticker->running());
        QVERIFY(!band->isVisible());
        // Message arrêté : la zone de l'ancienne bande n'est plus couverte, le trait y est versé.
        QVERIFY(strokeVersedAt(main, sceneRect(band), before));
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

        const qreal heldX = message->x();
        ticker->togglePause();
        QVERIFY(!ticker->paused());
        QElapsedTimer clock;
        clock.start();
        QTest::qWait(300);
        QVERIFY(shot(main, band) != held);
        QVERIFY(ticker->running());

        // Remis en pause : le texte est allé à gauche de sa place tenue, de la distance parcourue depuis la
        // reprise (un départ du bord droit le mettrait à droite).
        ticker->togglePause();
        const qreal seconds = static_cast<qreal>(clock.elapsed()) / 1000.0;
        const qreal pps = ticker->pixelsPerSecond();
        const qreal x = message->x();
        QVERIFY2(x < heldX - pps * qMax(0.0, seconds - 0.25) + 1 && x > heldX - pps * (seconds + 0.2),
                 qPrintable(QStringLiteral("x %1, tenu %2, %3 s").arg(x).arg(heldX).arg(seconds)));
        QVERIFY(x < heldX);
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

    // Un texte (ou une police) dont le bloc gardé dépasserait 64 Kio est refusé à la saisie : les autres
    // réglages gardés restent relus (avant : bloc refusé à la relecture, tous les réglages perdus sans un
    // mot).
    void oversizedTextIsRefusedAtInput()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        QSettings::setDefaultFormat(QSettings::IniFormat);
        QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, dir.path());
        const actcraft::features::TickerSettings defaults;
        {
            TickerController ticker;
            ticker.setSpeed(9);
            ticker.setLoop(true);
            ticker.setText(QString(70 * 1024, QLatin1Char('x')));
            QCOMPARE(ticker.text(), defaults.text);
            ticker.setFontFamily(QString(70 * 1024, QLatin1Char('f')));
            QCOMPARE(ticker.fontFamily(), defaults.fontFamily);
            // Sous la taille d'un fichier mais écrit avec des échappements (retours à la ligne) : refusé
            // aussi.
            ticker.setText(QString(40 * 1024, QLatin1Char('\n')));
            QCOMPARE(ticker.text(), defaults.text);
            ticker.setText(QStringLiteral("Bonjour"));
            ticker.setText(QString(actcraft::features::kMaxTickerFileBytes, QLatin1Char('y')));
            QCOMPARE(ticker.text(), QStringLiteral("Bonjour")); // le texte gardé est celui d'avant
            ticker.keepAsDefault();
        }
        TickerController fresh;
        QCOMPARE(fresh.text(), QStringLiteral("Bonjour"));
        QCOMPARE(fresh.speed(), 9);
        QVERIFY(fresh.loop());
    }

    // Boucle coupée en plein passage : le passage en cours finit, puis le message s'arrête (un animateur sans
    // fin ne finirait jamais). 
    void loopSwitchedOffDuringAPassEndsIt()
    {
        MainWindow main;
        QVERIFY(main.window() != nullptr);
        TickerController* ticker = tickerOf(main);
        QVERIFY(ticker != nullptr);
        ticker->setSpeed(10);
        QVERIFY(startLoop(main, ticker, TickerController::Center));
        QQuickItem* band = inTicker(main, QStringLiteral("tickerBand"));
        QQuickItem* message = inTicker(main, QStringLiteral("tickerMessage"));
        QVERIFY(band != nullptr && message != nullptr);
        QTRY_VERIFY(band->isVisible());
        const qint64 pass = ticker->durationMs(band->width(), message->width());
        QElapsedTimer clock;
        clock.start();
        QTest::qWait(300);
        ticker->setLoop(false);
        QTRY_VERIFY_WITH_TIMEOUT(!ticker->running(), static_cast<int>(pass + 3000));
        const qint64 took = clock.elapsed();
        QVERIFY2(took >= pass * 8 / 10 - 300 && took <= pass * 12 / 10 + 600,
                 qPrintable(QStringLiteral("%1 ms pour un passage de %2 ms").arg(took).arg(pass)));
    }

    // Texte changé en plein passage : la sortie suit la nouvelle largeur (le passage dure (écran + texte) /
    // vitesse du nouveau texte, sans saut de place).
    void textChangedDuringAPassMovesTheExit()
    {
        MainWindow main;
        QVERIFY(main.window() != nullptr);
        TickerController* ticker = tickerOf(main);
        QVERIFY(ticker != nullptr);
        ticker->setLoop(false);
        ticker->setPosition(TickerController::Center);
        ticker->setSpeed(10);
        QVERIFY(main.run(QStringLiteral("tool.ticker")));
        QQuickItem* band = inTicker(main, QStringLiteral("tickerBand"));
        QQuickItem* message = inTicker(main, QStringLiteral("tickerMessage"));
        QVERIFY(band != nullptr && message != nullptr);
        QTRY_VERIFY(band->isVisible());
        QElapsedTimer clock;
        clock.start();
        QTest::qWait(300);
        const qreal oldWidth = message->width();
        ticker->setText(QStringLiteral("Votre message ici ").repeated(6));
        QTRY_VERIFY(message->width() > oldWidth * 3);
        const qreal speed = ticker->pixelsPerSecond();
        const qint64 oldPass = qRound64((band->width() + oldWidth) / speed * 1000.0);
        const qint64 newPass = qRound64((band->width() + message->width()) / speed * 1000.0);
        QVERIFY(newPass > oldPass + 2000);
        QTRY_VERIFY_WITH_TIMEOUT(!ticker->running(), static_cast<int>(newPass + 3000));
        const qint64 took = clock.elapsed();
        QVERIFY2(took >= newPass * 9 / 10 - 300 && took <= newPass * 11 / 10 + 600,
                 qPrintable(QStringLiteral("%1 ms pour un passage de %2 ms (%3 avant)")
                                .arg(took)
                                .arg(newPass)
                                .arg(oldPass)));
    }

    // « Appliquer » garde les réglages comme défaut (un contrôleur recréé les relit) et ferme la boîte.
    void applyKeepsTheDefault()
    {
        MainWindow main;
        QVERIFY(main.window() != nullptr);
        TickerController* ticker = tickerOf(main);
        QVERIFY(ticker != nullptr);
        QQuickItem* dialog = openDialog(main);
        QVERIFY(dialog != nullptr);
        QQuickItem* apply = inDialog(dialog, QStringLiteral("tickerApply"));
        QVERIFY(apply != nullptr);
        QVERIFY(!QSettings().contains(QStringLiteral("tools/ticker/message")));

        ticker->setText(QStringLiteral("Appliqué"));
        ticker->setSpeed(7);
        ticker->setLoop(true);
        clickItem(main, apply);
        QTRY_VERIFY(!dialog->isVisible());
        TickerController fresh;
        QCOMPARE(fresh.text(), QStringLiteral("Appliqué"));
        QCOMPARE(fresh.speed(), 7);
        QVERIFY(fresh.loop());
    }

    // « Essayer » joue le message avec les réglages courants sans rien garder, et la boîte reste ouverte.
    void tryDoesNotKeepTheDefault()
    {
        MainWindow main;
        QVERIFY(main.window() != nullptr);
        TickerController* ticker = tickerOf(main);
        QVERIFY(ticker != nullptr);
        QQuickItem* dialog = openDialog(main);
        QVERIFY(dialog != nullptr);
        QQuickItem* tryButton = inDialog(dialog, QStringLiteral("tickerTry"));
        QVERIFY(tryButton != nullptr);

        ticker->setText(QStringLiteral("Essai"));
        ticker->setSpeed(8);
        ticker->stop();
        QVERIFY(!ticker->running());
        clickItem(main, tryButton);
        QTRY_VERIFY(ticker->running());
        QQuickItem* text = inTicker(main, QStringLiteral("tickerText"));
        QVERIFY(text != nullptr);
        QCOMPARE(text->property("text").toString(), QStringLiteral("Essai"));
        QVERIFY(dialog->isVisible());

        const actcraft::features::TickerSettings defaults;
        TickerController fresh;
        QCOMPARE(fresh.text(), defaults.text);
        QCOMPARE(fresh.speed(), defaults.speed);
        QVERIFY(!QSettings().contains(QStringLiteral("tools/ticker/message")));
    }

    // Fichiers de messages depuis la boîte : « Ouvrir » demande une URL au choix injecté (aucune vraie boîte
    // de fichiers), lit le fichier et montre ses réglages dans la boîte sans les garder ; un fichier trop
    // gros est refusé avec la raison affichée et ne change rien ; « Enregistrer » écrit un fichier relisible.
    // Les fichiers sont écrits dans un dossier temporaire du test.
    void messageFileLoadedFromTheDialog()
    {
        QTemporaryDir files;
        QVERIFY(files.isValid());
        actcraft::features::TickerSettings sent;
        sent.text = QStringLiteral("Chargé du fichier");
        sent.position = actcraft::features::TickerSettings::Position::Bottom;
        sent.loop = true;
        sent.speed = 8;
        sent.shadowOffset = 4;
        const QString path = files.filePath(QStringLiteral("message.json"));
        writeFile(path, actcraft::features::tickerToJson(sent));

        MainWindow main;
        QVERIFY(main.window() != nullptr);
        TickerController* ticker = tickerOf(main);
        QVERIFY(ticker != nullptr);
        QUrl chosen = QUrl::fromLocalFile(path);
        int asked = 0;
        bool askedToSave = true;
        ticker->setFileChooser([&](bool save) {
            ++asked;
            askedToSave = save;
            return chosen;
        });
        QSignalSpy nativeDialog(ticker, &TickerController::fileDialogRequested);
        QVERIFY(nativeDialog.isValid());
        QQuickItem* dialog = openDialog(main);
        QVERIFY(dialog != nullptr);
        QQuickItem* open = inDialog(dialog, QStringLiteral("tickerOpenFile"));
        QQuickItem* save = inDialog(dialog, QStringLiteral("tickerSaveFile"));
        QQuickItem* field = inDialog(dialog, QStringLiteral("tickerTextField"));
        QQuickItem* error = inDialog(dialog, QStringLiteral("tickerFileError"));
        QVERIFY(open != nullptr && save != nullptr && field != nullptr && error != nullptr);

        clickItem(main, open);
        QTRY_COMPARE(ticker->text(), sent.text);
        QCOMPARE(asked, 1);
        QVERIFY(!askedToSave);
        QCOMPARE(ticker->settings(), sent);
        QTRY_COMPARE(field->property("text").toString(), sent.text);
        QVERIFY(!error->isVisible());
        const actcraft::features::TickerSettings defaults;
        TickerController fresh;
        QCOMPARE(fresh.text(), defaults.text); // chargé, pas gardé comme défaut

        // Trop gros : refusé, réglages inchangés, raison affichée.
        const QString big = files.filePath(QStringLiteral("gros.json"));
        writeFile(big, QByteArray(70 * 1024, ' '));
        chosen = QUrl::fromLocalFile(big);
        clickItem(main, open);
        QTRY_VERIFY(error->isVisible());
        QVERIFY(!error->property("text").toString().isEmpty());
        QCOMPARE(ticker->settings(), sent);
        QCOMPARE(asked, 2);

        // Enregistrer : le fichier écrit se relit et redonne les réglages courants.
        const QString out = files.filePath(QStringLiteral("sortie.json"));
        chosen = QUrl::fromLocalFile(out);
        ticker->setText(QStringLiteral("Écrit"));
        clickItem(main, save);
        QTRY_VERIFY(!error->isVisible());
        QCOMPARE(asked, 3);
        QVERIFY(askedToSave);
        QFile written(out);
        QVERIFY(written.open(QIODevice::ReadOnly));
        const auto back = actcraft::features::tickerFromJson(written.readAll());
        QVERIFY(back.has_value());
        QCOMPARE(*back, ticker->settings());
        QCOMPARE(back->text, QStringLiteral("Écrit"));
        QCOMPARE(nativeDialog.count(), 0); // jamais de vraie boîte de fichiers
    }

    void tickerDialogBlocksTheStylus_data() { presentationharness::addPointerRows(); }

    // La boîte d'édition est une couverture de la page : rien n'est versé dessous, quel que soit le
    // périphérique ; hors de la boîte, le même trait est versé. Souris : le clic droit sur la boîte n'ouvre
    // pas le menu de la page, hors de la boîte il l'ouvre.
    void tickerDialogBlocksTheStylus()
    {
        QFETCH(Pointer, pointer);
        MainWindow main;
        QVERIFY(main.window() != nullptr);
        QSignalSpy menu(main.pageView(), &actcraft::ui::PageView::contextMenuRequested);
        QVERIFY(menu.isValid());
        const int before = main.pageObjectCount();
        QQuickItem* dialog = openDialog(main);
        QVERIFY(dialog != nullptr);
        const QRectF zone = sceneRect(dialog);

        // Marge gauche de la boîte, hors de tout contrôle : seule la couverture retient le trait.
        const QPointF on = dialog->mapToScene(QPointF(3, 150));
        const QPointF onTo = dialog->mapToScene(QPointF(3, 260));
        QVERIFY(zone.contains(on) && zone.contains(onTo));
        strokeWith(main, pointer, on, onTo);
        QTest::qWait(150);
        QCOMPARE(main.pageObjectCount(), before);

        // Hors de la boîte (et sous la bande du message, en haut) : le trait est versé.
        QPointF from;
        QPointF to;
        QVERIFY(onPage(main, 300, 400, &from) && onPage(main, 700, 400, &to));
        QVERIFY(!zone.contains(from) && !zone.contains(to));
        strokeWith(main, pointer, from, to);
        QTRY_COMPARE_WITH_TIMEOUT(main.pageObjectCount(), before + 1, 5000);
        QCOMPARE(menu.count(), 0);

        if (pointer != Pointer::Mouse) {
            return;
        }
        QTest::mouseClick(main.window(), Qt::RightButton, Qt::NoModifier, on.toPoint());
        QTest::qWait(150);
        QCOMPARE(menu.count(), 0);
        QTest::mouseClick(main.window(), Qt::RightButton, Qt::NoModifier, from.toPoint());
        QTRY_COMPARE(menu.count(), 1);
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
