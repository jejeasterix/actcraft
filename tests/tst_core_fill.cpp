#include <actcraft/core/document.h>
#include <actcraft/core/edits.h>
#include <actcraft/core/fill.h>
#include <actcraft/core/shapegeometry.h>
#include <actcraft/core/undo.h>
#include <actcraft/render/imagefill.h>

#include <QImage>
#include <QtTest>

#include <variant>

using namespace actcraft;

namespace {

core::Document blank()
{
    return core::Document(1024, 768);
}

core::ObjectId addRect(core::Document& d, core::RectF box)
{
    core::PageObject object = core::makeShapeObject("rectangle", box, core::ShapeStyle{});
    object.common.id = d.allocateId();
    core::insertAtTopOfLayer(d.page(0), std::move(object));
    return d.page(0).objects.back().common.id;
}

core::ObjectId addStroke(core::Document& d, std::vector<core::PointF> points)
{
    core::Stroke stroke;
    for (const auto& p : points)
        stroke.points.push_back({p.x, p.y, 0.5f});
    core::PageObject object{std::move(stroke)};
    object.common.id = d.allocateId();
    core::insertAtTopOfLayer(d.page(0), std::move(object));
    return d.page(0).objects.back().common.id;
}

core::PageObject& objectById(core::Document& d, core::ObjectId id)
{
    core::Page& page = d.page(0);
    return page.objects[core::indexOf(page, id)];
}

core::Argb fillColorOf(const core::PageObject& object)
{
    return std::get<core::Shape>(object).fill;
}

core::Argb backgroundColor(const core::Page& page)
{
    return page.background.fill.color;
}

} // namespace

class TestFill : public QObject
{
    Q_OBJECT
private slots:
    void insideClosedShapeFills()
    {
        auto d = blank();
        const auto a = addRect(d, {100, 100, 200, 100});
        const auto hit = core::fillHitAt(d.page(0), {200, 150}, 3.0);
        QCOMPARE(hit.target, core::FillTarget::ShapeInside);
        QCOMPARE(hit.object, a);
    }
    void onOutlineRecolorsPen()
    {
        auto d = blank();
        addRect(d, {100, 100, 200, 100});
        QCOMPARE(core::fillHitAt(d.page(0), {100, 150}, 3.0).target, core::FillTarget::Outline);
        const auto s = addStroke(d, {{400, 400}, {500, 400}});
        const auto hit = core::fillHitAt(d.page(0), {450, 401}, 3.0);
        QCOMPARE(hit.object, s);
        QCOMPARE(hit.target, core::FillTarget::Outline);
        core::UndoStack stack;
        stack.push(d, core::makeFillCommand(d.page(0), 0, hit, 0xFF112233u));
        QCOMPARE(std::get<core::Stroke>(objectById(d, s)).color, core::Argb(0xFF112233u));
    }
    void emptyAreaFillsBackground()
    {
        auto d = blank();
        core::UndoStack stack;
        const auto hit = core::fillHitAt(d.page(0), {10, 10}, 3.0);
        QCOMPARE(hit.target, core::FillTarget::PageBackground);
        stack.push(d, core::makeFillCommand(d.page(0), 0, hit, 0xFF336699u));
        QCOMPARE(backgroundColor(d.page(0)), core::Argb(0xFF336699u));
    }
    void lockedObjectsAreFilled()
    {
        auto d = blank();
        const auto a = addRect(d, {100, 100, 200, 100});
        objectById(d, a).common.locked = true;
        QCOMPARE(core::fillHitAt(d.page(0), {200, 150}, 3.0).object, a);
    }
    void imageFloodFillTolerance5()
    {
        QImage image(10, 10, QImage::Format_ARGB32);
        image.fill(QColor(200, 200, 200));
        image.setPixelColor(9, 9, QColor(200, 200, 230)); // écart de 30 sur 255 ≈ 12 % : hors tolérance 5
        const QImage out = render::floodFill(image, {0, 0}, 0xFFFF0000u, core::kDefaultImageFillTolerance);
        QCOMPARE(out.pixelColor(5, 5), QColor(255, 0, 0));
        QCOMPARE(out.pixelColor(9, 9), QColor(200, 200, 230));
    }
    void fillIsOneStep()
    {
        auto d = blank();
        const auto a = addRect(d, {100, 100, 200, 100});
        core::UndoStack stack;
        stack.push(d, core::makeFillCommand(d.page(0), 0, core::fillHitAt(d.page(0), {200, 150}, 3.0),
                                            0xFF00FF00u));
        QCOMPARE(fillColorOf(objectById(d, a)), core::Argb(0xFF00FF00u));
        QCOMPARE(stack.count(), std::size_t(1));
        stack.undo(d);
        QVERIFY(fillColorOf(objectById(d, a)) != core::Argb(0xFF00FF00u));
    }
};

QTEST_GUILESS_MAIN(TestFill)
#include "tst_core_fill.moc"
