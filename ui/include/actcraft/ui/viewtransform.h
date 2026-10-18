#pragma once

#include <QPoint>
#include <QPointF>
#include <QSize>
#include <QSizeF>
#include <Qt>
#include <QtGlobal>

namespace actcraft::ui {

inline constexpr double kMinZoom = 0.25;
inline constexpr double kMaxZoom = 8.0;
inline constexpr double kZoomStep = 1.15;             // par cran de molette (décision 15)
inline constexpr double kViewMargin = 32.0;           // marge de la page bornée dans la vue (décision 16)
inline constexpr double kScrollPixelsPerNotch = 60.0; // défilement par cran
inline constexpr int kCacheRefreshDelayMs = 150;      // N2-4
// Lot 10 (DA11) : un objet ajouté en fin de page attend au-dessus du cache ; le cache le prend à son prochain
// redessin, au plus tard après kCacheFoldIdleMs sans entrée, ou dès kCacheFoldMaxPending objets en attente
// (H10-2 : valeurs de départ, réglées par le banc).
inline constexpr int kCacheFoldIdleMs = 500;
inline constexpr int kCacheFoldMaxPending = 128;
inline constexpr int kMaxCacheSide = 4096; // N1-6
// H10-9 (L10-T8) : texture du cache multi-échantillonnée plafonnée plus bas. Sous MSAA 4×, une couche garde
// la couleur et la profondeur-stencil multi-échantillonnées plus la texture résolue : (2 × 4 + 1) × 4 = 36
// octets par pixel. Échantillonnage du cache en RHI = kMsaaSamples (app/renderbackend.h).
inline constexpr int kCacheMsaaSamples = 4;
inline constexpr int kMaxMsaaCacheSide = 3072;
static_assert((2 * kCacheMsaaSamples + 1) * 4 * qint64(kMaxMsaaCacheSide) * (kMaxMsaaCacheSide * 3 / 4) <=
                  qint64(256) * 1024 * 1024,
              "texture multi-échantillonnée d'une page 4:3 au plafond : 256 Mio au plus");

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
// Plafond du côté de la texture du cache (H10-9) : kMaxCacheSide pour un cache sans multi-échantillonnage ;
// sinon kMaxMsaaCacheSide, jamais sous le côté de la page à max(100 %, `sharpZoom`) : ni flou à 100 % ni à
// l'ajustement (`sharpZoom` = zoom d'« Ajuster »).
int cacheMaxSide(QSizeF page, double devicePixelRatio, int cacheSamples, double sharpZoom = 1.0);

// Barre de défilement d'un axe (F-022) : taille et position en fraction de l'étendue (page zoomée + 2
// marges), comme ScrollBar.size et ScrollBar.position de Qt Quick Controls.
// Page plus petite que la vue : taille 1, position 0.
struct ScrollAxis
{
    double size = 1.0;
    double position = 0.0;
};
ScrollAxis scrollAxis(double pan, double viewLength, double contentLength, double margin = kViewMargin);
// Inverse de scrollAxis : défilement pour une position (bornée à [0, 1 − taille]) ; page plus petite que la
// vue : centrée.
double panForScrollPosition(double position, double viewLength, double contentLength,
                            double margin = kViewMargin);

} // namespace actcraft::ui
