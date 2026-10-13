#pragma once

#include <actcraft/bench/result.h>

#include <QColor>
#include <QImage>
#include <QPoint>
#include <QPointF>
#include <QStringList>

namespace actcraft::bench {

inline constexpr int kWitnessTolerance = 24; // par canal
inline constexpr int kInkTolerance = 48;

bool colorNear(QColor actual, QColor expected, int tolerance) noexcept;
// Moyenne 3 × 3 autour du pixel ; voisins hors de l'image ignorés ; invalide si le pixel est hors de l'image.
QColor sampleColor(const QImage& image, QPoint pixel);
// Point de scène (pixels logiques) → pixel de capture (× rapport de pixels, arrondi).
QPoint sceneToImage(QPointF scenePoint, qreal devicePixelRatio);
ContentCheck checkColor(const QString& name, const QImage& image, QPoint pixel, QColor expected,
                        int tolerance);
// mi-trait, fin-trait, zoom-800, retour-100
QStringList captureSteps();
QString captureFileName(const QString& graphicsApi, const QString& step);

} // namespace actcraft::bench
