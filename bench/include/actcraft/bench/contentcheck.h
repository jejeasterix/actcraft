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
// Point de scène (pixels logiques) → pixel de capture qui le contient (× rapport de pixels, partie entière).
QPoint sceneToImage(QPointF scenePoint, qreal devicePixelRatio);
ContentCheck checkColor(const QString& name, const QImage& image, QPoint pixel, QColor expected,
                        int tolerance);
// L10-T5 : pixels de la ligne [from, to] (tracé de Bresenham, extrémités comprises) dont la couleur n'est ni
// à `margin` près du fond ni à `margin` près de l'encre, par canal : bords lissés. -1 si l'image est vide ou
// si un point sort de l'image.
int countIntermediatePixels(const QImage& image, QPoint from, QPoint to, QColor background, QColor ink,
                            int margin);
// Pixels de la même ligne à `margin` près de l'encre : contrôle positif (le segment croise bien un trait ou
// une ligne de grille). -1 dans les mêmes cas.
int countInkPixels(const QImage& image, QPoint from, QPoint to, QColor ink, int margin);
// mi-trait, fin-trait, zoom-800, retour-100
QStringList captureSteps();
QString captureFileName(const QString& graphicsApi, const QString& step);

} // namespace actcraft::bench
