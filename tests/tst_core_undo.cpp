#include <actcraft/core/document.h>
#include <actcraft/core/undo.h>

#include <QTest>

#include <memory>

using namespace actcraft::core;

namespace {

// Commande de test : ajoute `step` au titre du document (longueur = nombre d'étapes).
class Append final : public UndoCommand
{
public:
    explicit Append(char step, int mergeKey = -1) : m_steps(1, step), m_mergeKey(mergeKey) {}
    void redo(Document& d, Changes& changes) override
    {
        d.setTitle(d.title() + m_steps);
        changes.push_back(Change{Change::Kind::PageChanged, 0, 0, RectF{}});
    }
    void undo(Document& d, Changes& changes) override
    {
        d.setTitle(d.title().substr(0, d.title().size() - m_steps.size()));
        changes.push_back(Change{Change::Kind::PageChanged, 0, 0, RectF{}});
    }
    int mergeId() const noexcept override { return m_mergeKey; }
    bool mergeWith(const UndoCommand& other) override
    {
        m_steps += static_cast<const Append&>(other).m_steps;
        return true;
    }

private:
    std::string m_steps;
    int m_mergeKey;
};

// Commande déjà exécutée une fois par l'appelant : compte ses rejeux et ses annulations.
class Counted final : public UndoCommand
{
public:
    Counted(int* redos, int* undos, int mergeKey = -1) : m_redos(redos), m_undos(undos), m_mergeKey(mergeKey)
    {}
    void redo(Document& d, Changes&) override
    {
        ++*m_redos;
        d.setTitle(d.title() + 'x');
    }
    void undo(Document& d, Changes&) override
    {
        ++*m_undos;
        d.setTitle(d.title().substr(0, d.title().size() - 1));
    }
    int mergeId() const noexcept override { return m_mergeKey; }
    bool mergeWith(const UndoCommand&) override { return true; }

private:
    int* m_redos;
    int* m_undos;
    int m_mergeKey;
};

} // namespace

class TstCoreUndo : public QObject
{
    Q_OBJECT

private slots:
    void pushRunsTheCommandAndReturnsItsChanges()
    {
        Document d;
        UndoStack stack;
        const Changes changes = stack.push(d, std::make_unique<Append>('a'));
        QCOMPARE(d.title(), std::string("a"));
        QCOMPARE(changes.size(), std::size_t{1});
        QVERIFY(stack.canUndo());
        QVERIFY(!stack.canRedo());
        QCOMPARE(stack.count(), std::size_t{1});
        QCOMPARE(stack.index(), std::size_t{1});
    }

    void undoAndRedoWalkTheStack()
    {
        Document d;
        UndoStack stack;
        stack.push(d, std::make_unique<Append>('a'));
        stack.push(d, std::make_unique<Append>('b'));
        QCOMPARE(stack.undo(d).size(), std::size_t{1});
        QCOMPARE(d.title(), std::string("a"));
        QVERIFY(stack.canRedo());
        stack.redo(d);
        QCOMPARE(d.title(), std::string("ab"));
        stack.undo(d);
        stack.undo(d);
        QCOMPARE(d.title(), std::string());
        QVERIFY(!stack.canUndo());
        QVERIFY(stack.undo(d).empty()); // rien à annuler : aucun changement
    }

    void pushAfterUndoDropsTheRedoBranch()
    {
        Document d;
        UndoStack stack;
        stack.push(d, std::make_unique<Append>('a'));
        stack.push(d, std::make_unique<Append>('b'));
        stack.undo(d);
        stack.push(d, std::make_unique<Append>('c'));
        QCOMPARE(d.title(), std::string("ac"));
        QVERIFY(!stack.canRedo());
        QCOMPARE(stack.count(), std::size_t{2});
    }

    void limitDropsTheOldestCommands()
    {
        Document d;
        UndoStack stack(3);
        QCOMPARE(stack.limit(), std::size_t{3});
        for (const char c : std::string("abcde")) {
            stack.push(d, std::make_unique<Append>(c));
        }
        QCOMPARE(stack.count(), std::size_t{3});
        stack.undo(d);
        stack.undo(d);
        stack.undo(d);
        QCOMPARE(d.title(), std::string("ab")); // « a » et « b » ne s'annulent plus
        QVERIFY(!stack.canUndo());
    }

    void defaultLimitIsFiveHundred() { QCOMPARE(UndoStack().limit(), std::size_t{500}); }

    void cleanStateFollowsTheSavedIndex()
    {
        Document d;
        UndoStack stack;
        QVERIFY(stack.isClean());
        stack.push(d, std::make_unique<Append>('a'));
        QVERIFY(!stack.isClean());
        stack.setClean();
        QVERIFY(stack.isClean());
        stack.push(d, std::make_unique<Append>('b'));
        QVERIFY(!stack.isClean());
        stack.undo(d);
        QVERIFY(stack.isClean()); // revenu à l'état enregistré
        stack.undo(d);
        QVERIFY(!stack.isClean());
    }

    void cleanStateLostWhenItsBranchIsDropped()
    {
        Document d;
        UndoStack stack;
        stack.push(d, std::make_unique<Append>('a'));
        stack.setClean();
        stack.undo(d);
        stack.push(d, std::make_unique<Append>('b')); // l'état enregistré (« a ») n'est plus atteignable
        stack.undo(d);
        QVERIFY(!stack.isClean());
        stack.redo(d);
        QVERIFY(!stack.isClean());
    }

    void cleanStateLostWhenTheLimitDropsIt()
    {
        Document d;
        UndoStack stack(2);
        stack.setClean(); // document vide enregistré
        stack.push(d, std::make_unique<Append>('a'));
        stack.push(d, std::make_unique<Append>('b'));
        stack.push(d, std::make_unique<Append>('c'));
        stack.undo(d);
        stack.undo(d);
        QVERIFY(!stack.isClean()); // « a » reste : ce n'est pas l'état enregistré
    }

    void consecutiveCommandsOfTheSameFamilyMerge()
    {
        Document d;
        UndoStack stack;
        stack.push(d, std::make_unique<Append>('a', 7));
        stack.push(d, std::make_unique<Append>('b', 7));
        stack.push(d, std::make_unique<Append>('c', 8));
        QCOMPARE(stack.count(), std::size_t{2});
        stack.undo(d);
        QCOMPARE(d.title(), std::string("ab"));
        stack.undo(d);
        QCOMPARE(d.title(), std::string()); // « a » et « b » fusionnés : une seule étape
    }

    void mergingIntoTheCleanCommandLosesTheCleanState()
    {
        Document d;
        UndoStack stack;
        stack.push(d, std::make_unique<Append>('a', 7));
        stack.setClean();
        stack.push(d, std::make_unique<Append>('b', 7));
        QVERIFY(!stack.isClean());
    }

    void clearEmptiesAndMarksClean()
    {
        Document d;
        UndoStack stack;
        stack.push(d, std::make_unique<Append>('a'));
        stack.clear();
        QCOMPARE(stack.count(), std::size_t{0});
        QVERIFY(stack.isClean());
        QVERIFY(!stack.canUndo());
    }

    void pushExecutedDoesNotReplayTheCommand()
    {
        Document d;
        d.setTitle("x"); // l'appelant a déjà appliqué la commande
        UndoStack stack;
        int redos = 0;
        int undos = 0;
        stack.pushExecuted(std::make_unique<Counted>(&redos, &undos));
        QCOMPARE(redos, 0);
        QCOMPARE(d.title(), std::string("x"));
        QCOMPARE(stack.count(), std::size_t{1});
        QCOMPARE(stack.index(), std::size_t{1});
        QVERIFY(stack.canUndo());
        QVERIFY(!stack.canRedo());
    }

    void pushExecutedCommandUndoesAndRedoes()
    {
        Document d;
        d.setTitle("x");
        UndoStack stack;
        int redos = 0;
        int undos = 0;
        stack.pushExecuted(std::make_unique<Counted>(&redos, &undos));
        stack.undo(d);
        QCOMPARE(undos, 1);
        QCOMPARE(d.title(), std::string());
        QVERIFY(stack.canRedo());
        stack.redo(d);
        QCOMPARE(redos, 1);
        QCOMPARE(d.title(), std::string("x"));
    }

    void pushExecutedDropsTheRedoBranch()
    {
        Document d;
        UndoStack stack;
        stack.push(d, std::make_unique<Append>('a'));
        stack.push(d, std::make_unique<Append>('b'));
        stack.undo(d);
        int redos = 0;
        int undos = 0;
        d.setTitle(d.title() + 'x');
        stack.pushExecuted(std::make_unique<Counted>(&redos, &undos));
        QVERIFY(!stack.canRedo());
        QCOMPARE(stack.count(), std::size_t{2}); // « a » et la commande ; « b » est effacée
        QCOMPARE(redos, 0);
    }

    void pushExecutedMergesLikePush()
    {
        Document d;
        UndoStack stack;
        int redos = 0;
        int undos = 0;
        stack.pushExecuted(std::make_unique<Counted>(&redos, &undos, 7));
        stack.pushExecuted(std::make_unique<Counted>(&redos, &undos, 7));
        QCOMPARE(stack.count(), std::size_t{1});
        stack.pushExecuted(std::make_unique<Counted>(&redos, &undos, 8));
        QCOMPARE(stack.count(), std::size_t{2});
    }

    void pushExecutedHonoursTheLimit()
    {
        UndoStack stack(2);
        int redos = 0;
        int undos = 0;
        for (int i = 0; i < 5; ++i) {
            stack.pushExecuted(std::make_unique<Counted>(&redos, &undos));
        }
        QCOMPARE(stack.count(), std::size_t{2});
        QCOMPARE(stack.index(), std::size_t{2});
    }

    void pushExecutedKeepsTheCleanStateAccounting()
    {
        Document d;
        UndoStack stack(2);
        int redos = 0;
        int undos = 0;
        QVERIFY(stack.isClean());
        stack.pushExecuted(std::make_unique<Counted>(&redos, &undos));
        QVERIFY(!stack.isClean());
        stack.setClean();
        QVERIFY(stack.isClean());
        stack.pushExecuted(std::make_unique<Counted>(&redos, &undos));
        QVERIFY(!stack.isClean());
        stack.undo(d);
        QVERIFY(stack.isClean()); // revenu à l'état enregistré
        stack.pushExecuted(
            std::make_unique<Counted>(&redos, &undos)); // efface la branche : l'état propre reste
        QVERIFY(!stack.isClean());
        stack.pushExecuted(
            std::make_unique<Counted>(&redos, &undos)); // la borne chasse la commande enregistrée
        stack.undo(d);
        QVERIFY(!stack.isClean());
        stack.undo(d);
        QVERIFY(stack.isClean()); // l'indice enregistré a suivi le décalage de la borne
    }

    void pushExecutedLosesTheCleanStateOfADroppedBranch()
    {
        Document d;
        UndoStack stack;
        int redos = 0;
        int undos = 0;
        stack.pushExecuted(std::make_unique<Counted>(&redos, &undos));
        stack.setClean();
        stack.undo(d);
        stack.pushExecuted(std::make_unique<Counted>(&redos, &undos));
        stack.undo(d);
        QVERIFY(!stack.isClean());
        stack.redo(d);
        QVERIFY(!stack.isClean());
    }

    void pushExecutedIgnoresANullCommand()
    {
        UndoStack stack;
        stack.pushExecuted(nullptr);
        QCOMPARE(stack.count(), std::size_t{0});
        QVERIFY(!stack.canUndo());
        QVERIFY(stack.isClean());
    }
};

QTEST_GUILESS_MAIN(TstCoreUndo)
#include "tst_core_undo.moc"
