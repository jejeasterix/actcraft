#include <actcraft/core/edits.h>
#include <actcraft/core/hittest.h>
#include <actcraft/core/shapeedits.h>
#include <actcraft/core/shapegeometry.h>

#include <utility>

namespace actcraft::core {

namespace {

// Forme à préréglage de l'objet `id` sur la page, ou nullptr (objet absent, autre type, forme lue).
Shape* presetShape(Document& document, std::size_t pageIndex, ObjectId id)
{
    Page& page = document.page(pageIndex);
    const std::size_t rank = indexOf(page, id);
    if (rank == page.objects.size())
        return nullptr;
    auto* shape = std::get_if<Shape>(&body(page.objects[rank]));
    if (shape == nullptr || findShapePreset(shape->preset) == nullptr)
        return nullptr;
    return shape;
}

// Pose `parameters`, reconstruit la géométrie, rend l'ObjectChanged (zone = emprise avant ∪ après).
Change apply(Document& document, std::size_t pageIndex, ObjectId id, const std::vector<double>& parameters)
{
    Page& page = document.page(pageIndex);
    PageObject& object = page.objects[indexOf(page, id)];
    const RectF before = boundsOnPage(object);
    auto& shape = std::get<Shape>(body(object));
    shape.parameters = parameters;
    rebuildShape(shape);
    return Change{Change::Kind::ObjectChanged, pageIndex, id, unitedRect(before, boundsOnPage(object))};
}

} // namespace

SetShapeParametersCommand::SetShapeParametersCommand(std::size_t page, ObjectId id, std::vector<double> parameters)
    : m_page(page), m_id(id), m_after(std::move(parameters))
{}

void SetShapeParametersCommand::redo(Document& document, Changes& changes)
{
    m_applied = false;
    const Shape* shape = presetShape(document, m_page, m_id);
    if (shape == nullptr)
        return;
    m_before = shape->parameters;
    m_applied = true;
    changes.push_back(apply(document, m_page, m_id, m_after));
}

void SetShapeParametersCommand::undo(Document& document, Changes& changes)
{
    if (!m_applied || presetShape(document, m_page, m_id) == nullptr)
        return;
    changes.push_back(apply(document, m_page, m_id, m_before));
}

} // namespace actcraft::core
