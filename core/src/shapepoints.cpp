#include <actcraft/core/shapepoints.h>

namespace actcraft::core {

// Talon L5-T1 : rempli par L5-T9.
std::vector<PointF> shapePoints(const PageObject& object)
{
    static_cast<void>(object);
    return {};
}

// Talon L5-T1 : rempli par L5-T9.
MoveShapePointCommand::MoveShapePointCommand(std::size_t page, ObjectId shape, std::size_t index, PointF to)
    : m_page(page), m_shape(shape), m_index(index), m_to(to)
{}

// Talon L5-T1 : rempli par L5-T9.
void MoveShapePointCommand::redo(Document& document, Changes& changes)
{
    static_cast<void>(document);
    static_cast<void>(changes);
    static_cast<void>(m_page);
    static_cast<void>(m_shape);
    static_cast<void>(m_index);
    static_cast<void>(m_to);
    static_cast<void>(m_before);
}

// Talon L5-T1 : rempli par L5-T9.
void MoveShapePointCommand::undo(Document& document, Changes& changes)
{
    static_cast<void>(document);
    static_cast<void>(changes);
}

} // namespace actcraft::core
