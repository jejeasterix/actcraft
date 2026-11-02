#include <actcraft/core/connectors.h>
#include <actcraft/core/document.h>
#include <actcraft/core/edits.h>
#include <actcraft/core/model.h>
#include <actcraft/core/shapegeometry.h>
#include <actcraft/tools/connectortool.h>
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
    void show(std::vector<core::PageObject> objects) override { shown = std::move(objects); }
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
    tools::ConnectorTool tool{context};
    core::ObjectId objectA = 0;
    core::ObjectId objectB = 0;

    Harness()
    {
        objectA = add({0, 0, 100, 100});
        objectB = add({300, 0, 100, 100});
        doc.executed = 0;
    }

    core::ObjectId add(const core::RectF& box)
    {
        auto command = std::make_unique<core::AddObjectCommand>(
            0, core::makeShapeObject("rectangle", box, core::ShapeStyle{}));
        core::AddObjectCommand* raw = command.get();
        doc.execute(std::move(command));
        return raw->id();
    }

    void send(double x, double y, Phase phase)
    {
        input::Sample s;
        s.x = x;
        s.y = y;
        s.phase = phase;
        s.device = input::DeviceKind::Mouse;
        tool.onSample(s);
    }

    // Glisse de (x1, y1) à (x2, y2) en trois temps.
    void drag(double x1, double y1, double x2, double y2)
    {
        send(x1, y1, Phase::Press);
        send((x1 + x2) / 2, (y1 + y2) / 2, Phase::Move);
        send(x2, y2, Phase::Move);
        send(x2, y2, Phase::Release);
    }

    const core::Connector* lastConnector() const
    {
        const auto& objects = doc.page().objects;
        return objects.empty() ? nullptr : std::get_if<core::Connector>(&objects.back());
    }
};

} // namespace

class TestToolsConnector : public QObject
{
    Q_OBJECT
private slots:
    void idAndIdle()
    {
        Harness h;
        QCOMPARE(h.tool.id(), QStringLiteral("connector"));
        QVERIFY(!h.tool.isBusy());
    }

    void hoverShowsAttachPoints()
    {
        Harness h;
        h.send(50, 50, Phase::Move); // dans A
        QCOMPARE(h.selection.last.smartPoints.size(), std::size_t{4});
        QCOMPARE(h.selection.last.smartPoints.front().x, 0.0);
        h.send(350, 50, Phase::Move); // dans B
        QCOMPARE(h.selection.last.smartPoints.size(), std::size_t{4});
        QCOMPARE(h.selection.last.smartPoints.front().x, 300.0);
        h.send(600, 600, Phase::Move); // le vide : rien
        QVERIFY(h.selection.last.smartPoints.empty());
        QCOMPARE(h.doc.executed, 0);
    }

    void dragFromObjectToObjectConnects()
    {
        Harness h;
        h.drag(101, 1, 299, 2);
        QCOMPARE(h.doc.executed, 1);
        const core::Connector* c = h.lastConnector();
        QVERIFY(c != nullptr);
        QCOMPARE(c->objectA, h.objectA);
        QCOMPARE(c->objectB, h.objectB);
        QCOMPARE(c->firstEnd.x, 100.0);
        QCOMPARE(c->firstEnd.y, 0.0);
        QCOMPARE(c->secondEnd.x, 300.0);
        QCOMPARE(c->secondEnd.y, 0.0);
        QVERIFY(!h.tool.isBusy());
    }

    void releaseOnEmptyLeavesAFreeEnd()
    {
        Harness h;
        h.drag(101, 1, 600, 300);
        const core::Connector* c = h.lastConnector();
        QVERIFY(c != nullptr);
        QCOMPARE(c->objectA, h.objectA);
        QCOMPARE(c->objectB, core::ObjectId{0});
        QCOMPARE(c->secondEnd.x, 600.0);
        QCOMPARE(c->secondEnd.y, 300.0);
    }

    void connectorLandsInTheUpperLayer()
    {
        Harness h;
        h.drag(101, 1, 299, 2);
        QCOMPARE(h.doc.page().objects.back().common.layer, core::Layer::Top);
    }

    void styleIsUsed()
    {
        Harness h;
        h.tool.setStyle(5);
        h.drag(101, 1, 299, 2);
        const core::Connector* c = h.lastConnector();
        QVERIFY(c != nullptr);
        QCOMPARE(c->style, std::uint8_t{5});
    }

    void previewDuringDragThenCleared()
    {
        Harness h;
        h.send(101, 1, Phase::Press);
        QVERIFY(h.tool.isBusy());
        h.send(200, 50, Phase::Move);
        QCOMPARE(h.lift.shown.size(), std::size_t{1});
        QVERIFY(std::get_if<core::Connector>(&h.lift.shown.front()) != nullptr);
        QCOMPARE(h.doc.executed, 0); // aucune commande pendant le geste
        h.send(299, 2, Phase::Release);
        QVERIFY(h.lift.shown.empty());
    }

    void clickWithoutDragCreatesNothing()
    {
        Harness h;
        h.send(101, 1, Phase::Press);
        h.send(101, 1, Phase::Release);
        QCOMPARE(h.doc.executed, 0);
        QVERIFY(!h.tool.isBusy());
    }

    void cancelAbandonsTheGesture()
    {
        Harness h;
        h.send(101, 1, Phase::Press);
        h.send(250, 50, Phase::Move);
        h.tool.cancel();
        QVERIFY(!h.tool.isBusy());
        QVERIFY(h.lift.shown.empty());
        QCOMPARE(h.doc.executed, 0);
    }
};

QTEST_MAIN(TestToolsConnector)
#include "tst_tools_connector.moc"
