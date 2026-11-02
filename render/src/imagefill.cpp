#include <actcraft/render/imagefill.h>

namespace actcraft::render {

// Talon L5-T1 : rempli par L5-T8.
QImage floodFill(const QImage& image, QPoint seed, core::Argb color, int tolerance)
{
    static_cast<void>(seed);
    static_cast<void>(color);
    static_cast<void>(tolerance);
    return image;
}

} // namespace actcraft::render
