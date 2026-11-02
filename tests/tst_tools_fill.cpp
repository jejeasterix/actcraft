#include <actcraft/core/document.h>
#include <actcraft/core/edits.h>
#include <actcraft/core/fill.h>
#include <actcraft/core/model.h>
#include <actcraft/core/shapegeometry.h>
#include <actcraft/tools/filltool.h>
#include <actcraft/tools/userstate.h>

#include <QBuffer>
#include <QImage>
#include <QTest>

#include <atomic>
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
    QByteArray resourceData(const std::string& key) const override
    {
        const core::Resource* resource = document.resource(key);
        return resource ? resource->data : QByteArray();
    }
    std::uint64_t revision() const override { return static_cast<std::uint64_t>(executed); }
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
    tools::FillTool tool{context};

    Harness() { tool.setColor(0xFF00AA00u); }

    core::ObjectId addShape(const core::RectF& box, core::Argb fill)
    {
        core::ShapeStyle style;
        style.fill = fill;
        doc.execute(
            std::make_unique<core::AddObjectCommand>(0, core::makeShapeObject("rectangle", box, style)));
        return doc.page().objects.back().common.id;
    }

    // Image de ressource : 20 x 20 blanche, posée sur (0, 0, 100, 100).
    core::ObjectId addImage()
    {
        QImage image(20, 20, QImage::Format_ARGB32);
        image.fill(Qt::white);
        QByteArray png;
        QBuffer buffer(&png);
        buffer.open(QIODevice::WriteOnly);
        image.save(&buffer, "PNG");
        core::ImageObject object;
        object.bounds = {0, 0, 100, 100};
        object.resource = doc.document.addResource(png, "png");
        doc.execute(std::make_unique<core::AddObjectCommand>(0, core::PageObject{object}));
        return doc.page().objects.back().common.id;
    }

    void click(double x, double y)
    {
        input::Sample s;
        s.x = x;
        s.y = y;
        s.device = input::DeviceKind::Mouse;
        s.phase = Phase::Press;
        tool.onSample(s);
        s.phase = Phase::Release;
        tool.onSample(s);
    }

    const core::Shape& shape(core::ObjectId id) const
    {
        return std::get<core::Shape>(doc.page().objects[core::indexOf(doc.page(), id)]);
    }
    QImage imageOf(core::ObjectId id) const
    {
        const auto& object = std::get<core::ImageObject>(doc.page().objects[core::indexOf(doc.page(), id)]);
        return QImage::fromData(doc.resourceData(object.resource));
    }
};

} // namespace

class TestToolsFill : public QObject
{
    Q_OBJECT
private slots:
    void clickFillsTopmostClosedShape()
    {
        Harness h;
        const core::ObjectId below = h.addShape({0, 0, 200, 200}, 0xFFFF0000u);
        const core::ObjectId above = h.addShape({100, 100, 200, 200}, 0xFF0000FFu);
        h.doc.executed = 0;
        h.click(150, 150);
        QCOMPARE(h.doc.executed, 1);
        QCOMPARE(h.shape(above).fill, 0xFF00AA00u);
        QCOMPARE(h.shape(below).fill, 0xFFFF0000u);
    }

    void clickOnEmptyFillsBackground()
    {
        Harness h;
        h.addShape({0, 0, 50, 50}, 0xFFFF0000u);
        h.doc.executed = 0;
        h.click(900, 700);
        QCOMPARE(h.doc.executed, 1);
        QCOMPARE(h.doc.page().background.fill.kind, core::FillKind::Solid);
        QCOMPARE(h.doc.page().background.fill.color, 0xFF00AA00u);
    }

    void lockedShapeIsFilled()
    {
        Harness h;
        const core::ObjectId id = h.addShape({0, 0, 200, 200}, 0xFFFF0000u);
        h.doc.document.page(0).objects[core::indexOf(h.doc.page(), id)].common.locked = true;
        h.doc.executed = 0;
        h.click(100, 100);
        QCOMPARE(h.doc.executed, 1);
        QCOMPARE(h.shape(id).fill, 0xFF00AA00u);
    }

    void clickOnImageFloodFills()
    {
        Harness h;
        const core::ObjectId image = h.addImage();
        h.doc.executed = 0;
        h.click(50, 50);
        QTRY_COMPARE(h.doc.executed, 1);
        const QImage result = h.imageOf(image);
        QCOMPARE(result.size(), QSize(20, 20));
        QCOMPARE(result.pixel(10, 10), qRgba(0, 0xAA, 0, 255));
    }

    void imageFillOffMainThread()
    {
        Harness h;
        h.addImage();
        h.doc.executed = 0;
        std::atomic<QThread*> worker{nullptr};
        h.tool.setComputeObserver([&](QThread* thread) { worker = thread; });
        h.click(50, 50);
        QCOMPARE(h.doc.executed, 0); // la commande n'est pas poussée pendant le clic
        QTRY_COMPARE(h.doc.executed, 1);
        QVERIFY(worker.load() != nullptr);
        QVERIFY(worker.load() != QThread::currentThread());
    }

    void staleImageFillDropped()
    {
        Harness h;
        h.addImage();
        std::atomic<bool> computed{false};
        h.tool.setComputeObserver([&](QThread*) { computed = true; });
        h.doc.executed = 0;
        h.click(50, 50);
        h.addShape({300, 300, 50, 50}, 0xFFFF0000u); // le document change avant le retour du calcul
        QCOMPARE(h.doc.executed, 1);
        QTRY_VERIFY(computed.load());
        QTRY_COMPARE(h.tool.staleImageFillDropped(), 1);
        QCOMPARE(h.doc.executed, 1); // aucune commande de remplissage
    }

    void clickOnNothingDoesNothingOutsideBackground()
    {
        Harness h;
        h.doc.executed = 0;
        input::Sample s;
        s.x = 10;
        s.y = 10;
        s.phase = Phase::Move;
        h.tool.onSample(s);
        QCOMPARE(h.doc.executed, 0);
        QVERIFY(!h.tool.isBusy());
    }
};

QTEST_MAIN(TestToolsFill)
#include "tst_tools_fill.moc"
