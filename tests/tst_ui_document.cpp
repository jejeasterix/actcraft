#include <actcraft/core/document.h>
#include <actcraft/core/generator.h>
#include <actcraft/render/fixedpageitem.h>
#include <actcraft/ui/documentcontroller.h>

#include <QQmlComponent>
#include <QQmlEngine>
#include <QSignalSpy>
#include <QStringList>
#include <QTest>
#include <QtQml/qqmlextensionplugin.h>

#include <cstdint>
#include <memory>
#include <variant>

Q_IMPORT_QML_PLUGIN(ActCraft_UiPlugin)

using namespace actcraft;

namespace {

core::Stroke redStroke()
{
    core::Stroke s;
    s.points = {core::StrokePoint{100, 100, 0.5f}, core::StrokePoint{300, 100, 0.5f}};
    s.baseWidth = 8;
    s.color = 0xFFE53935u;
    return s;
}

} // namespace

// Document ouvert et page affichée (squelette du lot 1, DA2).
class TstUiDocument : public QObject
{
    Q_OBJECT

private slots:
    void startsWithOneBlankPageShown()
    {
        render::FixedPageItem fixed;
        ui::DocumentController controller;
        controller.setFixedPage(&fixed);
        QCOMPARE(controller.pageCount(), 1);
        QCOMPARE(controller.currentIndex(), 0);
        QCOMPARE(controller.pageLabel(), QStringLiteral("1 / 1"));
        QCOMPARE(fixed.objectCount(), 0);
        QTRY_VERIFY(fixed.isReady());
    }

    void nextOnLastPageAddsABlankPage()
    {
        render::FixedPageItem fixed;
        ui::DocumentController controller;
        controller.setFixedPage(&fixed);
        QSignalSpy pages(&controller, &ui::DocumentController::pagesChanged);
        controller.nextPage();
        QCOMPARE(pages.count(), 1);
        QCOMPARE(controller.pageCount(), 2);
        QCOMPARE(controller.currentIndex(), 1);
        QCOMPARE(controller.pageLabel(), QStringLiteral("2 / 2"));
        controller.previousPage();
        QCOMPARE(controller.pageLabel(), QStringLiteral("1 / 2"));
        controller.nextPage(); // la page 2 existe : pas de troisième page
        QCOMPARE(controller.pageCount(), 2);
    }

    void previousOnFirstPageDoesNothing()
    {
        render::FixedPageItem fixed;
        ui::DocumentController controller;
        controller.setFixedPage(&fixed);
        QSignalSpy pages(&controller, &ui::DocumentController::pagesChanged);
        controller.previousPage();
        QCOMPARE(pages.count(), 0);
        QCOMPARE(controller.currentIndex(), 0);
    }

    void strokeSurvivesPageRoundTrip()
    {
        render::FixedPageItem fixed;
        ui::DocumentController controller;
        controller.setFixedPage(&fixed);
        fixed.appendStroke(redStroke()); // comme le stylo (FixedPageSink)
        controller.nextPage();
        QCOMPARE(fixed.objectCount(), 0);
        controller.previousPage();
        QCOMPARE(fixed.objectCount(), 1);
        const core::Document& document = controller.document();
        QCOMPARE(document.page(0).objects.size(), std::size_t{1});
        QVERIFY(std::get<core::Stroke>(document.page(0).objects[0]).color == 0xFFE53935u);
        QVERIFY(document.page(1).objects.empty());
    }

    void documentReadsTheShownPage()
    {
        render::FixedPageItem fixed;
        ui::DocumentController controller;
        controller.setFixedPage(&fixed);
        fixed.appendStroke(redStroke());
        QCOMPARE(controller.document().page(0).objects.size(), std::size_t{1});
    }

    // Une seule source de vérité : la page affichée est celle de l'élément, quel que soit l'écrivain. Un
    // contrôleur qui recopierait chaque trait dans le document (patron du miroir, branché sur strokeAppended
    // ou sur le puits du stylo) ne verrait pas une page posée sans passer par appendStroke — Main.qml appelle
    // encore loadGeneratedPage() — et perdrait ces 300 objets au premier aller-retour.
    void documentFollowsEveryWriterOfTheShownPage()
    {
        render::FixedPageItem fixed;
        ui::DocumentController controller;
        controller.setFixedPage(&fixed);
        fixed.loadGeneratedPage(); // autre écrivain que le stylo : aucun strokeAppended
        fixed.appendStroke(redStroke());
        const std::uint64_t shown = core::fingerprint(fixed.page());
        QCOMPARE(core::fingerprint(controller.document().page(0)), shown);
        controller.nextPage();
        controller.previousPage();
        QCOMPARE(fixed.objectCount(), 301);
        QCOMPARE(core::fingerprint(fixed.page()), shown);
    }

    // Le contrôleur pilote core::Document (nextPageIndex) : la page ajoutée prend la taille de la dernière,
    // c'est-à-dire de la page affichée telle que l'élément la tient, recopiée avant l'ajout.
    void newPageTakesTheShownPageSize()
    {
        render::FixedPageItem fixed;
        ui::DocumentController controller;
        controller.setFixedPage(&fixed);
        core::Page wide;
        wide.width = 1280;
        wide.height = 720;
        fixed.setPage(wide);
        controller.nextPage();
        const core::Document& document = controller.document();
        QCOMPARE(document.pageCount(), std::size_t{2});
        QCOMPARE(document.page(1).width, 1280.0);
        QCOMPARE(document.page(1).height, 720.0);
        QCOMPARE(fixed.page().width, 1280.0);
    }

    // pagesChanged : une fois par déplacement, jamais sans déplacement, et émis quand tout est en place (le
    // libellé et l'élément montrent déjà la nouvelle page, y compris celle qui vient d'être ajoutée).
    void pagesChangedOncePerMoveWithTheNewPageInPlace()
    {
        render::FixedPageItem fixed;
        ui::DocumentController controller;
        controller.setFixedPage(&fixed);
        fixed.appendStroke(redStroke());
        QStringList seen;
        connect(&controller, &ui::DocumentController::pagesChanged, this, [&] {
            seen << QStringLiteral("%1 : %2").arg(controller.pageLabel()).arg(fixed.objectCount());
        });
        controller.nextPage();     // ajoute la page 2 et l'affiche
        controller.previousPage(); // retour sur la page 1
        controller.nextPage();     // la page 2 existe déjà
        controller.previousPage();
        controller.previousPage(); // première page : pas de déplacement
        QCOMPARE(seen, (QStringList{QStringLiteral("2 / 2 : 0"), QStringLiteral("1 / 2 : 1"),
                                    QStringLiteral("2 / 2 : 0"), QStringLiteral("1 / 2 : 1")}));
    }

    void noPagesChangedWithoutAMove()
    {
        render::FixedPageItem fixed;
        ui::DocumentController controller;
        QSignalSpy pages(&controller, &ui::DocumentController::pagesChanged);
        QSignalSpy view(&controller, &ui::DocumentController::fixedPageChanged);
        controller.setFixedPage(&fixed);
        controller.setFixedPage(&fixed); // même élément : rien
        controller.loadGeneratedFirstPage();
        controller.document();
        QCOMPARE(view.count(), 1);
        QCOMPARE(pages.count(), 0);
        QCOMPARE(controller.pageLabel(), QStringLiteral("1 / 1"));
    }

    void generatedFirstPageForTheBench()
    {
        render::FixedPageItem fixed;
        ui::DocumentController controller;
        controller.setFixedPage(&fixed);
        controller.loadGeneratedFirstPage();
        QCOMPARE(fixed.objectCount(), 300);
        QCOMPARE(core::fingerprint(controller.document().page(0)),
                 core::fingerprint(core::generatePage(core::kDefaultSeed)));
        controller.nextPage();
        QCOMPARE(fixed.objectCount(), 0);
        QCOMPARE(controller.pageLabel(), QStringLiteral("2 / 2"));
    }

    void registeredInQml()
    {
        QQmlEngine engine;
        QQmlComponent component(&engine);
        component.setData("import ActCraft.Ui\nDocumentController {}\n", QUrl());
        std::unique_ptr<QObject> controller(component.create());
        QVERIFY2(controller, qPrintable(component.errorString()));
        QCOMPARE(controller->property("pageLabel").toString(), QStringLiteral("1 / 1"));
        QVERIFY(QMetaObject::invokeMethod(controller.get(), "nextPage"));
        QCOMPARE(controller->property("pageCount").toInt(), 2);
    }
};

QTEST_MAIN(TstUiDocument)
#include "tst_ui_document.moc"
