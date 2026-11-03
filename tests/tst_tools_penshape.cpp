#include <actcraft/core/document.h>
#include <actcraft/core/edits.h>
#include <actcraft/core/model.h>
#include <actcraft/core/penmodifiers.h>
#include <actcraft/core/shapegeometry.h>
#include <actcraft/tools/penshapetool.h>
#include <actcraft/tools/userstate.h>

#include <QTest>

#include <cstdint>
#include <memory>
#include <utility>
#include <variant>
#include <vector>

using namespace actcraft;
using input::Phase;

namespace {

class FakeDocument final : public tools::DocumentEditor
{
public:
    core::Document document;
    int executed = 0;
    const core::Page& page() const override { return document.page(0); }
    std::size_t pageIndex() const override { return 0; }
    void execute(std::unique_ptr<core::UndoCommand> command) override
    {
        core::Changes changes;
        command->redo(document, changes);
        ++executed;
    }
    std::vector<core::PageObject> lift(const std::vector<core::ObjectId>&) override { return {}; }
    void drop(std::unique_ptr<core::UndoCommand>) override {}
    bool isLifting() const override { return false; }
};

class FakeLift final : public tools::LiftSurface
{
public:
    std::vector<core::PageObject> shown;
    int clears = 0;
    int shows = 0;
    void show(std::vector<core::PageObject> objects) override
    {
        shown = std::move(objects);
        ++shows;
    }
    void setTransform(const core::Transform&) override {}
    void clear() override
    {
        shown.clear();
        ++clears;
    }
};

class FakeSelection final : public tools::SelectionSurface
{
public:
    tools::SelectionView last;
    void show(const tools::SelectionView& view) override { last = view; }
};

class FakeView final : public tools::ViewInfo
{
public:
    double zoom() const override { return 1.0; }
    bool overTrash(core::PointF) const override { return false; }
    void setTrashHover(bool) override {}
    QStringList objectBarCommands() const override { return {}; }
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
    bool trigger(const QString&, const QVariant&) override { return true; }
};

struct Harness
{
    FakeDocument doc;
    FakeInk ink;
    FakeStrokes strokes;
    FakeLift lift;
    FakeSelection selection;
    FakeView view;
    FakeCommands commands;
    tools::UserState user;
    tools::ToolContext context{doc, ink, strokes, lift, selection, view, commands, user};
    tools::PenShapeTool tool{context};

    Harness()
    {
        core::Outline pen;
        pen.color = 0xFF112233u;
        pen.width = 6;
        tool.setPen(pen, false);
    }

    void send(double x, double y, Phase phase, std::int64_t ns = 0)
    {
        input::Sample s;
        s.x = x;
        s.y = y;
        s.phase = phase;
        s.timestampNs = ns;
        s.device = input::DeviceKind::Mouse;
        tool.onSample(s);
    }

    void click(double x, double y, std::int64_t ns)
    {
        send(x, y, Phase::Press, ns);
        send(x, y, Phase::Release, ns);
    }

    const core::Stroke* lastStroke() const
    {
        const auto& objects = doc.page().objects;
        return objects.empty() ? nullptr : std::get_if<core::Stroke>(&objects.back());
    }
};

} // namespace

class TestToolsPenShape : public QObject
{
    Q_OBJECT
private slots:
    void dragDrawsAPreviewThenOneStroke()
    {
        Harness h;
        h.tool.setShape(core::PenShape::Rectangle);
        h.send(10, 10, Phase::Press);
        h.send(50, 40, Phase::Move);
        h.send(100, 80, Phase::Move);
        QCOMPARE(h.doc.executed, 0);
        QVERIFY(h.tool.isBusy());
        h.send(100, 80, Phase::Release);
        QCOMPARE(h.doc.executed, 1);
        QVERIFY(!h.tool.isBusy());
        const core::Stroke* stroke = h.lastStroke();
        QVERIFY(stroke != nullptr);
        const auto expected = core::penShapePoints(core::PenShape::Rectangle, {10, 10}, {100, 80});
        QCOMPARE(stroke->points.size(), expected.size());
        QCOMPARE(stroke->points[2].x, 100.0);
        QCOMPARE(stroke->points[2].y, 80.0);
        QCOMPARE(stroke->color, 0xFF112233u);
        QCOMPARE(stroke->baseWidth, 6.0);
        QCOMPARE(stroke->kind, core::StrokeKind::Pen);
        QCOMPARE(h.doc.page().objects.back().common.layer, core::Layer::Top);
    }

    void previewUsesTheLiftSurface()
    {
        Harness h;
        h.tool.setShape(core::PenShape::Ellipse);
        h.send(10, 10, Phase::Press);
        QCOMPARE(h.lift.shows, 0);
        h.send(60, 40, Phase::Move);
        h.send(90, 70, Phase::Move);
        QCOMPARE(h.lift.shows, 2);
        QVERIFY(!h.lift.shown.empty());
        QCOMPARE(h.doc.executed, 0);
        QCOMPARE(h.lift.clears, 0);
        h.send(90, 70, Phase::Release);
        QVERIFY(h.lift.clears >= 1);
        QVERIFY(h.lift.shown.empty());
        QCOMPARE(h.doc.executed, 1);
    }

    void clickWithoutDragDrawsNothing()
    {
        Harness h;
        h.tool.setShape(core::PenShape::Rectangle);
        h.click(20, 20, 0);
        QCOMPARE(h.doc.executed, 0);
        QVERIFY(h.doc.page().objects.empty());
    }

    void highlighterKindIsKept()
    {
        Harness h;
        core::Outline pen;
        pen.color = 0x80FFFF00u;
        pen.width = 20;
        h.tool.setPen(pen, true);
        h.tool.setShape(core::PenShape::Horizontal);
        h.send(10, 10, Phase::Press);
        h.send(200, 40, Phase::Move);
        h.send(200, 40, Phase::Release);
        const core::Stroke* stroke = h.lastStroke();
        QVERIFY(stroke != nullptr);
        QCOMPARE(stroke->kind, core::StrokeKind::Highlighter);
        QCOMPARE(stroke->points.back().y, 10.0); // horizontale : y de départ gardé
    }

    void polylineEndsOnDoubleClick()
    {
        Harness h;
        h.tool.setShape(core::PenShape::Polyline);
        const std::int64_t second = 1'000'000'000;
        h.click(10, 10, 0);
        h.send(60, 10, Phase::Move);
        h.click(100, 10, second);
        h.click(100, 100, 2 * second);
        QCOMPARE(h.doc.executed, 0);
        QVERIFY(h.tool.isBusy());
        QVERIFY(h.lift.shows >= 1);
        h.click(100, 100, 2 * second + 100'000'000); // double clic : termine
        QCOMPARE(h.doc.executed, 1);
        QVERIFY(!h.tool.isBusy());
        const core::Stroke* stroke = h.lastStroke();
        QVERIFY(stroke != nullptr);
        QCOMPARE(stroke->points.size(), std::size_t(3));
        QCOMPARE(stroke->points[1].x, 100.0);
        QCOMPARE(stroke->points[2].y, 100.0);
    }

    // Choisir une couleur ou une épaisseur rappelle setShape avec le même tracé : la polyligne en cours reste.
    void sameShapeKeepsThePolylineInProgress()
    {
        Harness h;
        h.tool.setShape(core::PenShape::Polyline);
        h.click(10, 10, 0);
        h.click(100, 10, 1'000'000'000);
        QVERIFY(h.tool.isBusy());
        const int clears = h.lift.clears;
        h.tool.setShape(core::PenShape::Polyline); // même tracé
        QVERIFY(h.tool.isBusy());
        QCOMPARE(h.lift.clears, clears);
        h.click(100, 100, 2'000'000'000);
        h.click(100, 100, 2'100'000'000); // double clic : termine
        QCOMPARE(h.doc.executed, 1);
        const core::Stroke* stroke = h.lastStroke();
        QVERIFY(stroke != nullptr);
        QCOMPARE(stroke->points.size(), std::size_t(3));
    }

    // Un autre tracé abandonne bien celui en cours.
    void otherShapeResetsTheTool()
    {
        Harness h;
        h.tool.setShape(core::PenShape::Polyline);
        h.click(10, 10, 0);
        h.click(100, 10, 1'000'000'000);
        QVERIFY(h.tool.isBusy());
        h.tool.setShape(core::PenShape::Circle);
        QVERIFY(!h.tool.isBusy());
        QCOMPARE(h.doc.executed, 0);
    }

    void escapeAbandonsThePolyline()
    {
        Harness h;
        h.tool.setShape(core::PenShape::Polyline);
        h.click(10, 10, 0);
        h.click(100, 10, 1'000'000'000);
        QVERIFY(h.tool.onKey(Qt::Key_Escape, 0));
        QVERIFY(!h.tool.isBusy());
        QCOMPARE(h.doc.executed, 0);
        QVERIFY(h.lift.shown.empty());
    }

    void cancelLeavesNothing()
    {
        Harness h;
        h.tool.setShape(core::PenShape::Circle);
        h.send(10, 10, Phase::Press);
        h.send(80, 80, Phase::Move);
        h.tool.cancel();
        QCOMPARE(h.doc.executed, 0);
        QVERIFY(h.lift.shown.empty());
        QVERIFY(!h.tool.isBusy());
    }
};

QTEST_MAIN(TestToolsPenShape)
#include "tst_tools_penshape.moc"
