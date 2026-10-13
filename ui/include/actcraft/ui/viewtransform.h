#pragma once

#include <QPoint>
#include <QPointF>
#include <QSize>
#include <QSizeF>
#include <Qt>

namespace actcraft::ui {

inline constexpr double kMinZoom = 0.25;
inline constexpr double kMaxZoom = 8.0;
inline constexpr double kZoomStep = 1.15;             // par cran de molette (décision 15)
inline constexpr double kViewMargin = 32.0;           // marge de la page bornée dans la vue (décision 16)
inline constexpr double kScrollPixelsPerNotch = 60.0; // défilement par cran
inline constexpr int kCacheRefreshDelayMs = 150;      // N2-4
inline constexpr int kMaxCacheSide = 4096;            // N1-6

// Vue → page : page = (vue − pan) / zoom.
struct ViewTransform
{
    double zoom = 1.0;
    QPointF pan; // position de l'origine de la page dans la vue

    QPointF pageToView(QPointF p) const { return pan + p * zoom; }
    QPointF viewToPage(QPointF v) const { return (v - pan) / zoom; }
};

double clampZoom(double zoom) noexcept;
// 1,15^(delta/120) : tient compte des molettes fines (N2-11).
double wheelZoomFactor(int angleDeltaY) noexcept;
// Le point `anchor` de la vue reste sur le même point de page.
ViewTransform zoomAt(const ViewTransform& t, QPointF anchor, double newZoom);
// Page plus petite que la vue (marges comprises) : centrée sur cet axe ; sinon bornée à la marge.
ViewTransform clampPan(const ViewTransform& t, QSizeF view, QSizeF page, double margin = kViewMargin);
// « Ajuster » : page entière visible, centrée.
ViewTransform fitPage(QSizeF view, QSizeF page, double margin = kViewMargin);
// Déplacement de la page (pixels de vue) pour une molette sans Ctrl (décision 16).
QPointF wheelScrollDelta(QPoint angleDelta, QPoint pixelDelta, Qt::KeyboardModifiers modifiers);
// Taille de la texture du cache : page × zoom × rapport de pixels, plus grand côté plafonné (décision 7).
QSize cacheTextureSize(QSizeF page, double zoom, double devicePixelRatio, int maxSide = kMaxCacheSide);

} // namespace actcraft::ui
