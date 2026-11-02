#include <actcraft/core/alignment.h>

namespace actcraft::core {

// Talon L5-T1 : rempli par L5-T6.
TransformEachCommand::TransformEachCommand(std::size_t page, TransformList transforms)
    : m_page(page), m_transforms(std::move(transforms))
{}

// Talon L5-T1 : rempli par L5-T6.
void TransformEachCommand::redo(Document& document, Changes& changes)
{
    static_cast<void>(document);
    static_cast<void>(changes);
    static_cast<void>(m_page);
    static_cast<void>(m_before);
}

// Talon L5-T1 : rempli par L5-T6.
void TransformEachCommand::undo(Document& document, Changes& changes)
{
    static_cast<void>(document);
    static_cast<void>(changes);
}

// Talon L5-T1 : rempli par L5-T6.
TransformList alignTransforms(const Page& page, const std::vector<ObjectId>& ids, AlignMode mode)
{
    static_cast<void>(page);
    static_cast<void>(ids);
    static_cast<void>(mode);
    return {};
}

// Talon L5-T1 : rempli par L5-T6.
TransformList fitTransforms(const Page& page, const std::vector<ObjectId>& ids, FitMode mode)
{
    static_cast<void>(page);
    static_cast<void>(ids);
    static_cast<void>(mode);
    return {};
}

// Talon L5-T1 : rempli par L5-T6.
TransformList flipTransforms(const Page& page, const std::vector<ObjectId>& ids, Axis axis)
{
    static_cast<void>(page);
    static_cast<void>(ids);
    static_cast<void>(axis);
    return {};
}

// Talon L5-T1 : rempli par L5-T6.
TransformList originFlipTransforms(const Page& page, const std::vector<ObjectId>& ids, Axis axis,
                                   PointF origin)
{
    static_cast<void>(page);
    static_cast<void>(ids);
    static_cast<void>(axis);
    static_cast<void>(origin);
    return {};
}

// Talon L5-T1 : rempli par L5-T6.
std::vector<PageObject> mirrorCopies(const Page& page, const std::vector<ObjectId>& ids, Axis axis)
{
    static_cast<void>(page);
    static_cast<void>(ids);
    static_cast<void>(axis);
    return {};
}

} // namespace actcraft::core
