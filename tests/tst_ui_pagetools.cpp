#include "curtaincontroller.h"
#include "presentationdocument.h"
#include "spotlightcontroller.h"
#include "support/lot7registry.h"
#include "support/presentationharness.h"

#include <actcraft/core/document.h>
#include <actcraft/core/model.h>
#include <actcraft/ui/documentcontroller.h>

#include <QtTest>

#include <utility>
#include <vector>

namespace {

// Tout le fichier dans cet espace de noms anonyme (exécutable unique : aucun nom global hors du registre).
namespace core = actcraft::core;
using actcraft::ui::CurtainController;
using actcraft::ui::DocumentController;
using actcraft::ui::DocumentInsertions;
using actcraft::ui::SpotlightController;
using presentationharness::MainWindow;
using presentationharness::Pointer;

DocumentController* controllerOf(const MainWindow& main)
{
    return main.window() ? main.window()->findChild<DocumentController*>(QStringLiteral("documentController"))
                         : nullptr;
}

CurtainController* curtainOf(const MainWindow& main)
{
    return main.window() ? main.window()->findChild<CurtainController*>(QStringLiteral("curtain")) : nullptr;
}

SpotlightController* spotlightOf(const MainWindow& main)
{
    return main.window() ? main.window()->findChild<SpotlightController*>(QStringLiteral("spotlight"))
                         : nullptr;
}

QPointF inWindow(const MainWindow& main, QPointF pagePoint)
{
    return main.pageView()->pageToScene(pagePoint);
}

bool closeTo(double value, double expected)
{
    return qAbs(value - expected) <= 0.5;
}

// Bord du store saisi en `from` et tiré jusqu'à `to`, en coordonnées de page.
bool drag(CurtainController& curtain, QPointF from, QPointF to)
{
    const bool grabbed = curtain.press(from);
    curtain.move(to);
    curtain.release();
    return grabbed;
}

class TstUiPageTools : public QObject
{
    Q_OBJECT

private slots:
    // Les réglages écrits par les commandes page.* s'appliquent au changement de page, pas avant ; « comme
    // avant » garde l'état ; une page « spot » ouvre son spot et ferme le rideau ; ouvrir le spot ferme le
    // rideau.
    void pageChangeAppliesPageTools()
    {
        MainWindow main;
        QVERIFY(main.window() != nullptr);
        DocumentController* controller = controllerOf(main);
        CurtainController* curtain = curtainOf(main);
        SpotlightController* spotlight = spotlightOf(main);
        QVERIFY(controller != nullptr);
        QVERIFY(curtain != nullptr);
        QVERIFY(spotlight != nullptr);

        controller->nextPage(); // page 2 ajoutée et affichée
        QTRY_COMPARE(controller->currentIndex(), 1);
        QVERIFY(main.run(QStringLiteral("page.tools-mode"), QStringLiteral("curtain")));
        QVERIFY(main.run(QStringLiteral("page.curtain-mode"), QStringLiteral("left")));
        QVERIFY(main.run(QStringLiteral("page.curtain-distance"), 200));
        const core::PagePresentation& second = controller->document().page(1).presentation;
        QVERIFY(second.tools == core::PageToolsMode::Curtain);
        QVERIFY(second.curtain == core::CurtainMode::Left);
        QCOMPARE(second.curtainDistance, 200.0);
        QVERIFY(!curtain->active()); // écrit, pas encore appliqué

        controller->pageBack(); // page 1 « comme avant » : rien ne s'ouvre
        QTRY_COMPARE(controller->currentIndex(), 0);
        QVERIFY(!curtain->active());
        controller->nextPage();
        QTRY_COMPARE(controller->currentIndex(), 1);
        QVERIFY(curtain->active());
        QVERIFY2(closeTo(curtain->covered().x(), 200), qPrintable(QString::number(curtain->covered().x())));
        controller->pageBack(); // « comme avant » : le rideau de la page quittée continue
        QTRY_COMPARE(controller->currentIndex(), 0);
        QVERIFY(curtain->active());
        QVERIFY(closeTo(curtain->covered().x(), 200));

        // Page 1 en mode spot carré : appliqué en y revenant.
        QVERIFY(main.run(QStringLiteral("page.tools-mode"), QStringLiteral("spotlight")));
        QVERIFY(main.run(QStringLiteral("page.spotlight-mode"), QStringLiteral("square")));
        QVERIFY(curtain->active());
        controller->nextPage();
        QTRY_COMPARE(controller->currentIndex(), 1);
        QCOMPARE(spotlight->shape(), 0);
        controller->pageBack();
        QTRY_COMPARE(controller->currentIndex(), 0);
        QCOMPARE(spotlight->shape(), static_cast<int>(core::SpotlightShape::Square));
        QVERIFY(!curtain->active());

        // Page 2 : rideau ouvert, spot fermé ; ouvrir le spot ferme le rideau.
        controller->nextPage();
        QTRY_COMPARE(controller->currentIndex(), 1);
        QVERIFY(curtain->active());
        QCOMPARE(spotlight->shape(), 0);
        QVERIFY(main.run(QStringLiteral("present.spotlight-circle")));
        QCOMPARE(spotlight->shape(), static_cast<int>(core::SpotlightShape::Circle));
        QVERIFY(!curtain->active());
    }

    // Document remplacé (ouverture) dont la première page est en mode rideau : le rideau s'ouvre sans
    // changement de page affichée (documentReplaced).
    void openedDocumentAppliesPageTools()
    {
        MainWindow main;
        QVERIFY(main.window() != nullptr);
        DocumentController* controller = controllerOf(main);
        CurtainController* curtain = curtainOf(main);
        QVERIFY(controller != nullptr);
        QVERIFY(curtain != nullptr);
        QCOMPARE(controller->currentIndex(), 0);
        QVERIFY(!curtain->active());

        std::vector<core::Page> pages(2);
        pages[0].presentation.tools = core::PageToolsMode::Curtain;
        pages[0].presentation.curtain = core::CurtainMode::Top;
        pages[0].presentation.curtainDistance = 100;
        controller->replaceDocument(core::Document(std::move(pages)), QUrl(),
                                    QStringLiteral("Outils de page"));
        QCOMPARE(controller->currentIndex(), 0);
        QTRY_VERIFY(curtain->active());
        QVERIFY2(closeTo(curtain->covered().y(), 100), qPrintable(QString::number(curtain->covered().y())));
    }

    // La page porte un rideau mémorisé (haut, 120) ; le rideau est tiré à 300 sans le mémoriser ; un trait au
    // stylet hors du rideau ne le remet pas à 120 (les outils ne se relisent pas sur documentChanged).
    void strokeKeepsTheLiveCurtain()
    {
        MainWindow main;
        QVERIFY(main.window() != nullptr);
        DocumentController* controller = controllerOf(main);
        CurtainController* curtain = curtainOf(main);
        QVERIFY(controller != nullptr);
        QVERIFY(curtain != nullptr);

        controller->nextPage();
        QTRY_COMPARE(controller->currentIndex(), 1);
        core::PagePresentation value;
        value.tools = core::PageToolsMode::Curtain;
        value.curtain = core::CurtainMode::Top;
        value.curtainDistance = 120;
        DocumentInsertions(*controller).setPagePresentation(value);
        controller->pageBack();
        QTRY_COMPARE(controller->currentIndex(), 0);
        controller->nextPage();
        QTRY_COMPARE(controller->currentIndex(), 1);
        QVERIFY(curtain->active());
        QVERIFY2(closeTo(curtain->covered().y(), 120), qPrintable(QString::number(curtain->covered().y())));

        QVERIFY(drag(*curtain, QPointF(512, 122), QPointF(512, 300)));
        QVERIFY(closeTo(curtain->covered().y(), 300));

        const int before = main.pageObjectCount();
        presentationharness::strokeAt(main.window(), Pointer::Stylus, inWindow(main, QPointF(300, 100)),
                                      inWindow(main, QPointF(700, 100)));
        QTRY_COMPARE_WITH_TIMEOUT(main.pageObjectCount(), before + 1, 5000);
        QTest::qWait(50);
        QVERIFY(curtain->active());
        QVERIFY2(closeTo(curtain->covered().y(), 300), qPrintable(QString::number(curtain->covered().y())));
    }

    // « Mémoriser pour cette page » : le rideau tiré de 120 depuis le haut devient le réglage de la page
    // (outil rideau, haut, 120) en une étape ; une annulation rend les défauts.
    void rememberCurtainIsUndoneAtOnce()
    {
        MainWindow main;
        QVERIFY(main.window() != nullptr);
        DocumentController* controller = controllerOf(main);
        CurtainController* curtain = curtainOf(main);
        QQuickItem* root = main.item(QStringLiteral("curtainRoot"));
        QVERIFY(controller != nullptr);
        QVERIFY(curtain != nullptr);
        QVERIFY(root != nullptr);
        QVERIFY(!controller->canUndo());

        QVERIFY(main.run(QStringLiteral("present.curtain")));
        QVERIFY(curtain->active());
        QVERIFY(drag(*curtain, QPointF(512, 2), QPointF(512, 120)));
        QVERIFY(closeTo(curtain->covered().y(), 120));

        QVariant done;
        QVERIFY(QMetaObject::invokeMethod(root, "rememberForPage", Q_RETURN_ARG(QVariant, done)));
        QVERIFY(done.toBool());
        const std::size_t index = static_cast<std::size_t>(controller->currentIndex());
        const core::PagePresentation& remembered = controller->document().page(index).presentation;
        QVERIFY(remembered.tools == core::PageToolsMode::Curtain);
        QVERIFY(remembered.curtain == core::CurtainMode::Top);
        QVERIFY2(qAbs(remembered.curtainDistance - 120) <= 1e-6,
                 qPrintable(QString::number(remembered.curtainDistance)));
        QVERIFY(curtain->active()); // le rideau vivant n'a pas bougé
        QVERIFY(closeTo(curtain->covered().y(), 120));

        controller->undo();
        QVERIFY(core::isDefault(controller->document().page(index).presentation));
        QVERIFY(!controller->canUndo());
    }
};

} // namespace

ACTCRAFT_LOT7_TEST("pagetools", TstUiPageTools)
#include "tst_ui_pagetools.moc"
