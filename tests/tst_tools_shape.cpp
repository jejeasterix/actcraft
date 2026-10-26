#include <actcraft/core/document.h>
#include <actcraft/core/edits.h>
#include <actcraft/core/model.h>
#include <actcraft/core/shapegeometry.h>
#include <actcraft/tools/shapetool.h>
#include <actcraft/tools/userstate.h>

#include <QTest>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
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

input::Sample atTime(double x, double y, Phase phase, std::int64_t ms)
{
    input::Sample s = at(x, y, phase);
    s.timestampNs = ms * 1'000'000;
    return s;
}

void click(tools::ShapeTool& tool, double x, double y, std::int64_t ms)
{
    tool.onSample(atTime(x, y, Phase::Press, ms));
    tool.onSample(atTime(x, y, Phase::Release, ms + 50));
}

std::vector<core::PointF> pagePoints(const core::PageObject& o) // translation seule (convention de L4-T3)
{
    std::vector<core::PointF> out;
    for (const core::PointF& p : std::get<core::Line>(core::body(o)).points)
        out.push_back({p.x + o.common.transform.dx, p.y + o.common.transform.dy});
    return out;
}

double distanceToSegment(core::PointF p, core::PointF a, core::PointF b)
{
    const double dx = b.x - a.x;
    const double dy = b.y - a.y;
    const double length2 = dx * dx + dy * dy;
    const double t =
        length2 > 0.0 ? std::clamp(((p.x - a.x) * dx + (p.y - a.y) * dy) / length2, 0.0, 1.0) : 0.0;
    return std::hypot(p.x - (a.x + t * dx), p.y - (a.y + t * dy));
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
        QCOMPARE(o.common.transform.dy, 50.0); // Q4a2-m9 : l'axe Y aussi (52 -> 50, 103 -> 100)
        QCOMPARE(std::get<core::Shape>(core::body(o)).bounds.height, 50.0);
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
    void
    chainPresetsExecuteNothingOnRelease() // amendement 4 (D1) : un glissé pose des sommets, ne verse rien
    {
        for (const char* preset : {"polyline", "curve-chain"}) {
            Harness h;
            h.user.setShapeSettings(settings(preset, true, false));
            tools::ShapeTool tool(h.context);
            tool.onSample(at(100, 100, Phase::Press));
            tool.onSample(at(200, 160, Phase::Move));
            QVERIFY(!h.lift.shown.empty()); // L4-T31 : le glissé bouton tenu montre le segment en cours
            tool.onSample(at(200, 160, Phase::Release));
            QCOMPARE(h.doc.executed, 0);
        }
    }
    void polylineByClicksEndsOnDoubleClick() // L4-T31
    {
        Harness h;
        h.user.setShapeSettings(settings("polyline", true, false));
        tools::ShapeTool tool(h.context);
        click(tool, 100, 100, 0);
        click(tool, 200, 100, 1000);
        QCOMPARE(h.lift.shown.size(), std::size_t{1}); // aperçu refait au sommet posé (pas de survol, H4-19)
        QCOMPARE(std::get<core::Line>(core::body(h.lift.shown.front())).points.size(), std::size_t{2});
        QCOMPARE(h.doc.executed, 0);
        QVERIFY(tool.isBusy());
        click(tool, 200, 200, 2000);
        QCOMPARE(std::get<core::Line>(core::body(h.lift.shown.front())).points.size(), std::size_t{3});
        click(tool, 201, 201, 2200); // double-clic : à moins de 6 px et de 500 ms, ne pose pas de sommet
        QCOMPARE(h.doc.executed, 1);
        QVERIFY(h.lift.shown.empty());
        QVERIFY(!tool.isBusy());
        const std::vector<core::PointF> points = pagePoints(h.doc.document.page(0).objects.back());
        QCOMPARE(points.size(), std::size_t{3});
        QCOMPARE(points[0].x, 100.0);
        QCOMPARE(points[1].x, 200.0);
        QCOMPARE(points[2].y, 200.0);
    }
    void curveChainIsSmoothed() // L4-T31
    {
        Harness h;
        h.user.setShapeSettings(settings("curve-chain", true, false));
        tools::ShapeTool tool(h.context);
        click(tool, 0, 0, 0);
        click(tool, 100, 0, 1000);
        click(tool, 100, 100, 2000);
        click(tool, 200, 100, 3000);
        click(tool, 200, 100, 3100); // double-clic
        QCOMPARE(h.doc.executed, 1);
        const std::vector<core::PointF> points = pagePoints(h.doc.document.page(0).objects.back());
        QCOMPARE(points.size(), std::size_t{3 * core::kCurveChainSamples + 1});
        QCOMPARE(points.front().x, 0.0);
        QCOMPARE(points.back().x, 200.0);
        QCOMPARE(points.back().y, 100.0);
        // M-M (L4-C7) : lissée, la chaîne quitte la ligne brisée de ses sommets. Sans lissage, chaque point
        // tomberait sur l'un des trois segments (écart nul).
        const std::vector<core::PointF> vertices{{0.0, 0.0}, {100.0, 0.0}, {100.0, 100.0}, {200.0, 100.0}};
        double farthest = 0.0;
        for (const core::PointF& p : points) {
            double nearest = std::numeric_limits<double>::max();
            for (std::size_t i = 0; i + 1 < vertices.size(); ++i) {
                nearest = std::min(nearest, distanceToSegment(p, vertices[i], vertices[i + 1]));
            }
            farthest = std::max(farthest, nearest);
        }
        QVERIFY2(farthest > 2.0, qPrintable(QString::number(farthest)));
    }
    void freeformFollowsTheDrag() // L4-T31
    {
        Harness h;
        h.user.setShapeSettings(settings("freeform", true, false));
        tools::ShapeTool tool(h.context);
        tool.onSample(at(0, 0, Phase::Press));
        for (int i = 1; i <= 10; ++i)
            tool.onSample(at(10.0 * i, (i % 2) * 5.0, Phase::Move));
        QCOMPARE(h.lift.shown.size(), std::size_t{1});
        QCOMPARE(h.doc.executed, 0);
        tool.onSample(at(100, 0, Phase::Release));
        QCOMPARE(h.doc.executed, 1);
        QVERIFY(h.lift.shown.empty());
        QVERIFY(pagePoints(h.doc.document.page(0).objects.back()).size() >= std::size_t{11});
        tool.onSample(at(300, 300, Phase::Press)); // sous le seuil : rien
        tool.onSample(at(301, 301, Phase::Release));
        QCOMPARE(h.doc.executed, 1);
    }
    void chainCancelKeepsTwoVerticesOrMore() // L4-T31
    {
        Harness h;
        h.user.setShapeSettings(settings("polyline", true, false));
        tools::ShapeTool tool(h.context);
        click(tool, 100, 100, 0);
        tool.cancel(); // un seul sommet : rien n'est versé
        QCOMPARE(h.doc.executed, 0);
        QVERIFY(!tool.isBusy());
        click(tool, 100, 100, 1000);
        click(tool, 200, 100, 2000);
        tool.cancel(); // changement d'outil ou de page : la ligne commencée est versée, comme le trait du
                       // stylo
        QCOMPARE(h.doc.executed, 1);
        QVERIFY(h.lift.shown.empty());
        QVERIFY(!tool.isBusy());
    }
    void escapeVersesTheChainInOneStep() // L4-T31 : Échap -> tool.select -> cancelGesture() -> cancel()
    {
        Harness h;
        h.user.setShapeSettings(settings("polyline", true, false));
        tools::ShapeTool tool(h.context);
        click(tool, 100, 100, 0);
        click(tool, 200, 100, 1000);
        click(tool, 200, 200, 2000);
        // Le chemin Échap -> tool.select -> cancelGesture() relève de ToolHost (lot 3, ui/src/toolhost.cpp)
        // et n'est pas rejoué ici ; ce test vérifie ce qu'il déclenche : cancel() verse la ligne en une
        // étape.
        QVERIFY(tool.isBusy()); // Q4a2-m10 : la précondition est vérifiée, plus masquée par un `if`
        tool.cancel();
        QCOMPARE(h.doc.executed, 1); // une seule AddObjectCommand : une étape d'annulation
        QCOMPARE(pagePoints(h.doc.document.page(0).objects.back()).size(), std::size_t{3});
        QVERIFY(h.lift.shown.empty());
        QVERIFY(!tool.isBusy());
    }
    void chainAbandonAddsNothing() // L4-T31
    {
        Harness h;
        h.user.setShapeSettings(settings("polyline", true, false));
        tools::ShapeTool tool(h.context);
        click(tool, 100, 100, 0);
        click(tool, 200, 100, 1000);
        tool.abandon();
        QCOMPARE(h.doc.executed, 0);
        QVERIFY(h.lift.shown.empty());
        QVERIFY(!tool.isBusy());
    }
};

QTEST_GUILESS_MAIN(TstToolsShape)
#include "tst_tools_shape.moc"
