#include <actcraft/core/fill.h>

namespace actcraft::core {

// Talon L5-T1 : rempli par L5-T8.
FillHit fillHitAt(const Page& page, PointF point, double tolerance)
{
    static_cast<void>(page);
    static_cast<void>(point);
    static_cast<void>(tolerance);
    return {};
}

// Talon L5-T1 : rempli par L5-T8.
std::unique_ptr<UndoCommand> makeFillCommand(const Page& page, std::size_t pageIndex, const FillHit& hit,
                                             Argb color)
{
    static_cast<void>(page);
    static_cast<void>(pageIndex);
    static_cast<void>(hit);
    static_cast<void>(color);
    return nullptr;
}

// Talon L5-T1 : rempli par L5-T8.
std::unique_ptr<UndoCommand> makeImageFillCommand(std::size_t pageIndex, ObjectId image, std::string pngBytes)
{
    static_cast<void>(pageIndex);
    static_cast<void>(image);
    static_cast<void>(pngBytes);
    return nullptr;
}

} // namespace actcraft::core
