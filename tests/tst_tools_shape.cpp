#include <actcraft/core/document.h>
#include <actcraft/core/edits.h>
#include <actcraft/core/model.h>
#include <actcraft/tools/shapetool.h>
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
    void show(std::vector<core::PageObject> objects) override { shown = std::move(objects); }
    void setTransform(const core::Transform&) override {}
    void clear() override { shown.clear(); }
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
};

input::Sample at(double x, double y, Phase phase)
{
    input::Sample s;
    s.x = x;
    s.y = y;
    s.phase = phase;
    s.device = input::DeviceKind::Mouse;
    return s;
}

tools::ShapeSettings settings(const char* preset, bool filled, bool fromCenter)
{
    tools::ShapeSettings s;
    s.preset = QString::fromLatin1(preset);
    s.filled = filled;
    s.fill = 0xFFFFFFFFu;
    s.fromCenter = fromCenter;
    return s;
}

} // namespace

class TstToolsShape : public QObject
{
    Q_OBJECT

private slots:
    void dragPreviewsWithoutExecuting()
    {
        Harness h;
        h.user.setShapeSettings(settings("rectangle", true, false));
        tools::ShapeTool tool(h.context);
        tool.onSample(at(100, 100, Phase::Press));
        tool.onSample(at(200, 160, Phase::Move));
        QCOMPARE(h.lift.shown.size(), std::size_t{1});
        QCOMPARE(h.doc.executed, 0);
        QVERIFY(tool.isBusy());
    }
    void releaseExecutesOneAdd()
    {
        Harness h;
        h.user.setShapeSettings(settings("rectangle", true, false));
        tools::ShapeTool tool(h.context);
        tool.onSample(at(100, 100, Phase::Press));
        tool.onSample(at(200, 160, Phase::Move));
        tool.onSample(at(200, 160, Phase::Release));
        QCOMPARE(h.doc.executed, 1);
        QVERIFY(h.lift.shown.empty());
        QVERIFY(!tool.isBusy());
        const auto& o = h.doc.document.page(0).objects.back();
        QCOMPARE(o.common.layer, core::Layer::Middle);
        QCOMPARE(QString::fromStdString(std::get<core::Shape>(core::body(o)).preset),
                 QStringLiteral("rectangle"));
    }
    void singleClickAddsNothing()
    {
        Harness h;
        tools::ShapeTool tool(h.context);
        tool.onSample(at(100, 100, Phase::Press));
        tool.onSample(at(101, 102, Phase::Release));
        QCOMPARE(h.doc.executed, 0);
        QVERIFY(h.lift.shown.empty());
        QVERIFY(!tool.isBusy());
    }
    void fromCenterSetting()
    {
        Harness h;
        h.user.setShapeSettings(settings("rectangle", true, true));
        tools::ShapeTool tool(h.context);
        tool.onSample(at(100, 100, Phase::Press));
        tool.onSample(at(150, 130, Phase::Release));
        const auto& o = h.doc.document.page(0).objects.back();
        QCOMPARE(o.common.transform.dx, 50.0);
        QCOMPARE(o.common.transform.dy, 70.0);
    }
    // Amendement 5 (D5 contredit avec témoin, L4-O2) : Maj, tenue avant l'appui ou enfoncée au milieu du
    // glisser, ne change ni l'aperçu ni la forme posée (ni carré, ni 45°) ; le relais de Ctrl et d'avant
    // appui non plus.
    void shiftLeavesTheShapeUnchanged()
    {
        const auto previewPrint = [](const std::vector<core::PageObject>& shown) {
            core::Page page;
            page.objects = shown;
            return core::fingerprint(page);
        };
        for (const char* preset : {"rectangle", "line"}) {
            std::uint64_t shownWithout = 0; // témoin sans Maj
            std::uint64_t placedWithout = 0;
            // 0 : sans Maj ; 1 : Maj tenue avant l'appui ; 2 : Maj enfoncée au milieu du glisser.
            for (int variant = 0; variant < 3; ++variant) {
                Harness h;
                h.user.setShapeSettings(settings(preset, true, false));
                tools::ShapeTool tool(h.context);
                if (variant == 1) {
                    QVERIFY(!tool.onKey(Qt::Key_Shift, static_cast<int>(Qt::ShiftModifier)));
                    tool.onKey(Qt::Key_unknown, static_cast<int>(Qt::ShiftModifier)); // relais d'avant appui
                }
                tool.onSample(at(0, 0, Phase::Press));
                tool.onSample(at(50, 35, Phase::Move));
                if (variant == 2) {
                    QVERIFY(!tool.onKey(Qt::Key_Shift, static_cast<int>(Qt::ShiftModifier)));
                    tool.onKey(Qt::Key_Control, static_cast<int>(Qt::ShiftModifier | Qt::ControlModifier));
                }
                tool.onSample(at(100, 70, Phase::Move)); // ligne à 35°, rectangle de 100 × 70
                QCOMPARE(h.lift.shown.size(), std::size_t{1});
                const std::uint64_t shown = previewPrint(h.lift.shown);
                tool.onSample(at(100, 70, Phase::Release));
                QCOMPARE(h.doc.executed, 1);
                const std::uint64_t placed = core::fingerprint(h.doc.document.page(0));
                if (variant == 0) {
                    shownWithout = shown;
                    placedWithout = placed;
                    continue;
                }
                QCOMPARE(shown, shownWithout);
                QCOMPARE(placed, placedWithout);
                if (QString::fromLatin1(preset) == QStringLiteral("rectangle")) {
                    const auto& s = std::get<core::Shape>(core::body(h.doc.document.page(0).objects.back()));
                    QCOMPARE(s.bounds.width, 100.0); // pas un carré
                    QCOMPARE(s.bounds.height, 70.0);
                }
            }
        }
    }
    void snapsPressAndRelease()
    {
        Harness h;
        h.user.setShapeSettings(settings("rectangle", true, false));
        auto& grid = h.doc.document.page(0).grid;
        grid.snapEnabled = true;
        core::GridLevel level;
        level.spacingX = level.spacingY = 50;
        grid.levels.push_back(level);
        tools::ShapeTool tool(h.context);
        tool.onSample(at(48, 52, Phase::Press));
        tool.onSample(at(147, 103, Phase::Release));
        const auto& o = h.doc.document.page(0).objects.back();
        QCOMPARE(o.common.transform.dx, 50.0);
        QCOMPARE(std::get<core::Shape>(core::body(o)).bounds.width, 100.0);
    }
    void cancelClearsThePreview()
    {
        Harness h;
        tools::ShapeTool tool(h.context);
        tool.onSample(at(0, 0, Phase::Press));
        tool.onSample(at(100, 100, Phase::Move));
        QVERIFY(!h.lift.shown.empty());
        tool.cancel();
        QVERIFY(h.lift.shown.empty());
        QVERIFY(!tool.isBusy());
        QCOMPARE(h.doc.executed, 0);
    }
    void unfilledChoiceHonoured()
    {
        Harness h;
        h.user.setShapeSettings(settings("rectangle", false, false));
        tools::ShapeTool tool(h.context);
        tool.onSample(at(0, 0, Phase::Press));
        tool.onSample(at(100, 100, Phase::Release));
        QCOMPARE(std::get<core::Shape>(core::body(h.doc.document.page(0).objects.back())).fill >> 24, 0u);
    }
    void chainPresetsExecuteNothingOnRelease() // amendement 4 (D1) : leur geste est celui de L4-T31
    {
        for (const char* preset : {"polyline", "curve-chain"}) {
            Harness h;
            h.user.setShapeSettings(settings(preset, true, false));
            tools::ShapeTool tool(h.context);
            tool.onSample(at(100, 100, Phase::Press));
            tool.onSample(at(200, 160, Phase::Move));
            QVERIFY(h.lift.shown.empty());
            tool.onSample(at(200, 160, Phase::Release));
            QCOMPARE(h.doc.executed, 0);
        }
    }
};

QTEST_GUILESS_MAIN(TstToolsShape)
#include "tst_tools_shape.moc"
