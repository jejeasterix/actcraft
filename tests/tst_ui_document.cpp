#include <actcraft/core/document.h>
#include <actcraft/core/edits.h>
#include <actcraft/core/generator.h>
#include <actcraft/render/fixedpageitem.h>
#include <actcraft/ui/documentcontroller.h>

#include <QQmlComponent>
#include <QQmlEngine>
#include <QRegularExpression>
#include <QSignalSpy>
#include <QSizeF>
#include <QStringList>
#include <QTest>
#include <QUrl>
#include <QtQml/qqmlextensionplugin.h>

#include <cstdint>
#include <limits>
#include <memory>
#include <utility>
#include <variant>
#include <vector>

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

// Document ouvert, page affichée et pile d'annulation (DA2, décision 16 ; relais du lot 1 défait).
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
        controller.pageBack();
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
        controller.pageBack();
        QCOMPARE(pages.count(), 0);
        QCOMPARE(controller.currentIndex(), 0);
    }

    void strokeLandsOnTheTopLayerOfTheShownPage()
    {
        render::FixedPageItem fixed;
        ui::DocumentController controller;
        controller.setFixedPage(&fixed);
        controller.commitStroke(redStroke());
        const core::Page& page = controller.document().page(0);
        QCOMPARE(page.objects.size(), std::size_t{1});
        QCOMPARE(page.objects[0].common.layer, core::Layer::Top);
        QVERIFY(page.objects[0].common.id != 0);
        QCOMPARE(fixed.objectCount(), 1);
        QCOMPARE(&fixed.page(), &page); // l'élément lit le document
        QVERIFY(controller.isModified());
    }

    // Un seul écrivain du document : la pile. Le relais du lot 1 écrivait dans l'élément puis recopiait.
    void everyWriteGoesThroughTheUndoStack()
    {
        render::FixedPageItem fixed;
        ui::DocumentController controller;
        controller.setFixedPage(&fixed);
        QSignalSpy modified(&controller, &ui::DocumentController::modifiedChanged);
        controller.commitStroke(redStroke());
        QVERIFY(controller.canUndo());
        controller.undo();
        QVERIFY(controller.document().page(0).objects.empty());
        QCOMPARE(fixed.objectCount(), 0);
        QVERIFY(!controller.isModified());
        controller.redo();
        QCOMPARE(fixed.objectCount(), 1);
        QVERIFY(controller.isModified());
        QVERIFY(modified.count() >= 3);
    }

    // Identifiants alloués par LE document : le relais en tirait un par page interne de l'élément (compteur
    // remis à 1 à chaque page montrée), d'où des doublons d'une page à l'autre. La page générée du banc
    // (identifiants 1 à 300) n'en partage aucun avec les autres pages ni avec les traits suivants.
    void idsStayUniqueAcrossPagesAndRoundTrips()
    {
        render::FixedPageItem fixed;
        ui::DocumentController controller;
        controller.setFixedPage(&fixed);
        controller.commitStroke(redStroke());
        controller.commitStroke(redStroke());
        controller.nextPage();
        controller.commitStroke(redStroke());
        controller.pageBack();
        controller.commitStroke(redStroke());
        controller.nextPage();
        controller.commitStroke(redStroke());
        QCOMPARE(controller.document().page(0).objects.size(), std::size_t{3});
        QCOMPARE(controller.document().page(1).objects.size(), std::size_t{2});
        QCOMPARE(fixed.objectCount(), 2);
        QVERIFY(core::hasUniqueIds(controller.document()));
        controller.loadGeneratedFirstPage(); // page 2 affichée
        QVERIFY(core::hasUniqueIds(controller.document()));
        QCOMPARE(controller.document().page(1).objects.size(), std::size_t{2});
        controller.pageBack();
        controller.commitStroke(redStroke());
        QCOMPARE(fixed.objectCount(), 301);
        QVERIFY(core::hasUniqueIds(controller.document()));
    }

    // UndoStack::push enregistre aussi une commande sans effet (hors périmètre, L2-T2) : le contrôleur ne
    // pousse pas un trait qui n'a rien ajouté (identifiants épuisés) et garde la branche à rétablir.
    void aStrokeThatAddsNothingIsNotRecorded()
    {
        render::FixedPageItem fixed;
        ui::DocumentController controller;
        controller.setFixedPage(&fixed);
        core::Document exhausted;
        exhausted.reserveIdsBelow(std::numeric_limits<core::ObjectId>::max());
        controller.replaceDocument(std::move(exhausted), QUrl(), QString());
        controller.nextPage();
        controller.undo();
        QVERIFY(controller.canRedo());
        QSignalSpy changed(&controller, &ui::DocumentController::documentChanged);
        QSignalSpy undo(&controller, &ui::DocumentController::undoChanged);
        QTest::ignoreMessage(QtWarningMsg, QRegularExpression(QStringLiteral("puis")));
        controller.commitStroke(redStroke());
        QVERIFY(controller.document().page(0).objects.empty());
        QVERIFY(!controller.canUndo());
        QVERIFY(controller.canRedo());
        QVERIFY(!controller.isModified());
        QCOMPARE(changed.count(), 0);
        QCOMPARE(undo.count(), 0);
    }

    void strokeSurvivesPageRoundTrip()
    {
        render::FixedPageItem fixed;
        ui::DocumentController controller;
        controller.setFixedPage(&fixed);
        controller.commitStroke(redStroke());
        controller.nextPage();
        QCOMPARE(fixed.objectCount(), 0);
        controller.pageBack();
        QCOMPARE(fixed.objectCount(), 1);
        QVERIFY(std::get<core::Stroke>(controller.document().page(0).objects[0]).color == 0xFFE53935u);
        QVERIFY(controller.document().page(1).objects.empty());
    }

    void addedPageIsUndoneAndTheShownPageStaysValid()
    {
        render::FixedPageItem fixed;
        ui::DocumentController controller;
        controller.setFixedPage(&fixed);
        controller.nextPage(); // page 2 ajoutée par la pile, affichée
        QCOMPARE(controller.pageLabel(), QStringLiteral("2 / 2"));
        QSignalSpy pages(&controller, &ui::DocumentController::pagesChanged);
        controller.undo();
        QCOMPARE(controller.pageCount(), 1);
        QCOMPARE(controller.currentIndex(), 0);
        QCOMPARE(fixed.sourcePage(), std::size_t{0});
        QCOMPARE(fixed.sourceDocument(), &controller.document());
        controller.redo();
        QCOMPARE(controller.pageCount(), 2);
        QCOMPARE(controller.pageLabel(), QStringLiteral("1 / 2"));
        QCOMPARE(pages.count(), 2); // le nombre de pages a changé deux fois
    }

    // Page insérée avant la page affichée : l'indice rendu par applyToCache est suivi, la page fixe relit la
    // même page à son nouvel indice (pas la page vierge insérée à sa place).
    void pageInsertedBeforeTheShownPageKeepsItShown()
    {
        render::FixedPageItem fixed;
        ui::DocumentController controller;
        controller.setFixedPage(&fixed);
        controller.commitStroke(redStroke());
        controller.execute(std::make_unique<core::AddPageCommand>(0, 1024, 768));
        QCOMPARE(controller.currentIndex(), 1);
        QCOMPARE(controller.pageLabel(), QStringLiteral("2 / 2"));
        QCOMPARE(fixed.sourcePage(), std::size_t{1});
        QCOMPARE(&fixed.page(), &controller.document().page(1));
        QCOMPARE(fixed.objectCount(), 1);
        controller.undo();
        QCOMPARE(controller.currentIndex(), 0);
        QCOMPARE(fixed.objectCount(), 1);
    }

    void newPageTakesTheLastPageSize()
    {
        render::FixedPageItem fixed;
        ui::DocumentController controller;
        controller.setFixedPage(&fixed);
        controller.newDocument(800, 600);
        controller.nextPage();
        QCOMPARE(controller.document().page(1).width, 800.0);
        QCOMPARE(controller.pageSize(), QSizeF(800, 600));
    }

    void newAndReplacedDocumentsAreCleanAndShownFromPageOne()
    {
        render::FixedPageItem fixed;
        ui::DocumentController controller;
        controller.setFixedPage(&fixed);
        QCOMPARE(controller.title(), QStringLiteral("Sans titre"));
        controller.commitStroke(redStroke());
        core::Document other(std::vector<core::Page>(3));
        other.setTitle("Autre");
        QSignalSpy pages(&controller, &ui::DocumentController::pagesChanged);
        controller.replaceDocument(std::move(other),
                                   QUrl::fromLocalFile(QStringLiteral("C:/x/autre.actcraft")),
                                   QStringLiteral("autre"));
        QCOMPARE(controller.pageLabel(), QStringLiteral("1 / 3"));
        QCOMPARE(controller.title(), QStringLiteral("autre"));
        QVERIFY(!controller.isModified());
        QVERIFY(!controller.canUndo());
        QCOMPARE(controller.fileUrl().fileName(), QStringLiteral("autre.actcraft"));
        QCOMPARE(pages.count(), 1);
        controller.newDocument(1280, 1024);
        QCOMPARE(controller.pageSize(), QSizeF(1280, 1024));
        QCOMPARE(controller.title(), QStringLiteral("Sans titre"));
        QVERIFY(controller.fileUrl().isEmpty());
    }

    void fingerprintPropertyFollowsTheDocument()
    {
        render::FixedPageItem fixed;
        ui::DocumentController controller;
        controller.setFixedPage(&fixed);
        const QString empty = controller.property("fingerprint").toString();
        QCOMPARE(empty.size(), 16);
        QSignalSpy changed(&controller, &ui::DocumentController::documentChanged);
        controller.commitStroke(redStroke());
        QCOMPARE(changed.count(), 1);
        QVERIFY(controller.property("fingerprint").toString() != empty);
        QCOMPARE(
            controller.property("fingerprint").toString(),
            QStringLiteral("%1").arg(core::fingerprint(controller.document()), 16, 16, QLatin1Char('0')));
    }

    void detachingTheItemLeavesItEmpty()
    {
        render::FixedPageItem fixed;
        ui::DocumentController controller;
        controller.setFixedPage(&fixed);
        controller.commitStroke(redStroke());
        controller.setFixedPage(nullptr);
        QVERIFY(fixed.sourceDocument() == nullptr);
        QCOMPARE(fixed.objectCount(), 0);
        QCOMPARE(controller.document().page(0).objects.size(), std::size_t{1}); // rien n'est perdu
    }

    // pagesChanged : une fois par déplacement, jamais sans déplacement, et émis quand tout est en place (le
    // libellé et l'élément montrent déjà la nouvelle page, y compris celle qui vient d'être ajoutée).
    void pagesChangedOncePerMoveWithTheNewPageSettled()
    {
        render::FixedPageItem fixed;
        ui::DocumentController controller;
        controller.setFixedPage(&fixed);
        controller.commitStroke(redStroke());
        QStringList seen;
        connect(&controller, &ui::DocumentController::pagesChanged, this, [&] {
            seen << QStringLiteral("%1 : %2").arg(controller.pageLabel()).arg(fixed.objectCount());
        });
        controller.nextPage(); // ajoute la page 2 et l'affiche
        controller.pageBack(); // retour sur la page 1
        controller.nextPage(); // la page 2 existe déjà
        controller.pageBack();
        controller.pageBack(); // première page : pas de déplacement
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

    // La vue termine le trait en cours sur pageAboutToChange : ce qu'elle verse alors doit rejoindre la page
    // quittée, donc le signal précède le déplacement.
    void pageAboutToChangeComesBeforeTheMove()
    {
        render::FixedPageItem fixed;
        ui::DocumentController controller;
        controller.setFixedPage(&fixed);
        QSignalSpy leaving(&controller, &ui::DocumentController::pageAboutToChange);
        connect(&controller, &ui::DocumentController::pageAboutToChange, &fixed,
                [&controller] { controller.commitStroke(redStroke()); });
        controller.nextPage();
        QCOMPARE(leaving.count(), 1);
        QCOMPARE(fixed.objectCount(), 0);
        QCOMPARE(controller.document().page(0).objects.size(), std::size_t{1});
        controller.pageBack();
        QCOMPARE(leaving.count(), 2);
        QCOMPARE(fixed.objectCount(), 1);
        QCOMPARE(controller.document().page(1).objects.size(), std::size_t{1});
        controller.pageBack(); // première page : aucun déplacement, aucun signal
        QCOMPARE(leaving.count(), 2);
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
        QVERIFY(!controller.isModified());
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
