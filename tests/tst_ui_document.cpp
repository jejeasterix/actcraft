#include <actcraft/core/document.h>
#include <actcraft/core/edits.h>
#include <actcraft/core/generator.h>
#include <actcraft/core/objectedits.h>
#include <actcraft/core/pagecache.h>
#include <actcraft/core/pagezones.h>
#include <actcraft/core/replace.h>
#include <actcraft/render/fixedpageitem.h>
#include <actcraft/ui/documentcontroller.h>

#include <QQmlComponent>
#include <QQmlEngine>
#include <QRegularExpression>
#include <QSignalSpy>
#include <QSizeF>
#include <QString>
#include <QStringList>
#include <QTemporaryDir>
#include <QTest>
#include <QUrl>
#include <QtQml/qqmlextensionplugin.h>

#include <cstddef>
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

// Forme de la couche des traits (couche supérieure) : remplacer un trait par elle la garde à son rang.
core::PageObject topSquare()
{
    core::Shape shape;
    shape.kind = core::ShapeKind::Rectangle;
    shape.bounds = core::RectF{150, 150, 80, 80};
    shape.pen.width = 4;
    core::PageObject object = shape;
    object.common.layer = core::Layer::Top;
    return object;
}

using Origin = ui::DocumentController::ChangeOrigin;

// Un seul observateur (D-S7) : ni setCacheObserver (lot 3) ni setStrokeObserver (lot 7) gardés à côté.
template <typename T>
constexpr bool hasCacheObserverHook = requires(T& controller) { controller.setCacheObserver(nullptr); };
template <typename T>
constexpr bool hasStrokeObserverHook = requires(T& controller) { controller.setStrokeObserver(nullptr); };
static_assert(!hasCacheObserverHook<ui::DocumentController>);
static_assert(!hasStrokeObserverHook<ui::DocumentController>);

// Cache espion : les appels que core::applyToCache lui envoie, dans l'ordre.
class RecordingCache final : public core::PageCache
{
public:
    void insert(core::ObjectId id) override { calls << QStringLiteral("insert %1").arg(id); }
    void remove(core::ObjectId id, const core::RectF&) override
    {
        calls << QStringLiteral("remove %1").arg(id);
    }
    void invalidate(core::ObjectId id, const core::CacheArea&) override
    {
        calls << QStringLiteral("invalidate %1").arg(id);
    }
    QStringList calls;
};

// Observateur comme les espions des lots 3 à 5 : rejoue ce qu'il reçoit sur un cache espion par
// core::applyToCache(changes, currentIndex(), espion), et garde les changements bruts et leur origine.
struct ObserverSpy
{
    RecordingCache cache;
    std::vector<core::Changes> received;
    std::vector<Origin> origins;
    std::vector<int> shownDuringCall; // currentIndex() pendant l'appel
    int token = 0;

    void attach(ui::DocumentController& controller)
    {
        token =
            controller.addChangeObserver([this, &controller](const core::Changes& applied, Origin origin) {
                received.push_back(applied);
                origins.push_back(origin);
                shownDuringCall.push_back(controller.currentIndex());
                core::applyToCache(applied, static_cast<std::size_t>(controller.currentIndex()), cache);
            });
    }
};

// Enregistre dans `path` et attend la fin de l'écriture ; vrai si elle a réussi.
bool saveAndWait(ui::DocumentController& controller, const QString& path)
{
    QSignalSpy saved(&controller, &ui::DocumentController::saveFinished);
    controller.saveToUrl(QUrl::fromLocalFile(path));
    return saved.wait(10000) && saved.first().first().toBool();
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

    // Signal d'état de la pile (canUndo, canRedo) : à chaque écriture, annulation et rétablissement.
    void undoChangedFollowsEveryStackMove()
    {
        render::FixedPageItem fixed;
        ui::DocumentController controller;
        controller.setFixedPage(&fixed);
        QSignalSpy undo(&controller, &ui::DocumentController::undoChanged);
        controller.commitStroke(redStroke());
        QCOMPARE(undo.count(), 1);
        controller.undo();
        QCOMPARE(undo.count(), 2);
        controller.redo();
        QCOMPARE(undo.count(), 3);
    }

    // Page affichée retirée par une annulation : même indice, borné au nombre de pages (pas la page 1).
    void undoingTheShownLastPageShowsTheNewLastPage()
    {
        render::FixedPageItem fixed;
        ui::DocumentController controller;
        controller.setFixedPage(&fixed);
        controller.replaceDocument(core::Document(std::vector<core::Page>(3)), QUrl(), QString());
        controller.nextPage();
        controller.nextPage();
        controller.nextPage(); // page 4 ajoutée et affichée
        QCOMPARE(controller.pageLabel(), QStringLiteral("4 / 4"));
        controller.undo();
        QCOMPARE(controller.pageLabel(), QStringLiteral("3 / 3"));
        QCOMPARE(fixed.sourcePage(), std::size_t{2});
    }

    // Contrôleur détruit avant l'élément : l'élément ne lit plus le document détruit (page interne vide).
    void destroyedControllerDetachesTheItem()
    {
        render::FixedPageItem fixed;
        {
            ui::DocumentController controller;
            controller.setFixedPage(&fixed);
            controller.commitStroke(redStroke());
            QCOMPARE(fixed.objectCount(), 1);
        }
        QVERIFY(fixed.sourceDocument() == nullptr);
        QCOMPARE(fixed.objectCount(), 0);
    }

    // Page générée du banc : la taille de la page affichée suit (pagesChanged) ; « modifié » reste ce qu'il
    // était (la page du banc n'est pas une modification, mais n'efface pas celles des autres pages).
    void generatedFirstPageKeepsSizeAndModifiedState()
    {
        render::FixedPageItem fixed;
        ui::DocumentController controller;
        controller.setFixedPage(&fixed);
        controller.newDocument(800, 600);
        QSignalSpy pages(&controller, &ui::DocumentController::pagesChanged);
        controller.loadGeneratedFirstPage();
        QCOMPARE(pages.count(), 1);
        const core::Page& first = controller.document().page(0);
        QCOMPARE(controller.pageSize(), QSizeF(first.width, first.height));
        QVERIFY(controller.pageSize() != QSizeF(800, 600));
        QVERIFY(!controller.isModified());
        controller.nextPage();
        controller.commitStroke(redStroke()); // page 2 modifiée
        QSignalSpy modified(&controller, &ui::DocumentController::modifiedChanged);
        controller.loadGeneratedFirstPage(); // page 2 affichée : sa taille ne change pas
        QCOMPARE(pages.count(), 2);
        QVERIFY(controller.isModified());
        QVERIFY(!controller.canUndo());
        QCOMPARE(modified.count(), 0);
        controller.newDocument(800, 600);
        QVERIFY(!controller.isModified()); // remplacement : propre
    }

    // Observateur unique (L0-S5, D-S7) : il reçoit exactement les changements passés à core::applyToCache.
    void observerGetsTheCacheChanges()
    {
        render::FixedPageItem fixed;
        ui::DocumentController controller;
        controller.setFixedPage(&fixed);
        ObserverSpy spy;
        spy.attach(controller);
        controller.commitStroke(redStroke());
        const core::ObjectId id = controller.document().page(0).objects[0].common.id;
        QCOMPARE(spy.cache.calls, QStringList{QStringLiteral("insert %1").arg(id)});
        controller.undo();
        QCOMPARE(spy.cache.calls,
                 (QStringList{QStringLiteral("insert %1").arg(id), QStringLiteral("remove %1").arg(id)}));
        QCOMPARE(spy.received.size(), std::size_t{2});
        // Page insérée avant la page affichée : l'observateur est appelé avant le recalage, avec l'indice
        // que la page fixe a reçu (sinon son espion ne rejouerait pas les mêmes appels).
        controller.execute(std::make_unique<core::AddPageCommand>(0, 1024, 768));
        QCOMPARE(spy.shownDuringCall.back(), 0);
        QCOMPARE(controller.currentIndex(), 1);
        controller.commitStroke(redStroke());
        const core::ObjectId second = controller.document().page(1).objects[0].common.id;
        QCOMPARE(spy.cache.calls.back(), QStringLiteral("insert %1").arg(second));
    }

    void observerSeesTheOrigin()
    {
        ui::DocumentController controller; // sans page fixe : l'observateur est appelé quand même
        ObserverSpy spy;
        spy.attach(controller);
        controller.commitStroke(redStroke());
        controller.execute(std::make_unique<core::AddObjectCommand>(0, topSquare()));
        controller.undo();
        controller.redo();
        controller.nextPage(); // page ajoutée par la pile
        const std::vector<Origin> expected{Origin::Stroke, Origin::Command, Origin::Undo, Origin::Redo,
                                           Origin::Command};
        QCOMPARE(spy.origins.size(), expected.size());
        for (std::size_t i = 0; i < expected.size(); ++i) {
            QCOMPARE(static_cast<int>(spy.origins[i]), static_cast<int>(expected[i]));
        }
    }

    void removedObserverIsSilent()
    {
        ui::DocumentController controller;
        QStringList log;
        const int first = controller.addChangeObserver(
            [&log](const core::Changes&, Origin) { log << QStringLiteral("a"); });
        const int second = controller.addChangeObserver(
            [&log](const core::Changes&, Origin) { log << QStringLiteral("b"); });
        QVERIFY(first > 0);
        QVERIFY(second > 0);
        QVERIFY(first != second);
        controller.commitStroke(redStroke());
        QCOMPARE(log, (QStringList{QStringLiteral("a"), QStringLiteral("b")})); // ordre d'ajout
        controller.removeChangeObserver(first);
        controller.commitStroke(redStroke());
        controller.undo();
        QCOMPARE(log, (QStringList{QStringLiteral("a"), QStringLiteral("b"), QStringLiteral("b"),
                                   QStringLiteral("b")}));
        controller.removeChangeObserver(second);
        controller.redo();
        QCOMPARE(log.size(), 4);
        // Un observateur qui se retire pendant son appel : appelé une fois, la diffusion continue sans lui.
        int self = 0;
        self = controller.addChangeObserver([&log, &controller, &self](const core::Changes&, Origin) {
            log << QStringLiteral("c");
            controller.removeChangeObserver(self);
        });
        QVERIFY(self > second);
        controller.commitStroke(redStroke());
        controller.commitStroke(redStroke());
        QCOMPARE(log.size(), 5);
        QCOMPARE(log.back(), QStringLiteral("c"));
    }

    // Remplacer un trait du milieu (règle du lot 10) : ni le retrait ni l'insertion hors fin de page
    // n'attendent, le cache est redessiné tout de suite ; l'annulation aussi.
    void replaceInTheMiddleIsNotDeferred()
    {
        render::FixedPageItem fixed;
        ui::DocumentController controller;
        controller.setFixedPage(&fixed);
        QTRY_VERIFY(fixed.isReady());
        fixed.setDeferredFolding(true);
        controller.commitStroke(redStroke());
        controller.commitStroke(redStroke());
        controller.commitStroke(redStroke());
        QCOMPARE(fixed.pendingCount(), 3); // en fin de page : ils attendent au-dessus du cache
        fixed.foldPending();
        QCOMPARE(fixed.pendingCount(), 0);
        const core::ObjectId middle = controller.document().page(0).objects[1].common.id;
        QSignalSpy content(&fixed, &render::FixedPageItem::contentChanged);
        controller.execute(std::make_unique<core::ReplaceObjectsCommand>(
            0, std::vector<core::ObjectId>{middle}, std::vector<core::PageObject>{topSquare()}));
        QCOMPARE(fixed.pendingCount(), 0);
        QVERIFY(content.count() >= 1);
        QCOMPARE(controller.shownObjectCount(), 3);
        controller.undo();
        QCOMPARE(fixed.pendingCount(), 0);
        QCOMPARE(controller.document().page(0).objects[1].common.id, middle);
    }

    // Réglages de présentation (I3) : par la pile, sans redessin du cache, mais transmis à l'observateur.
    void presentationChangeRedrawsNothing()
    {
        render::FixedPageItem fixed;
        ui::DocumentController controller;
        controller.setFixedPage(&fixed);
        QTRY_VERIFY(fixed.isReady());
        ObserverSpy spy;
        spy.attach(controller);
        QSignalSpy content(&fixed, &render::FixedPageItem::contentChanged);
        QSignalSpy document(&controller, &ui::DocumentController::documentChanged);
        // La valeur de la page : aucun changement, donc ni pile ni observateur (run filtre avant la pile).
        controller.execute(std::make_unique<core::SetPagePresentationCommand>(0, core::PagePresentation{}));
        QVERIFY(!controller.canUndo());
        QVERIFY(spy.received.empty());
        QCOMPARE(document.count(), 0);
        core::PagePresentation presentation;
        presentation.hasOrigin = true;
        presentation.origin = core::PointF{120, 80};
        controller.execute(std::make_unique<core::SetPagePresentationCommand>(0, presentation));
        QCOMPARE(content.count(), 0);
        QVERIFY(spy.cache.calls.isEmpty());
        QCOMPARE(document.count(), 1);
        QVERIFY(controller.canUndo());
        // L'observateur reçoit le changement brut, que le cache ignore (un observateur fait de cache ne le
        // verrait jamais).
        QCOMPARE(spy.received.size(), std::size_t{1});
        QCOMPARE(spy.received[0].size(), std::size_t{1});
        QVERIFY(spy.received[0][0].kind == core::Change::Kind::PageSettingsChanged);
        QCOMPARE(static_cast<int>(spy.origins[0]), static_cast<int>(Origin::Command));
    }

    void shownObjectCountCounts()
    {
        ui::DocumentController controller;
        QCOMPARE(controller.shownObjectCount(), 0);
        controller.commitStroke(redStroke());
        controller.commitStroke(redStroke());
        QCOMPARE(controller.shownObjectCount(), 2);
        QCOMPARE(controller.property("shownObjectCount").toInt(), 2);
        controller.nextPage();
        QCOMPARE(controller.shownObjectCount(), 0); // page affichée seulement
    }

    // B1 du lot 3 : soulever n'annonce pas d'édition (l'outil qui soulève ne s'annule pas lui-même). Le corps
    // est un talon au socle ; l'assertion reste vraie quand L3-T12 le remplit.
    void liftDoesNotAnnounceAnEdit()
    {
        render::FixedPageItem fixed;
        ui::DocumentController controller;
        controller.setFixedPage(&fixed);
        controller.commitStroke(redStroke());
        const core::ObjectId id = controller.document().page(0).objects[0].common.id;
        QSignalSpy edit(&controller, &ui::DocumentController::editAboutToApply);
        controller.liftObjects({});
        QCOMPARE(edit.count(), 0);
        controller.liftObjects({id});
        QCOMPARE(edit.count(), 0);
    }

    // Rétablir la page (F-131, D9) : la page telle qu'enregistrée en dernier, ou telle qu'ouverte.
    void revertPageReturnsToSaved()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        ui::DocumentController controller;
        controller.commitStroke(redStroke());
        const QString path = dir.filePath(QStringLiteral("a.actcraft"));
        QVERIFY(saveAndWait(controller, path));
        const QString saved = controller.fingerprintText();
        QVERIFY(!controller.canRevertPage()); // la page est celle du fichier
        controller.commitStroke(redStroke());
        QVERIFY(controller.canRevertPage());
        controller.revertPage();
        QCOMPARE(controller.fingerprintText(), saved);
        QCOMPARE(controller.shownObjectCount(), 1); // le trait enregistré reste : pas une page vierge
        QVERIFY(controller.canUndo());
        QVERIFY(!controller.canRevertPage());
        // Ouvert et jamais réenregistré : la page telle qu'ouverte.
        ui::DocumentController opened;
        QSignalSpy loaded(&opened, &ui::DocumentController::loadFinished);
        opened.openUrl(QUrl::fromLocalFile(path));
        QVERIFY(loaded.wait(10000));
        QCOMPARE(loaded.first().first().toBool(), true);
        const QString atOpening = opened.fingerprintText();
        QCOMPARE(opened.shownObjectCount(), 1);
        QVERIFY(!opened.canRevertPage());
        opened.commitStroke(redStroke());
        opened.revertPage();
        QCOMPARE(opened.fingerprintText(), atOpening);
        QCOMPARE(opened.shownObjectCount(), 1);
    }

    // Rétablir la page d'une page jamais enregistrée garde ses zones (mise en page, pas contenu) : une page
    // réglée en 2 × 3 cadres n'a rien à rétablir, et le rétablissement après un trait les garde.
    void revertBlankPageKeepsItsZones()
    {
        ui::DocumentController controller;
        core::PageZones zones;
        zones.horizontal = 2;
        zones.vertical = 3;
        controller.execute(std::make_unique<core::SetPageZonesCommand>(0, zones));
        QVERIFY(!controller.canRevertPage());
        controller.commitStroke(redStroke());
        QVERIFY(controller.canRevertPage());
        controller.revertPage();
        QCOMPARE(controller.shownObjectCount(), 0);
        QCOMPARE(controller.document().page(0).zones.horizontal, 2);
        QCOMPARE(controller.document().page(0).zones.vertical, 3);
        QVERIFY(!controller.canRevertPage());
    }

    // L3-T16 : la référence de « Rétablir la page » change à la fin d'un enregistrement ; Main.qml relit
    // canRevertPage à documentChanged : le signal doit donc partir à ce moment-là, sinon l'entrée reste
    // active.
    void revertPageRenotifiedAtEndOfSave()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        ui::DocumentController controller;
        controller.commitStroke(redStroke());
        QVERIFY(controller.canRevertPage());
        QSignalSpy changed(&controller, &ui::DocumentController::canRevertPageChanged);
        QVERIFY(saveAndWait(controller, dir.filePath(QStringLiteral("a.actcraft"))));
        QVERIFY(changed.count() >= 1);
        QVERIFY(!controller.canRevertPage());
    }

    // Jamais enregistré : page vierge de même taille (D9). « Nouveau » après un enregistrement : la référence
    // de l'ancien document ne vaut plus.
    void revertNeverSavedEmptiesPage()
    {
        ui::DocumentController controller;
        controller.newDocument(800, 600);
        controller.commitStroke(redStroke());
        controller.commitStroke(redStroke());
        QVERIFY(controller.canRevertPage());
        controller.revertPage();
        QCOMPARE(controller.shownObjectCount(), 0);
        QCOMPARE(controller.pageSize(), QSizeF(800, 600));
        QVERIFY(controller.canUndo());
        QVERIFY(!controller.canRevertPage());
        controller.undo(); // une étape
        QCOMPARE(controller.shownObjectCount(), 2);
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        QVERIFY(saveAndWait(controller, dir.filePath(QStringLiteral("a.actcraft"))));
        controller.newDocument(800, 600);
        controller.commitStroke(redStroke());
        controller.revertPage();
        QCOMPARE(controller.shownObjectCount(), 0);
    }

    // D9 (L3-T0, amendement de l'orchestrateur) : une étape, et le document reste modifié, même quand la page
    // redevient celle du fichier (la pile n'est pas remontée jusqu'à l'état propre).
    void revertPageLeavesTheDocumentModified()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        ui::DocumentController controller;
        controller.commitStroke(redStroke());
        QVERIFY(saveAndWait(controller, dir.filePath(QStringLiteral("a.actcraft"))));
        QVERIFY(!controller.isModified());
        controller.commitStroke(redStroke());
        QSignalSpy undoSpy(&controller, &ui::DocumentController::undoChanged);
        controller.revertPage();
        QCOMPARE(controller.shownObjectCount(), 1); // contenu du fichier
        QVERIFY(controller.isModified());
        QCOMPARE(controller.property("modified").toBool(), true);
        QCOMPARE(undoSpy.count(), 1);
        QVERIFY(!controller.canRedo());
        controller.undo(); // une seule étape : le second trait revient
        QCOMPARE(controller.shownObjectCount(), 2);
        QVERIFY(controller.canUndo()); // les traits restent annulables
    }

    // L2-T15 : editAboutToApply avant toute édition qui n'est pas un trait, avant le mouvement de la pile ;
    // jamais au soulèvement ni au dépôt (B1).
    void undoAnnouncesTheEditFirst()
    {
        ui::DocumentController controller;
        QStringList log;
        connect(&controller, &ui::DocumentController::editAboutToApply, &controller,
                [&log] { log << QStringLiteral("edit"); });
        connect(&controller, &ui::DocumentController::undoChanged, &controller,
                [&log] { log << QStringLiteral("undo"); });
        const QStringList announced{QStringLiteral("edit"), QStringLiteral("undo")};
        controller.commitStroke(redStroke());
        controller.commitStroke(redStroke());
        log.clear();
        controller.undo();
        QCOMPARE(log, announced);
        log.clear();
        controller.redo();
        QCOMPARE(log, announced);
        log.clear();
        controller.revertPage(); // jamais enregistré : page vierge
        QCOMPARE(log, announced);
        controller.undo();
        const core::ObjectId first = controller.document().page(0).objects.front().common.id;
        log.clear();
        controller.deleteObjects({first});
        QCOMPARE(log, announced);
        QCOMPARE(controller.shownObjectCount(), 1);
        log.clear();
        controller.clearPage(static_cast<int>(core::ClearKind::Annotations));
        QCOMPARE(log, announced);
        QCOMPARE(controller.shownObjectCount(), 0);
        controller.commitStroke(redStroke());
        const core::ObjectId id = controller.document().page(0).objects.back().common.id;
        log.clear();
        controller.liftObjects({id});
        QVERIFY(log.isEmpty());
        controller.dropLifted(std::make_unique<core::TransformObjectsCommand>(
            0, std::vector<core::ObjectId>{id}, core::translation(10, 0)));
        QCOMPARE(log, QStringList{QStringLiteral("undo")});

        // La vue termine son trait à l'annonce (Main.qml : finishStroke) : Annuler annule ce trait, et la
        // branche à rétablir reste entière.
        ui::DocumentController view;
        view.commitStroke(redStroke());
        const core::ObjectId kept = view.document().page(0).objects.front().common.id;
        bool inProgress = true;
        connect(&view, &ui::DocumentController::editAboutToApply, &view, [&] {
            if (inProgress) {
                inProgress = false;
                view.commitStroke(redStroke());
            }
        });
        view.undo();
        QVERIFY(!inProgress);
        QCOMPARE(view.shownObjectCount(), 1);
        QCOMPARE(view.document().page(0).objects.front().common.id, kept);
        QVERIFY(view.canRedo());
    }

    // Effacer ▸ Annotations (F-130) : les traits partent, la forme ajoutée par la pile reste ; une étape.
    void clearPageAnnotations()
    {
        ui::DocumentController controller;
        controller.commitStroke(redStroke());
        controller.commitStroke(redStroke());
        controller.execute(std::make_unique<core::AddObjectCommand>(0, topSquare()));
        controller.clearPage(static_cast<int>(core::ClearKind::Annotations));
        QCOMPARE(controller.shownObjectCount(), 1);
        QVERIFY(std::holds_alternative<core::Shape>(controller.document().page(0).objects[0]));
        controller.clearPage(99); // inconnu : rien
        QCOMPARE(controller.shownObjectCount(), 1);
        controller.undo();
        QCOMPARE(controller.shownObjectCount(), 3);
    }

    // Marqueur (F-225, L3-T6) : le trait arrive opaque, sa translucidité à part ; le document la garde, en
    // une étape d'annulation d'origine Stroke, sur la couche supérieure.
    void highlighterKeepsItsTranslucency()
    {
        ui::DocumentController controller;
        ObserverSpy spy;
        spy.attach(controller);
        controller.commitAnnotation(redStroke(), 128);
        QCOMPARE(controller.shownObjectCount(), 1);
        const core::PageObject& object = controller.document().page(0).objects.back();
        QCOMPARE(static_cast<int>(object.common.translucency), 128);
        QVERIFY(object.common.layer == core::Layer::Top);
        QVERIFY(std::get<core::Stroke>(object).color == 0xFFE53935u); // couleur gardée opaque
        QCOMPARE(spy.origins.size(), std::size_t{1});
        QCOMPARE(static_cast<int>(spy.origins[0]), static_cast<int>(Origin::Stroke));
        QVERIFY(controller.canUndo());
        controller.undo();
        QCOMPARE(controller.shownObjectCount(), 0);
        QVERIFY(!controller.canUndo());
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

    void pageContentChangedNamesThePage()
    {
        actcraft::ui::DocumentController controller;
        controller.newDocument(1024, 768);
        controller.nextPage(); // deux pages, la 2e affichée
        QSignalSpy spy(&controller, &actcraft::ui::DocumentController::pageContentChanged);
        actcraft::core::Stroke stroke;
        stroke.points = {{10, 10, 0.5f}, {50, 50, 0.5f}};
        controller.commitStroke(stroke);
        QCOMPARE(spy.count(), 1);
        QCOMPARE(spy.at(0).at(0).toInt(), 1);
    }

    void showPageShowsIt()
    {
        actcraft::ui::DocumentController controller;
        controller.newDocument(1024, 768);
        controller.nextPage();
        controller.nextPage();
        controller.showPage(0);
        QCOMPARE(controller.currentIndex(), 0);
        controller.showPage(9);
        QCOMPARE(controller.currentIndex(), 2);
    }
};

QTEST_MAIN(TstUiDocument)
#include "tst_ui_document.moc"
