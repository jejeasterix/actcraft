#include "dicecontroller.h"
#include "support/lot7registry.h"
#include "support/presentationharness.h"

#include <actcraft/core/model.h>
#include <actcraft/features/dice.h>
#include <actcraft/ui/documentcontroller.h>

#include <QSettings>
#include <QSignalSpy>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QtTest>

#include <memory>
#include <variant>

namespace {

using actcraft::ui::DiceController;
using actcraft::ui::DocumentController;
using presentationharness::MainWindow;
using presentationharness::Pointer;
namespace core = actcraft::core;

DocumentController* controllerOf(const MainWindow& main)
{
    return main.window() ? main.window()->findChild<DocumentController*>(QStringLiteral("documentController"))
                         : nullptr;
}

DiceController* diceOf(const MainWindow& main)
{
    return main.window() ? main.window()->findChild<DiceController*>(QStringLiteral("dice")) : nullptr;
}

const core::Page& shown(const DocumentController& controller)
{
    return controller.document().page(static_cast<std::size_t>(controller.currentIndex()));
}

QPointF centreOf(const QQuickItem* item)
{
    return item->mapToScene(QPointF(item->width() / 2, item->height() / 2));
}

void clickOn(const MainWindow& main, const QString& name)
{
    // Cherché dans le panneau des dés : d'autres panneaux flottants du lot portent les mêmes noms
    // (« floatingPanelClose »), et main.item rend le premier de l'arbre, parfois celui d'un panneau fermé.
    QQuickItem* item = presentationharness::findItem(main.item(QStringLiteral("diceRoot")), name);
    QVERIFY2(item != nullptr, qPrintable(name));
    QTest::mouseClick(main.window(), Qt::LeftButton, Qt::NoModifier, centreOf(item).toPoint());
    QCoreApplication::processEvents();
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

// Ouvre le panneau par sa commande et le pose, sa barre de titre centrée sur le point de page (400, 300).
QQuickItem* openDice(const MainWindow& main)
{
    if (!main.run(QStringLiteral("tool.dice"))) {
        return nullptr;
    }
    QQuickItem* panel = main.item(QStringLiteral("diceRoot"));
    QQuickItem* layer = main.item(QStringLiteral("presentationLayer"));
    if (!panel || !layer || !panel->isVisible()) {
        return nullptr;
    }
    panel->setPosition(layer->mapFromScene(main.pageView()->pageToScene(QPointF(400, 300)) -
                                           QPointF(panel->width() / 2, 22)));
    return panel;
}

// Tout le fichier dans cet espace de noms anonyme (exécutable unique : aucun nom global hors du registre).
class TstUiDice : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase()
    {
        QStandardPaths::setTestModeEnabled(true);
        QCoreApplication::setOrganizationName(QStringLiteral("ActCraftTest"));
        QCoreApplication::setApplicationName(QStringLiteral("tst_ui_dice"));
    }

    // Réglages neufs pour chaque fonction : jamais le vrai registre, jamais ceux d'une fonction précédente.
    void init()
    {
        m_settings = std::make_unique<QTemporaryDir>();
        QVERIFY(m_settings->isValid());
        QSettings::setDefaultFormat(QSettings::IniFormat);
        QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, m_settings->path());
    }

    // E5 : réglages vides, un dé et sept secondes.
    void oneDieTheFirstTime()
    {
        DiceController dice;
        QCOMPARE(dice.count(), 1);
        QCOMPARE(dice.speed(), 7);
        dice.roll();
        dice.skip();
        QCOMPARE(dice.faces().size(), 1);
    }

    // Nombre et vitesse réglés sont retrouvés par un contrôleur recréé, bornes comprises.
    void diceCountAndSpeedRemembered()
    {
        {
            DiceController dice;
            dice.setCount(4);
            dice.setSpeed(9);
        }
        DiceController again;
        QCOMPARE(again.count(), 4);
        QCOMPARE(again.speed(), 9);
        again.setCount(9);
        again.setSpeed(20);
        QCOMPARE(again.count(), 5);
        QCOMPARE(again.speed(), 11);
        DiceController last;
        QCOMPARE(last.count(), 5);
        QCOMPARE(last.speed(), 11);
    }

    // Graine fixée, lancer, « Écrire sur la page » : un texte de la couche intermédiaire porte le total, en
    // une étape d'annulation.
    void diceTotalWrittenOnThePage()
    {
        MainWindow main;
        DocumentController* controller = controllerOf(main);
        QVERIFY(controller != nullptr);
        QVERIFY(openDice(main) != nullptr);
        DiceController* dice = diceOf(main);
        QVERIFY(dice != nullptr);

        dice->setSeed(42);
        dice->setCount(3);
        actcraft::features::Dice reference(42);
        reference.setCount(3);
        reference.roll();
        QVERIFY(!dice->canWrite());
        dice->roll();
        QVERIFY(dice->rolling());
        QVERIFY(!dice->canWrite());
        QCOMPARE(dice->faces().size(), 3);
        dice->skip();
        QVERIFY(!dice->rolling());
        QCOMPARE(dice->total(), reference.total());
        QVERIFY(dice->canWrite());

        const std::size_t before = shown(*controller).objects.size();
        clickOn(main, QStringLiteral("diceWriteButton"));
        QCOMPARE(shown(*controller).objects.size(), before + 1);
        const core::PageObject& written = shown(*controller).objects.back();
        const auto* text = std::get_if<core::Text>(&core::body(written));
        QVERIFY(text != nullptr);
        QVERIFY(written.common.layer == core::Layer::Middle);
        QVERIFY2(QString::fromStdString(text->html).contains(QStringLiteral(">%1<").arg(reference.total())),
                 text->html.c_str());
        controller->undo();
        QCOMPARE(shown(*controller).objects.size(), before);
        QVERIFY(!controller->canUndo());
    }

    // La croix ferme le panneau et arrête le lancer ; fermé, il ne couvre plus la page ; la commande le
    // rouvre.
    void dicePanelClosesByItsCross()
    {
        MainWindow main;
        QVERIFY(main.window() != nullptr);
        QQuickItem* panel = openDice(main);
        QVERIFY(panel != nullptr);
        DiceController* dice = diceOf(main);
        QVERIFY(dice != nullptr);
        const QPointF body = panel->mapToScene(QPointF(30, 70));
        const int before = main.pageObjectCount();

        dice->roll();
        QVERIFY(dice->rolling());
        clickOn(main, QStringLiteral("floatingPanelClose"));
        QVERIFY(!panel->isVisible());
        QVERIFY(!dice->rolling());

        presentationharness::strokeAt(main.window(), Pointer::Mouse, body, body + QPointF(120, 30));
        QTRY_COMPARE(main.pageObjectCount(), before + 1);

        QVERIFY(main.run(QStringLiteral("tool.dice")));
        QVERIFY(panel->isVisible());
    }

    void diceBlocksTheStylus_data() { presentationharness::addPointerRows(); }

    // Panneau ouvert, il couvre la page quel que soit le périphérique : « Lancer » lance sans tracer, la
    // barre déplace sans tracer, le corps retient le trait ; hors du panneau, un trait est versé. À la
    // souris, le clic droit sur le panneau n'ouvre pas le menu de la page, hors du panneau il l'ouvre.
    void diceBlocksTheStylus()
    {
        QFETCH(Pointer, pointer);
        MainWindow main;
        QVERIFY(main.window() != nullptr);
        QQuickItem* panel = openDice(main);
        QVERIFY(panel != nullptr);
        DiceController* dice = diceOf(main);
        QVERIFY(dice != nullptr);
        QQuickItem* roll = main.item(QStringLiteral("diceRollButton"));
        QQuickItem* total = main.item(QStringLiteral("diceTotal"));
        QVERIFY(roll != nullptr && total != nullptr);
        const int before = main.pageObjectCount();

        // « Lancer » : le lancer démarre, aucun trait.
        const QPointF button = centreOf(roll);
        if (pointer == Pointer::Stylus) {
            QVERIFY2(stylusTap(main.window(), button), "tablette non acceptée : rejouée en souris par Qt");
        } else {
            presentationharness::strokeAt(main.window(), pointer, button - QPointF(6, 0),
                                          button + QPointF(6, 0), 3);
        }
        QTRY_VERIFY(dice->rolling());
        dice->skip();
        QTest::qWait(50);
        QCOMPARE(main.pageObjectCount(), before);

        // Barre de titre : le panneau se déplace, aucun trait.
        const QPointF start = panel->position();
        const QPointF bar = panel->mapToScene(QPointF(100, 22));
        presentationharness::strokeAt(main.window(), pointer, bar, bar + QPointF(60, 40));
        QTest::qWait(50);
        QCOMPARE(main.pageObjectCount(), before);
        QVERIFY2(panel->position() != start, "panneau déplacé par sa barre");

        // Corps, sans gestionnaire : seule la couverture retient le trait.
        const QPointF body = centreOf(total) - QPointF(50, 0);
        presentationharness::strokeAt(main.window(), pointer, body, body + QPointF(100, 0));
        QTest::qWait(50);
        QCOMPARE(main.pageObjectCount(), before);

        if (pointer == Pointer::Mouse) {
            // L'appui gauche de la souris sur un élément sans gestionnaire ne prouve rien seul : le clic
            // droit, que la vue teste par sa couverture (menu de la page), discrimine.
            QSignalSpy menu(main.pageView(), &actcraft::ui::PageView::contextMenuRequested);
            QVERIFY(menu.isValid());
            QTest::mouseClick(main.window(), Qt::RightButton, Qt::NoModifier, centreOf(total).toPoint());
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
            QTest::mouseClick(main.window(), Qt::LeftButton, Qt::NoModifier, centreOf(total).toPoint());
            QTRY_VERIFY(!pageMenu->property("visible").toBool());
        }

        // Hors du panneau : un trait est versé.
        presentationharness::strokeAt(main.window(), pointer, main.pageView()->pageToScene(QPointF(40, 700)),
                                      main.pageView()->pageToScene(QPointF(140, 700)));
        QTRY_COMPARE(main.pageObjectCount(), before + 1);
    }

private:
    std::unique_ptr<QTemporaryDir> m_settings;
};

} // namespace

ACTCRAFT_LOT7_TEST("dice", TstUiDice)
#include "tst_ui_dice.moc"
