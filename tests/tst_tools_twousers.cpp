#include <actcraft/core/document.h>
#include <actcraft/core/edits.h>
#include <actcraft/core/lift.h>
#include <actcraft/input/routing.h>
#include <actcraft/tools/grabs.h>
#include <actcraft/tools/selecttool.h>
#include <actcraft/tools/userstate.h>

#include <QTest>

#include <algorithm>
#include <memory>
#include <optional>
#include <string>
#include <vector>

using namespace actcraft;
using input::Phase;

namespace {

using Ids = std::vector<core::ObjectId>;

// Document réel partagé par les deux utilisateurs, avec soulèvement comme le contrôleur de ui/ (même forme
// que tst_tools_select) : les objets soulevés quittent la page affichée, `drop` les remet puis exécute la
// commande. Les objets soulevés n'étant plus sur la page, l'objet tenu se prouve aussi pendant l'appui
// posé, avant tout soulèvement : là, seul le registre des objets tenus le protège.
class SharedEditor final : public tools::DocumentEditor
{
public:
    core::Document document;
    std::vector<std::string> calls; // « execute », « lift », « drop:null », « drop:command »

    const core::Page& page() const override { return document.page(0); }
    std::size_t pageIndex() const override { return 0; }
    void execute(std::unique_ptr<core::UndoCommand> command) override
    {
        calls.emplace_back("execute");
        core::Changes changes;
        command->redo(document, changes);
    }
    std::vector<core::PageObject> lift(const std::vector<core::ObjectId>& ids) override
    {
        calls.emplace_back("lift");
        std::vector<core::LiftedObject> taken = core::takeObjects(document.page(0), ids);
        std::vector<core::PageObject> copies;
        for (const core::LiftedObject& item : taken) {
            copies.push_back(item.object);
            m_lifted.push_back(item);
        }
        return copies;
    }
    void drop(std::unique_ptr<core::UndoCommand> command) override
    {
        calls.emplace_back(command ? "drop:command" : "drop:null");
        core::restoreObjects(document.page(0), m_lifted);
        m_lifted.clear();
        if (command) {
            core::Changes changes;
            command->redo(document, changes);
        }
    }
    bool isLifting() const override { return !m_lifted.empty(); }

    const core::PageObject* find(core::ObjectId id) const
    {
        const core::Page& p = document.page(0);
        const std::size_t index = core::indexOf(p, id);
        return index == p.objects.size() ? nullptr : &p.objects[index];
    }

private:
    std::vector<core::LiftedObject> m_lifted;
};

class FakeLift final : public tools::LiftSurface
{
public:
    void show(std::vector<core::PageObject>) override {}
    void setTransform(const core::Transform&) override {}
    void clear() override {}
};

class FakeSelection final : public tools::SelectionSurface
{
public:
    std::vector<tools::SelectionView> views;
    void show(const tools::SelectionView& view) override { views.push_back(view); }
};

class FakeView final : public tools::ViewInfo
{
public:
    QStringList bar; // boutons de la barre d'objet ; aucun par défaut
    double zoom() const override { return 1.0; }
    bool overTrash(core::PointF p) const override { return p.x > 1000; }
    void setTrashHover(bool) override {}
    QStringList objectBarCommands() const override { return bar; }
};

class FakeInk final : public tools::InkSurface
{
public:
    void beginStroke(core::Argb, double) override {}
    void addPoint(const core::StrokePoint&, std::uint64_t) override {}
    void clear() override {}
};

class FakeStrokes final : public tools::DocumentSink
{
public:
    void commitStroke(core::Stroke) override {}
};

class FakeCommands final : public tools::CommandSink
{
public:
    QStringList triggered;
    bool trigger(const QString& id, const QVariant&) override
    {
        triggered.push_back(id);
        return true;
    }
};

core::PageObject rectangle(core::ObjectId id, double x, double y, double w, double h)
{
    core::Shape shape;
    shape.bounds = {x, y, w, h};
    shape.outlineWidth = 0;
    core::PageObject object{shape};
    object.common.id = id;
    return object;
}

input::Sample sampleAt(Phase phase, double x, double y)
{
    input::Sample s;
    s.phase = phase;
    s.x = x;
    s.y = y;
    s.device = input::DeviceKind::Mouse;
    return s;
}

Ids sorted(Ids ids)
{
    std::sort(ids.begin(), ids.end());
    return ids;
}

bool contains(const Ids& ids, core::ObjectId id)
{
    return std::find(ids.begin(), ids.end(), id) != ids.end();
}

// Un utilisateur : son état, son sélecteur, ses surfaces ; le document et la vue sont communs.
struct Seat
{
    Seat(SharedEditor& editor, FakeView& view, input::UserId id)
        : user(id), context{editor, ink, strokes, lift, selection, view, commands, user}, tool(context)
    {
        context.snapDisabled = true; // positions exactes : l'accrochage n'est pas l'objet de ce test
    }
    FakeInk ink;
    FakeStrokes strokes;
    FakeLift lift;
    FakeSelection selection;
    FakeCommands commands;
    tools::UserState user;
    tools::ToolContext context;
    tools::SelectTool tool;

    void press(double x, double y) { tool.onSample(sampleAt(Phase::Press, x, y)); }
    void move(double x, double y) { tool.onSample(sampleAt(Phase::Move, x, y)); }
    void release(double x, double y) { tool.onSample(sampleAt(Phase::Release, x, y)); }
    void click(double x, double y)
    {
        press(x, y);
        release(x, y);
    }
    // Glisser de (x0, y0) à (x1, y1) en `steps` mouvements, relâché à l'arrivée.
    void drag(double x0, double y0, double x1, double y1, int steps)
    {
        press(x0, y0);
        for (int i = 1; i <= steps; ++i) {
            move(x0 + (x1 - x0) * i / steps, y0 + (y1 - y0) * i / steps);
        }
        release(x1, y1);
    }
};

// Une page, deux utilisateurs : 1 = (100, 100, 100 × 100), 3 = (400, 400, 50 × 50). `shared` : les deux
// sélecteurs partagent un registre des objets tenus (comme ui::ToolHost en mode à deux utilisateurs) ; sinon
// aucun registre (un seul utilisateur, comportement d'avant L11-T8).
struct Board
{
    explicit Board(bool shared = true)
    {
        core::Page& page = editor.document.page(0);
        page.objects.push_back(rectangle(1, 100, 100, 100, 100));
        page.objects.push_back(rectangle(3, 400, 400, 50, 50));
        if (shared) {
            first.tool.setGrabs(&grabs);
            second.tool.setGrabs(&grabs);
        }
    }
    SharedEditor editor;
    FakeView view;
    tools::Grabs grabs;
    Seat first{editor, view, input::kFirstUser};
    Seat second{editor, view, input::kSecondUser};

    core::RectF bounds(core::ObjectId id) const
    {
        const core::PageObject* object = editor.find(id);
        return object ? core::boundsOnPage(editor.document.page(0), *object) : core::RectF{}; // transformée
    }
};

const std::optional<input::UserId> kNobody;

} // namespace

class TstToolsTwousers : public QObject
{
    Q_OBJECT
private slots:
    // F-303 : chaque utilisateur sa sélection ; le sélecteur de l'un ne voit ni ne change celle de l'autre.
    void selectionsAreSeparate()
    {
        Board b;
        b.first.click(150, 150);
        b.second.click(420, 420);
        QVERIFY(b.first.user.selection() == Ids{1});
        QVERIFY(b.second.user.selection() == Ids{3});

        b.second.tool.selectAll();
        QVERIFY(b.first.user.selection() == Ids{1});
        QVERIFY(sorted(b.second.user.selection()) == (Ids{1, 3}));

        b.first.tool.clearSelection();
        QVERIFY(b.first.user.selection().empty());
        QVERIFY(sorted(b.second.user.selection()) == (Ids{1, 3}));

        // Rectangle élastique de l'utilisateur 1 : la sélection de l'utilisateur 0 n'en sait rien.
        b.first.click(420, 420);
        b.second.drag(50, 50, 260, 260, 4);
        QVERIFY(b.second.user.selection() == Ids{1});
        QVERIFY(b.first.user.selection() == Ids{3});
        // Chaque sélecteur publie son propre cadre.
        QVERIFY(!b.first.selection.views.empty());
        QVERIFY(!b.second.selection.views.empty());
    }

    // H11-13 : un objet tenu par un utilisateur est inaccessible à l'autre jusqu'au relâchement.
    void heldObjectRefusedToTheOther()
    {
        {
            Board b;
            // L'utilisateur 0 appuie sur le rectangle 3 et le tient : rien n'est encore soulevé, l'objet est
            // toujours sur la page.
            b.first.press(420, 420);
            QVERIFY(b.grabs.holder(3) == std::optional<input::UserId>(input::kFirstUser));
            // L'utilisateur 1 clique dessus : aucune sélection.
            b.second.click(420, 420);
            QVERIFY(b.second.user.selection().empty());
            // Il le glisse : l'objet ne bouge pas.
            b.second.drag(425, 425, 525, 525, 4);
            QVERIFY(!contains(b.second.user.selection(), 3));
            QCOMPARE(b.bounds(3).x, 400.0);
            QCOMPARE(b.bounds(3).y, 400.0);
            // Son rectangle élastique l'entoure : rien de sélectionné.
            b.second.drag(380, 380, 480, 480, 4);
            QVERIFY(b.second.user.selection().empty());
            // Tout sélectionner : tout sauf l'objet tenu.
            b.second.tool.selectAll();
            QVERIFY(b.second.user.selection() == Ids{1});
            // L'utilisateur 0 n'a rien perdu : il glisse l'objet tenu (+40 en x), le tient toujours pendant
            // le glisser, puis le relâche.
            b.first.move(430, 420);
            b.first.move(460, 420);
            QVERIFY(b.grabs.holder(3) == std::optional<input::UserId>(input::kFirstUser));
            b.first.release(460, 420);
            QCOMPARE(b.bounds(3).x, 440.0);
            QCOMPARE(b.bounds(3).y, 400.0);
        }
        {
            Board b;
            // L'utilisateur 1 avait déjà sélectionné le rectangle 3 ; l'utilisateur 0 le saisit ensuite.
            b.second.click(420, 420);
            QVERIFY(b.second.user.selection() == Ids{3});
            b.first.press(420, 420);
            QVERIFY(b.grabs.holder(3) == std::optional<input::UserId>(input::kFirstUser));
            // L'utilisateur 1 glisse depuis l'intérieur de son ancien cadre : l'objet ne bouge pas.
            b.second.drag(430, 430, 530, 530, 4);
            QCOMPARE(b.bounds(3).x, 400.0);
            QCOMPARE(b.bounds(3).y, 400.0);
            QVERIFY(!contains(b.second.user.selection(), 3));
            // Les commandes de sa sélection n'atteignent pas non plus l'objet tenu.
            b.second.user.setSelection({3});
            b.second.tool.deleteSelection();
            QVERIFY(b.editor.find(3) != nullptr);
            b.first.release(420, 420);
        }
        {
            Board b;
            b.first.press(420, 420); // tient 3
            // L'arbre des objets pose la sélection puis rafraîchit (ToolHost::selectObjects) : l'objet tenu
            // en sort.
            b.second.user.setSelection({1, 3});
            b.second.tool.refresh();
            QVERIFY(b.second.user.selection() == Ids{1});
            // Agrandir, réduire, translucidité, Suppr : l'objet tenu sort de la sélection, le cadre est
            // republié sans lui, rien n'est exécuté.
            const auto pruned = [&b](const auto& command) {
                b.second.user.setSelection({3});
                const std::size_t before = b.second.selection.views.size();
                command();
                return b.second.user.selection().empty() && b.second.selection.views.size() > before &&
                       !b.second.selection.views.back().visible;
            };
            QVERIFY(pruned([&b] { b.second.tool.enlarge(); }));
            QVERIFY(pruned([&b] { b.second.tool.reduce(); }));
            QVERIFY(pruned([&b] { b.second.tool.setTranslucency(100); }));
            bool handled = true;
            QVERIFY(pruned([&b, &handled] { handled = b.second.tool.onKey(Qt::Key_Delete, 0); }));
            QVERIFY(!handled); // rien à supprimer
            QVERIFY(std::find(b.editor.calls.begin(), b.editor.calls.end(), "execute") ==
                    b.editor.calls.end());
            QCOMPARE(b.bounds(3).x, 400.0);
            b.first.release(420, 420);
        }
        {
            Board b;
            b.view.bar = {QStringLiteral("object.enlarge")};
            b.first.click(420, 420); // l'utilisateur 0 sélectionne 3
            const tools::SelectionView frame = b.first.selection.views.back();
            QVERIFY(frame.visible);
            QVERIFY(!frame.handles.empty());
            QVERIFY(!frame.bar.empty());
            // Poignée saisie : la sélection est tenue.
            b.first.press(frame.handles[0].x, frame.handles[0].y);
            QVERIFY(b.grabs.holder(3) == std::optional<input::UserId>(input::kFirstUser));
            b.second.click(420, 420);
            QVERIFY(b.second.user.selection().empty());
            b.first.tool.cancel();
            QVERIFY(b.grabs.holder(3) == kNobody);
            // Bouton de la barre appuyé : tenue jusqu'au relâché, la commande part ensuite.
            b.first.press(frame.bar[0].center.x, frame.bar[0].center.y);
            QVERIFY(b.grabs.holder(3) == std::optional<input::UserId>(input::kFirstUser));
            b.second.click(420, 420);
            QVERIFY(b.second.user.selection().empty());
            b.first.release(frame.bar[0].center.x, frame.bar[0].center.y);
            QVERIFY(b.grabs.holder(3) == kNobody);
            QCOMPARE(b.first.commands.triggered, QStringList{QStringLiteral("object.enlarge")});
        }
    }

    void releasedObjectIsFreeAgain()
    {
        Board b;
        // Tenu puis relâché : libre.
        b.first.drag(420, 420, 460, 420, 4);
        QVERIFY(b.grabs.holder(3) == kNobody);
        QCOMPARE(b.bounds(3).x, 440.0);
        b.second.click(450, 420);
        QVERIFY(b.second.user.selection() == Ids{3});
        b.second.drag(450, 420, 450, 470, 4);
        QCOMPARE(b.bounds(3).y, 450.0);
        QVERIFY(b.grabs.holder(3) == kNobody);

        // Geste abandonné : relâché aussi.
        b.first.press(150, 150);
        QVERIFY(b.grabs.holder(1) == std::optional<input::UserId>(input::kFirstUser));
        b.first.tool.cancel();
        QVERIFY(b.grabs.holder(1) == kNobody);
        b.second.click(150, 150);
        QVERIFY(b.second.user.selection() == Ids{1});

        // Le registre : un utilisateur ne relâche que ce qu'il tient ; le même utilisateur peut reprendre.
        tools::Grabs grabs;
        QVERIFY(grabs.hold(1, input::kFirstUser));
        QVERIFY(grabs.hold(1, input::kFirstUser));
        QVERIFY(grabs.hold(3, input::kSecondUser));
        QVERIFY(!grabs.hold(1, input::kSecondUser));
        QVERIFY(!grabs.hold(3, input::kFirstUser));
        QVERIFY(grabs.holder(1) == std::optional<input::UserId>(input::kFirstUser));
        grabs.release(input::kFirstUser);
        QVERIFY(grabs.holder(1) == kNobody);
        QVERIFY(grabs.holder(3) == std::optional<input::UserId>(input::kSecondUser));
        QVERIFY(grabs.hold(1, input::kSecondUser));

        // Changer de registre relâche ce que l'utilisateur tenait dans l'ancien ; détruire l'outil aussi.
        tools::Grabs other; // déclaré avant le montage : il survit aux outils
        Board c;
        c.first.press(420, 420);
        QVERIFY(c.grabs.holder(3) == std::optional<input::UserId>(input::kFirstUser));
        c.first.tool.setGrabs(&other);
        QVERIFY(c.grabs.holder(3) == kNobody);
        c.first.tool.cancel();
        auto seat = std::make_unique<Seat>(c.editor, c.view, input::kSecondUser);
        seat->tool.setGrabs(&c.grabs);
        seat->press(150, 150);
        QVERIFY(c.grabs.holder(1) == std::optional<input::UserId>(input::kSecondUser));
        seat.reset();
        QVERIFY(c.grabs.holder(1) == kNobody);
    }

    // Sautées : les outils de maths sont tenus par un seul état (MathToolsController n'a pas d'outils par
    // utilisateur), hypothèse H11-O13, raccord à trancher par Jérôme. Corps prêt pour la règle :
    // rulerBelongsToItsUserBody, dont l'ouverture passera alors par la commande des outils de maths.
    void rulerBelongsToItsUser()
    {
        QSKIP("outils de maths tenus par un seul état, pas d'outils par utilisateur (H11-O13, raccord à "
              "trancher)");
    }
    // Corps à écrire au raccord (H11-O13) : l'utilisateur 0 ouvre la règle par la commande des outils de
    // maths, l'utilisateur 1 trace au stylo le long de son bord ; le trait enregistré de l'utilisateur 1
    // garde ses points (non redressé), alors que le même trait de l'utilisateur 0 est redressé contre la
    // règle.
    void inkOfTheOtherNotGuided()
    {
        QSKIP("outils de maths tenus par un seul état, pas d'outils par utilisateur (H11-O13, raccord à "
              "trancher)");
    }

    // Un seul utilisateur : un registre posé ne change rien (mêmes appels au document, même sélection,
    // mêmes positions) ; ce qu'il tient ne le bloque pas lui-même, et tout est relâché à la fin.
    void singleUserUnchanged()
    {
        Board with(true);
        Board without(false);
        for (Board* b : {&with, &without}) {
            b->first.drag(150, 150, 190, 150, 4);
            b->first.click(420, 420);
            b->first.drag(425, 425, 425, 465, 4);
            b->first.tool.selectAll();
            b->first.drag(160, 160, 160, 200, 4);
            b->first.drag(50, 50, 260, 300, 4);
            b->first.tool.deleteSelection();
        }
        QVERIFY(with.editor.calls == without.editor.calls);
        QVERIFY(with.first.user.selection() == without.first.user.selection());
        QCOMPARE(with.first.selection.views.size(), without.first.selection.views.size());
        QCOMPARE(with.bounds(3).x, without.bounds(3).x);
        QCOMPARE(with.bounds(3).y, without.bounds(3).y);
        QCOMPARE(with.bounds(3).y, 480.0);
        QVERIFY(with.editor.find(1) == nullptr);
        QVERIFY(with.grabs.holder(1) == kNobody);
        QVERIFY(with.grabs.holder(3) == kNobody);
    }

private:
    // Règle ouverte par l'utilisateur 0 : dans son état, pas dans celui de l'utilisateur 1.
    void rulerBelongsToItsUserBody()
    {
        Board b;
        // Au raccord (H11-O13) : ouvrir la règle par sa commande (math.ruler, A11) pour l'utilisateur 0.
        b.first.user.setOpenMathTools({QStringLiteral("math.ruler")});
        const std::vector<QString>& mine = b.first.user.openMathTools();
        const std::vector<QString>& theirs = b.second.user.openMathTools();
        QVERIFY(std::find(mine.begin(), mine.end(), QStringLiteral("math.ruler")) != mine.end());
        QVERIFY(std::find(theirs.begin(), theirs.end(), QStringLiteral("math.ruler")) == theirs.end());
    }
};

QTEST_GUILESS_MAIN(TstToolsTwousers)
#include "tst_tools_twousers.moc"
