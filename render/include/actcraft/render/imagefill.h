#pragma once
#include <actcraft/core/document.h>
#include <actcraft/core/model.h>

#include <QImage>
#include <QPoint>

namespace actcraft::render {

// Zone 4-connexe de `seed` à moins de `tolerance` (0 à 100 % de l'écart maximal par canal) ; alpha gardé.
QImage floodFill(const QImage& image, QPoint seed, core::Argb color, int tolerance);

} // namespace actcraft::render
