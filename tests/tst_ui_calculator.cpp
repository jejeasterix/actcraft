#include "calculatorcontroller.h"
#include "support/lot7registry.h"
#include "support/presentationharness.h"

#include <actcraft/core/model.h>
#include <actcraft/ui/documentcontroller.h>

#include <QSettings>
#include <QSignalSpy>
#include <QStandardPaths>
#include <QStringList>
#include <QTemporaryDir>
#include <QtTest>

#include <memory>
#include <variant>

namespace {

using actcraft::ui::CalculatorController;
using actcraft::ui::DocumentController;
using presentationharness::MainWindow;
using presentationharness::Pointer;
namespace core = actcraft::core;

DocumentController* controllerOf(const MainWindow& main)
{
    return main.window() ? main.window()->findChild<DocumentController*>(QStringLiteral("documentController"))
                         : nullptr;
}

CalculatorController* calculatorOf(const MainWindow& main)
{
    return main.window() ? main.window()->findChild<CalculatorController*>(QStringLiteral("calculator"))
                         : nullptr;
}

const core::Page& shown(const DocumentController& controller)
{
    return controller.document().page(static_cast<std::size_t>(controller.currentIndex()));
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

// Ouvre le panneau par sa commande et le pose, sa barre de titre centrée sur le point de page (400, 300).
QQuickItem* openCalculator(const MainWindow& main)
{
    if (!main.run(QStringLiteral("tool.calculator"))) {
        return nullptr;
    }
    QQuickItem* panel = main.item(QStringLiteral("calculatorRoot"));
    QQuickItem* layer = main.item(QStringLiteral("presentationLayer"));
    if (!panel || !layer || !panel->isVisible()) {
        return nullptr;
    }
    panel->setPosition(layer->mapFromScene(main.pageView()->pageToScene(QPointF(400, 300)) -
                                           QPointF(panel->width() / 2, 22)));
    return panel;
}

// Une suite de touches par le clic de souris sur les boutons du panneau. Faux si une touche manque.
bool clickKeys(const MainWindow& main, const QStringList& names)
{
    for (const QString& name : names) {
        QQuickItem* item = main.item(QStringLiteral("calcKey") + name);
        if (!item) {
            return false;
        }
        QTest::mouseClick(main.window(), Qt::LeftButton, Qt::NoModifier, centreOf(item).toPoint());
        QCoreApplication::processEvents();
    }
    return true;
}

QString textOf(const MainWindow& main, const QString& objectName)
{
    QQuickItem* item = main.item(objectName);
    return item ? item->property("text").toString() : QStringLiteral("<absent>");
}

// Tout le fichier dans cet espace de noms anonyme (exécutable unique : aucun nom global hors du registre).
class TstUiCalculator : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase()
    {
        QStandardPaths::setTestModeEnabled(true);
        QCoreApplication::setOrganizationName(QStringLiteral("ActCraftTest"));
        QCoreApplication::setApplicationName(QStringLiteral("tst_ui_calculator"));
    }

    // Réglages neufs pour chaque fonction : jamais le vrai registre, jamais ceux d'une fonction précédente.
    void init()
    {
        m_settings = std::make_unique<QTemporaryDir>();
        QVERIFY(m_settings->isValid());
        QSettings::setDefaultFormat(QSettings::IniFormat);
        QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, m_settings->path());
    }

    // Réglage « calculatrice » : ActCraft par défaut ; réglage et programme retrouvés par un contrôleur
    // recréé.
    void calculatorSettingRemembered()
    {
        {
            CalculatorController calc;
            QCOMPARE(calc.mode(), 0);
            QVERIFY(calc.programPath().isEmpty());
            calc.setMode(2);
            calc.setProgramPath(QStringLiteral("C:/Outils/calc.exe"));
        }
        CalculatorController again;
        QCOMPARE(again.mode(), 2);
        QCOMPARE(again.programPath(), QStringLiteral("C:/Outils/calc.exe"));
        again.setMode(7);
        QCOMPARE(again.mode(), 2); // borné
    }

    // Sous Windows, « calculatrice du système » lance calc.exe et « programme choisi » lance le programme,
    // par le lanceur injecté (jamais un vrai processus) ; le panneau ne se montre pas. Ailleurs, ces deux
    // réglages retombent sur la calculatrice d'ActCraft. Réglage « ActCraft » : aucun lancement.
    void systemCalculatorLaunchedOnWindows()
    {
        QStringList launched;
        const auto fake = [&launched](const QString& program) {
            launched.append(program);
            return true;
        };
        CalculatorController calc;
        calc.setLauncher(fake);

        QVERIFY(calc.open());
        QVERIFY(launched.isEmpty());

        calc.setMode(1);
#ifdef Q_OS_WIN
        QVERIFY(CalculatorController::systemModesAvailable());
        QVERIFY(!calc.open());
        QCOMPARE(launched, QStringList{QStringLiteral("calc.exe")});

        launched.clear();
        calc.setMode(2);
        QVERIFY(calc.open()); // programme choisi, mais aucun chemin : le panneau d'ActCraft
        QVERIFY(launched.isEmpty());
        calc.setProgramPath(QStringLiteral("C:/Outils/calc.exe"));
        QVERIFY(!calc.open());
        QCOMPARE(launched, QStringList{QStringLiteral("C:/Outils/calc.exe")});

        // Lanceur qui échoue : le panneau d'ActCraft se montre quand même.
        calc.setLauncher([](const QString&) { return false; });
        QVERIFY(calc.open());

        // Par la commande : le programme part, le panneau reste fermé.
        MainWindow main;
        CalculatorController* controller = calculatorOf(main);
        QVERIFY(controller != nullptr);
        launched.clear();
        controller->setLauncher(fake);
        controller->setMode(1);
        QVERIFY(main.run(QStringLiteral("tool.calculator")));
        QCOMPARE(launched, QStringList{QStringLiteral("calc.exe")});
        QQuickItem* panel = main.item(QStringLiteral("calculatorRoot"));
        QVERIFY(panel != nullptr);
        QVERIFY(!panel->isVisible());
        controller->setMode(0);
        QVERIFY(main.run(QStringLiteral("tool.calculator")));
        QVERIFY(panel->isVisible());
        QCOMPARE(launched.size(), 1);
#else
        QVERIFY(!CalculatorController::systemModesAvailable());
        QVERIFY(calc.open());
        QVERIFY(launched.isEmpty());
#endif
    }

    // Saisir 12 + 3 : ligne haute « 12 + 3 », ligne basse « 3 » ; après « = », la ligne basse vaut « 15 ».
    void panelShowsTheCalculationAboveTheResult()
    {
        MainWindow main;
        QVERIFY(openCalculator(main) != nullptr);
        QCOMPARE(textOf(main, QStringLiteral("calculatorDisplay")), QStringLiteral("0"));

        QVERIFY(clickKeys(main, {QStringLiteral("1"), QStringLiteral("2"), QStringLiteral("Plus")}));
        QCOMPARE(textOf(main, QStringLiteral("calculatorExpression")), QStringLiteral("12 +"));
        QCOMPARE(textOf(main, QStringLiteral("calculatorDisplay")), QStringLiteral("12"));
        QVERIFY(clickKeys(main, {QStringLiteral("3")}));
        QCOMPARE(textOf(main, QStringLiteral("calculatorExpression")), QStringLiteral("12 + 3"));
        QCOMPARE(textOf(main, QStringLiteral("calculatorDisplay")), QStringLiteral("3"));
        QVERIFY(clickKeys(main, {QStringLiteral("Equals")}));
        QCOMPARE(textOf(main, QStringLiteral("calculatorExpression")), QStringLiteral("12 + 3 ="));
        QCOMPARE(textOf(main, QStringLiteral("calculatorDisplay")), QStringLiteral("15"));
    }

    // « CE » n'efface que la dernière saisie ; « C » efface tout le calcul.
    void clearEntryAndClearAllAreTwoKeys()
    {
        MainWindow main;
        QVERIFY(openCalculator(main) != nullptr);

        QVERIFY(clickKeys(main, {QStringLiteral("1"), QStringLiteral("2"), QStringLiteral("Plus"),
                                 QStringLiteral("3"), QStringLiteral("CE")}));
        QCOMPARE(textOf(main, QStringLiteral("calculatorDisplay")), QStringLiteral("0"));
        QCOMPARE(textOf(main, QStringLiteral("calculatorExpression")), QStringLiteral("12 +"));
        QVERIFY(clickKeys(main, {QStringLiteral("4"), QStringLiteral("Equals")}));
        QCOMPARE(textOf(main, QStringLiteral("calculatorDisplay")), QStringLiteral("16"));

        QVERIFY(clickKeys(main, {QStringLiteral("C")}));
        QCOMPARE(textOf(main, QStringLiteral("calculatorDisplay")), QStringLiteral("0"));
        QCOMPARE(textOf(main, QStringLiteral("calculatorExpression")), QString());
    }

    // Le clavier physique tape aussi, panneau ouvert.
    void keyboardTypesToo()
    {
        MainWindow main;
        QVERIFY(openCalculator(main) != nullptr);
        for (const Qt::Key key : {Qt::Key_2, Qt::Key_Plus, Qt::Key_3}) {
            QTest::keyClick(main.window(), key);
        }
        QTest::keyClick(main.window(), Qt::Key_Return);
        QCOMPARE(textOf(main, QStringLiteral("calculatorDisplay")), QStringLiteral("5"));
        QTest::keyClick(main.window(), Qt::Key_C);
        QCOMPARE(textOf(main, QStringLiteral("calculatorDisplay")), QStringLiteral("0"));
    }

    // « Poser sur la page » écrit le calcul (« 1,5 + 1 = 2,5 ») en un texte de la couche intermédiaire, en
    // une étape d'annulation.
    void calculationToPageIsUndoneAtOnce()
    {
        MainWindow main;
        DocumentController* controller = controllerOf(main);
        QVERIFY(controller != nullptr);
        QVERIFY(openCalculator(main) != nullptr);
        QVERIFY(clickKeys(main, {QStringLiteral("1"), QStringLiteral("Comma"), QStringLiteral("5"),
                                 QStringLiteral("Plus"), QStringLiteral("1"), QStringLiteral("Equals")}));
        QCOMPARE(textOf(main, QStringLiteral("calculatorDisplay")), QStringLiteral("2,5"));

        const std::size_t before = shown(*controller).objects.size();
        QQuickItem* place = main.item(QStringLiteral("calculatorPlaceButton"));
        QVERIFY(place != nullptr);
        QTest::mouseClick(main.window(), Qt::LeftButton, Qt::NoModifier, centreOf(place).toPoint());
        QCoreApplication::processEvents();
        QCOMPARE(shown(*controller).objects.size(), before + 1);
        const core::PageObject& written = shown(*controller).objects.back();
        const auto* text = std::get_if<core::Text>(&core::body(written));
        QVERIFY(text != nullptr);
        QVERIFY(written.common.layer == core::Layer::Middle);
        QVERIFY2(QString::fromStdString(text->html).contains(QStringLiteral(">1,5 + 1 = 2,5<")),
                 text->html.c_str());
        controller->undo();
        QCOMPARE(shown(*controller).objects.size(), before);
        QVERIFY(!controller->canUndo());
    }

    // Le résultat se glisse hors du panneau : lâché sur la page en (300, 200), le calcul s'y écrit en un
    // texte dont les bornes contiennent ce point, en une étape d'annulation ; lâché dans le panneau, rien ne
    // s'écrit.
    void calculationDraggedToThePageIsUndoneAtOnce()
    {
        MainWindow main;
        DocumentController* controller = controllerOf(main);
        QVERIFY(controller != nullptr);
        QVERIFY(openCalculator(main) != nullptr);
        QVERIFY(clickKeys(main, {QStringLiteral("1"), QStringLiteral("Plus"), QStringLiteral("1"),
                                 QStringLiteral("Equals")}));
        QQuickItem* display = main.item(QStringLiteral("calculatorDisplay"));
        QVERIFY(display != nullptr);
        QCOMPARE(textOf(main, QStringLiteral("calculatorDisplay")), QStringLiteral("2"));
        const std::size_t before = shown(*controller).objects.size();

        // Dans le panneau : rien (le lâcher écrit dans le document au relâché, lu ici sans attente).
        presentationharness::strokeAt(main.window(), Pointer::Mouse, centreOf(display) - QPointF(40, 0),
                                      centreOf(display) + QPointF(40, 0));
        QCOMPARE(shown(*controller).objects.size(), before);

        // Hors du panneau mais hors de la page (au-dessus de son bord haut) : rien non plus (revue c3a n° 2).
        presentationharness::strokeAt(main.window(), Pointer::Mouse, centreOf(display),
                                      main.pageView()->pageToScene(QPointF(300, -30)));
        QCOMPARE(shown(*controller).objects.size(), before);

        presentationharness::strokeAt(main.window(), Pointer::Mouse, centreOf(display),
                                      main.pageView()->pageToScene(QPointF(300, 200)));
        QTRY_COMPARE(shown(*controller).objects.size(), before + 1);
        const core::PageObject& written = shown(*controller).objects.back();
        const auto* text = std::get_if<core::Text>(&core::body(written));
        QVERIFY(text != nullptr);
        QVERIFY(written.common.layer == core::Layer::Middle);
        QVERIFY2(QString::fromStdString(text->html).contains(QStringLiteral(">1 + 1 = 2<")),
                 text->html.c_str());
        QVERIFY(text->bounds.contains(core::PointF{300, 200}));
        controller->undo();
        QCOMPARE(shown(*controller).objects.size(), before);
        QVERIFY(!controller->canUndo()); // une seule étape
    }

    // Sans document où écrire, la poignée du résultat est inactive (garde `enabled: canWrite`) : pas de
    // glisser, comme pour le bouton. Document rendu, elle redevient active. Mutant : garde retirée, poignée
    // toujours active.
    void resultDragIsDisabledWithoutDocument()
    {
        MainWindow main;
        CalculatorController* calc = calculatorOf(main);
        DocumentController* document = controllerOf(main);
        QVERIFY(calc != nullptr && document != nullptr);
        QVERIFY(openCalculator(main) != nullptr);
        QObject* drag =
            main.window()->contentItem()->findChild<QObject*>(QStringLiteral("calculatorResultDrag"));
        QVERIFY(drag != nullptr);
        QVERIFY(calc->canWrite());
        QCOMPARE(drag->property("enabled").toBool(), true);

        calc->setDocumentController(nullptr);
        QVERIFY(!calc->canWrite());
        QCOMPARE(drag->property("enabled").toBool(), false);

        calc->setDocumentController(document);
        QCOMPARE(drag->property("enabled").toBool(), true);
    }

    void calculatorBlocksTheStylus_data() { presentationharness::addPointerRows(); }

    // Panneau ouvert, il couvre la page quel que soit le périphérique : la touche « 7 » saisit sans tracer,
    // la barre déplace sans tracer, le corps retient le trait ; hors du panneau, un trait est versé. À la
    // souris, le clic droit sur le panneau n'ouvre pas le menu de la page, hors du panneau il l'ouvre.
    void calculatorBlocksTheStylus()
    {
        QFETCH(Pointer, pointer);
        MainWindow main;
        QVERIFY(main.window() != nullptr);
        QQuickItem* panel = openCalculator(main);
        QVERIFY(panel != nullptr);
        QQuickItem* seven = main.item(QStringLiteral("calcKey7"));
        QQuickItem* display = main.item(QStringLiteral("calculatorDisplay"));
        QVERIFY(seven != nullptr && display != nullptr);
        const int before = main.pageObjectCount();

        // Touche « 7 » : l'affichage passe à 7, aucun trait.
        const QPointF button = centreOf(seven);
        if (pointer == Pointer::Stylus) {
            QVERIFY2(stylusTap(main.window(), button), "tablette non acceptée : rejouée en souris par Qt");
        } else {
            presentationharness::strokeAt(main.window(), pointer, button - QPointF(6, 0),
                                          button + QPointF(6, 0), 3);
        }
        QTRY_COMPARE(textOf(main, QStringLiteral("calculatorDisplay")), QStringLiteral("7"));
        QVERIFY(main.countStaysAt(before));

        // Barre de titre : le panneau se déplace, aucun trait.
        const QPointF start = panel->position();
        const QPointF bar = panel->mapToScene(QPointF(100, 22));
        presentationharness::strokeAt(main.window(), pointer, bar, bar + QPointF(60, 40));
        QTRY_VERIFY2(panel->position() != start, "panneau déplacé par sa barre");
        QVERIFY(main.countStaysAt(before));

        // Corps, sans gestionnaire : seule la couverture retient le trait. Visé sur la ligne du calcul, sans
        // gestionnaire (le résultat porte le glisser du calcul, qui prendrait le trait à la souris et au
        // doigt).
        QQuickItem* expression = main.item(QStringLiteral("calculatorExpression"));
        QVERIFY(expression != nullptr);
        const QPointF body = centreOf(expression);
        presentationharness::strokeAt(main.window(), pointer, body - QPointF(50, 0), body + QPointF(50, 0));
        QVERIFY(main.countStaysAt(before));

        if (pointer == Pointer::Mouse) {
            // L'appui gauche de la souris sur un élément sans gestionnaire ne prouve rien seul : le clic
            // droit, que la vue teste par sa couverture (menu de la page), discrimine.
            QSignalSpy menu(main.pageView(), &actcraft::ui::PageView::contextMenuRequested);
            QVERIFY(menu.isValid());
            QTest::mouseClick(main.window(), Qt::RightButton, Qt::NoModifier, centreOf(display).toPoint());
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
            QTest::mouseClick(main.window(), Qt::LeftButton, Qt::NoModifier, centreOf(display).toPoint());
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

ACTCRAFT_LOT7_TEST("calculator", TstUiCalculator)
#include "tst_ui_calculator.moc"
