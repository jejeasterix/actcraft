#include <actcraft/core/document.h>
#include <actcraft/core/pagecache.h>
#include <actcraft/core/pagesettings.h>
#include <actcraft/core/undo.h>

#include <QtTest>

#include <cmath>
#include <cstddef>
#include <memory>

using namespace actcraft::core;

namespace {

// Espion du cache : compte chaque appel que applyToCache lui transmet.
struct Spy final : PageCache
{
    int inserts = 0;
    int removes = 0;
    int invalidations = 0;
    void insert(ObjectId) override { ++inserts; }
    void remove(ObjectId, const RectF&) override { ++removes; }
    void invalidate(ObjectId, const CacheArea&) override { ++invalidations; }
    int calls() const noexcept { return inserts + removes + invalidations; }
};

Spy spyOf(const Changes& changes)
{
    Spy spy;
    applyToCache(changes, 0, spy);
    return spy;
}

Grid aboveGrid(bool visible)
{
    Grid g;
    g.visible = visible;
    g.drawnAbove = true;
    g.levels.push_back(GridLevel{});
    return g;
}

bool isWholePageChange(const Changes& changes)
{
    return changes.size() == 1 && changes[0].kind == Change::Kind::PageChanged && changes[0].page == 0 &&
           changes[0].object == 0 && changes[0].area.width <= 0 && changes[0].area.height <= 0;
}

bool isIgnoredByTheCache(const Changes& changes)
{
    return changes.size() == 1 && changes[0].kind == Change::Kind::PageSettingsChanged &&
           changes[0].page == 0 && changes[0].object == 0;
}

} // namespace

class TstCorePageSettings : public QObject
{
    Q_OBJECT

private slots:
    // Le test qui garde le contrat : une grille qui ne dessine rien sous les objets, avant comme après, ne
    // touche pas le cache (elle est en surcouche, L4-T7) ; l'interface est prévenue par PageSettingsChanged.
    // Émettre PageChanged pour toute grille (patron de ClearPageCommand) le ferait échouer.
    void gridAboveChangeIsIgnoredByTheCache()
    {
        Document document;
        document.page(0).grid = aboveGrid(true);
        UndoStack stack;
        const Changes changes = stack.push(document, std::make_unique<SetGridCommand>(0, aboveGrid(false)));
        QCOMPARE(changes.size(), std::size_t{1}); // l'interface est prévenue
        QVERIFY(isIgnoredByTheCache(changes));
        QCOMPARE(spyOf(changes).calls(), 0);
        QVERIFY(!document.page(0).grid.visible);

        const Changes undone = stack.undo(document);
        QVERIFY(isIgnoredByTheCache(undone));
        QCOMPARE(spyOf(undone).calls(), 0);
        QVERIFY(document.page(0).grid.visible);

        // Grille du dessous cachée qu'on règle : paintGrid ne dessine rien d'une grille cachée.
        Grid hidden;
        hidden.levels.push_back(GridLevel{});
        hidden.levels[0].spacingX = 50.0;
        document.page(0).grid = Grid{};
        const Changes hiddenChanges = stack.push(document, std::make_unique<SetGridCommand>(0, hidden));
        QVERIFY(isIgnoredByTheCache(hiddenChanges));
        QCOMPARE(spyOf(hiddenChanges).calls(), 0);
    }

    // Une grille du dessous qui change, avant ou après : un seul redessin de toute la page.
    void gridBelowChangeRedrawsOnce()
    {
        Document document;
        Grid below = aboveGrid(true);
        below.drawnAbove = false;
        UndoStack stack;
        const Changes shown = stack.push(document, std::make_unique<SetGridCommand>(0, below));
        QVERIFY(isWholePageChange(shown));
        Spy spy = spyOf(shown);
        QCOMPARE(spy.invalidations, 1);
        QCOMPARE(spy.calls(), 1);

        // Passée au-dessus : la grille d'avant était dans le cache, il faut l'en retirer (avant compte).
        const Changes raised = stack.push(document, std::make_unique<SetGridCommand>(0, aboveGrid(true)));
        QVERIFY(isWholePageChange(raised));
        QCOMPARE(spyOf(raised).invalidations, 1);
        QVERIFY(document.page(0).grid.drawnAbove);

        // Annuler la remet dessous : de nouveau un redessin.
        const Changes lowered = stack.undo(document);
        QVERIFY(isWholePageChange(lowered));
        QCOMPARE(spyOf(lowered).invalidations, 1);
        QVERIFY(!document.page(0).grid.drawnAbove);
    }

    void backgroundChangeRedrawsOnce()
    {
        Document document;
        Background yellow;
        yellow.fill = solidFill(0xFFFFFF00u);
        UndoStack stack;
        const Changes changes = stack.push(document, std::make_unique<SetBackgroundCommand>(0, yellow));
        QVERIFY(isWholePageChange(changes));
        QCOMPARE(spyOf(changes).invalidations, 1);
        QCOMPARE(document.page(0).background.fill.color, yellow.fill.color);

        const Changes undone = stack.undo(document);
        QVERIFY(isWholePageChange(undone));
        QCOMPARE(document.page(0).background.fill.color, Background{}.fill.color);
    }

    // Pas cumulé, observé dans l'original (L4-O1) : 768 × 1,1³ = 1022,208 ; le pas fixe de 10 % de la hauteur
    // d'origine (D10, contredit) donnerait 998,4 et ferait échouer ce test.
    void extendStepsByTenPercentOfCurrent()
    {
        Document document; // 1024 × 768
        UndoStack stack;
        const Changes changes = stack.push(document, std::make_unique<ExtendPageCommand>(0));
        QVERIFY(isWholePageChange(changes));
        QCOMPARE(document.page(0).baseHeight, 768.0);
        QVERIFY(std::abs(document.page(0).height - 844.8) < 1e-3);
        stack.push(document, std::make_unique<ExtendPageCommand>(0));
        stack.push(document, std::make_unique<ExtendPageCommand>(0));
        QVERIFY(std::abs(document.page(0).height - 1022.208) < 1e-3);
        QCOMPARE(document.page(0).baseHeight, 768.0); // la hauteur d'origine reste celle du premier clic
        QCOMPARE(stack.count(), std::size_t{3});      // une étape par clic
        QVERIFY(std::abs(nextExtendedHeight(document.page(0)) - 1124.4288) < 1e-3);
    }

    void threeUndosRestoreTheHeight()
    {
        Document document;
        UndoStack stack;
        for (int i = 0; i < 3; ++i) {
            stack.push(document, std::make_unique<ExtendPageCommand>(0));
        }
        for (int i = 0; i < 3; ++i) {
            QVERIFY(isWholePageChange(stack.undo(document)));
        }
        QCOMPARE(document.page(0).height, 768.0);
        QCOMPARE(document.page(0).baseHeight, 0.0);
        stack.redo(document);
        QVERIFY(std::abs(document.page(0).height - 844.8) < 1e-9);
        QCOMPARE(document.page(0).baseHeight, 768.0);
    }

    void designerChangesMerge()
    {
        Document document;
        UndoStack stack;
        Grid g = aboveGrid(true);
        auto first = std::make_unique<SetGridCommand>(0, g, false);
        QCOMPARE(static_cast<const UndoCommand&>(*first).mergeId(), kMergeGrid);
        stack.push(document, std::move(first));
        g.levels[0].spacingX = 50.0;
        stack.push(document, std::make_unique<SetGridCommand>(0, g, true));
        g.levels[0].spacingX = 60.0;
        stack.push(document, std::make_unique<SetGridCommand>(0, g, true));
        QCOMPARE(stack.count(), std::size_t{1});
        const Changes undone = stack.undo(document);
        QVERIFY(!document.page(0).grid.visible); // la commande fusionnée garde l'avant de la première
        QVERIFY(isIgnoredByTheCache(undone));

        stack.redo(document);
        QCOMPARE(document.page(0).grid.levels[0].spacingX, 60.0); // et l'après de la dernière
        // Sans fusion demandée, une commande de grille ouvre une étape.
        stack.push(document, std::make_unique<SetGridCommand>(0, aboveGrid(false), false));
        QCOMPARE(stack.count(), std::size_t{2});
    }

    void closeMergeSeparatesTwoSessions() // amendement 4 (V7) : une étape par séance du créateur
    {
        Document document;
        UndoStack stack;
        Grid g = aboveGrid(true);
        stack.push(document, std::make_unique<SetGridCommand>(0, g, true));
        g.levels[0].spacingX = 50.0;
        stack.push(document, std::make_unique<SetGridCommand>(0, g, true));
        stack.closeMerge(); // fermeture de la séance (L4-T13)
        g.levels[0].spacingX = 60.0;
        stack.push(document, std::make_unique<SetGridCommand>(0, g, true));
        QCOMPARE(stack.count(), std::size_t{2});
        stack.undo(document);
        QCOMPARE(document.page(0).grid.levels[0].spacingX, 50.0);
        stack.undo(document);
        QVERIFY(!document.page(0).grid.visible);
    }
};

QTEST_GUILESS_MAIN(TstCorePageSettings)
#include "tst_core_pagesettings.moc"
