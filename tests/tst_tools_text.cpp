#include <actcraft/core/model.h>
#include <actcraft/tools/texttool.h>
#include <actcraft/tools/userstate.h>

#include <QTest>

#include <memory>
#include <utility>
#include <vector>

using namespace actcraft;

namespace {

class FakeEditor final : public tools::DocumentEditor
{
public:
    core::Page current;
    const core::Page& page() const override { return current; }
    std::size_t pageIndex() const override { return 0; }
    void execute(std::unique_ptr<core::UndoCommand>) override {}
    std::vector<core::PageObject> lift(const std::vector<core::ObjectId>&) override { return {}; }
    void drop(std::unique_ptr<core::UndoCommand>) override {}
    bool isLifting() const override { return false; }
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
    void show(const tools::SelectionView&) override {}
};

class FakeView final : public tools::ViewInfo
{
public:
    double zoom() const override { return 1.0; }
    bool overTrash(core::PointF) const override { return false; }
    void setTrashHover(bool) override {}
    QStringList objectBarCommands() const override { return {}; }
};

// Garde chaque commande déclenchée avec son argument.
class FakeCommands final : public tools::CommandSink
{
public:
    std::vector<std::pair<QString, QVariant>> calls;
    std::pair<QString, QVariant> last;
    bool trigger(const QString& id, const QVariant& argument) override
    {
        last = {id, argument};
        calls.emplace_back(id, argument);
        return true;
    }
};

struct Harness
{
    FakeEditor editor;
    FakeInk ink;
    FakeStrokes strokes;
    FakeLift lift;
    FakeSelection selection;
    FakeView view;
    FakeCommands commands;
    tools::UserState user;
    tools::ToolContext context{editor, ink, strokes, lift, selection, view, commands, user};
};

input::Sample at(double x, double y, input::Phase phase)
{
    input::Sample s;
    s.x = x;
    s.y = y;
    s.phase = phase;
    s.device = input::DeviceKind::Mouse;
    return s;
}

} // namespace

class TstToolsText : public QObject
{
    Q_OBJECT

private slots:
    // P17 : un appui demande l'édition au point touché ; l'argument est un texte « x;y;v » (amendement 1).
    void pressAsksToEditAt()
    {
        Harness h;
        tools::TextTool tool(h.context, false);
        tool.onSample(at(120, 80, input::Phase::Press));
        QCOMPARE(h.commands.last.first, QStringLiteral("text.edit-at"));
        QCOMPARE(h.commands.last.second.toString(), QStringLiteral("120.00;80.00;0"));
        QCOMPARE(tool.id(), QStringLiteral("text"));
    }

    // P26 / D9 : l'outil de haut en bas le dit dans l'argument et dans son identifiant.
    void verticalToolSaysVertical()
    {
        Harness h;
        tools::TextTool tool(h.context, true);
        tool.onSample(at(10, 10, input::Phase::Press));
        QVERIFY(h.commands.last.second.toString().endsWith(QStringLiteral(";1")));
        QCOMPARE(tool.id(), QStringLiteral("text-vertical"));
    }

    // F-252 : quitter l'outil valide l'édition en cours (argument absent).
    void deactivateCommits()
    {
        Harness h;
        tools::TextTool tool(h.context, false);
        tool.deactivate();
        QCOMPARE(h.commands.last.first, QStringLiteral("text.edit-at"));
        QVERIFY(!h.commands.last.second.isValid());
    }

    // L'édition vit dans l'interface : l'outil n'est jamais occupé.
    void neverBusy()
    {
        Harness h;
        tools::TextTool tool(h.context, false);
        QVERIFY(!tool.isBusy());
        tool.onSample(at(10, 10, input::Phase::Press));
        QVERIFY(!tool.isBusy());
    }

    // Seul l'appui compte : déplacement et relâchement ne déclenchent rien.
    void moveAndReleaseAreIgnored()
    {
        Harness h;
        tools::TextTool tool(h.context, false);
        tool.onSample(at(10, 10, input::Phase::Move));
        tool.onSample(at(10, 10, input::Phase::Release));
        QVERIFY(h.commands.calls.empty());
    }

    // cancel() et abandon() ne déclenchent rien : Échap, changement de page et Annuler valident l'édition
    // côté interface (amendement 5 : ToolHost::cancelGesture n'appelle cancel() que si l'outil est occupé, ce
    // que l'outil Texte n'est jamais).
    void cancelSendsNothing()
    {
        Harness h;
        tools::TextTool tool(h.context, true);
        tool.cancel();
        tool.abandon();
        QVERIFY(h.commands.calls.empty());
    }
};

QTEST_GUILESS_MAIN(TstToolsText)
#include "tst_tools_text.moc"
