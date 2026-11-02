#include <actcraft/ui/viewtransform.h>

#include <algorithm>
#include <cmath>

namespace actcraft::ui {

double clampZoom(double zoom) noexcept
{
    return std::clamp(zoom, kMinZoom, kMaxZoom);
}

double wheelZoomFactor(int angleDeltaY) noexcept
{
    return std::pow(kZoomStep, angleDeltaY / 120.0);
}

ViewTransform zoomAt(const ViewTransform& t, QPointF anchor, double newZoom)
{
    const QPointF page = t.viewToPage(anchor);
    ViewTransform result;
    result.zoom = clampZoom(newZoom);
    result.pan = anchor - page * result.zoom;
    return result;
}

ViewTransform clampPan(const ViewTransform& t, QSizeF view, QSizeF page, double margin)
{
    const auto axis = [margin](double pan, double viewLength, double pageLength) {
        if (pageLength + 2 * margin <= viewLength) {
            return (viewLength - pageLength) / 2;
        }
        return std::clamp(pan, viewLength - pageLength - margin, margin);
    };
    ViewTransform result = t;
    result.pan.setX(axis(t.pan.x(), view.width(), page.width() * t.zoom));
    result.pan.setY(axis(t.pan.y(), view.height(), page.height() * t.zoom));
    return result;
}

ViewTransform fitPage(QSizeF view, QSizeF page, double margin)
{
    ViewTransform result;
    result.zoom = clampZoom(
        std::min((view.width() - 2 * margin) / page.width(), (view.height() - 2 * margin) / page.height()));
    result.pan = QPointF((view.width() - page.width() * result.zoom) / 2,
                         (view.height() - page.height() * result.zoom) / 2);
    return result;
}

QPointF wheelScrollDelta(QPoint angleDelta, QPoint pixelDelta, Qt::KeyboardModifiers modifiers)
{
    QPointF delta =
        !pixelDelta.isNull() ? QPointF(pixelDelta) : QPointF(angleDelta) * (kScrollPixelsPerNotch / 120.0);
    // Maj + molette = horizontal, seulement si la molette n'a pas déjà une composante x (macOS la donne).
    if (delta.x() == 0 && (modifiers & Qt::ShiftModifier)) {
        delta = QPointF(delta.y(), 0);
    }
    return delta;
}

QSize cacheTextureSize(QSizeF page, double zoom, double devicePixelRatio, int maxSide)
{
    double w = std::ceil(page.width() * zoom * devicePixelRatio);
    double h = std::ceil(page.height() * zoom * devicePixelRatio);
    const double longest = std::max(w, h);
    if (longest > maxSide) {
        const double scale = maxSide / longest;
        w = std::min<double>(maxSide, std::round(w * scale));
        h = std::min<double>(maxSide, std::round(h * scale));
    }
    return QSize(std::max(1, static_cast<int>(w)), std::max(1, static_cast<int>(h)));
}

int cacheMaxSide(QSizeF page, double devicePixelRatio, int cacheSamples, double sharpZoom)
{
    if (cacheSamples <= 1) {
        return kMaxCacheSide;
    }
    const double zoom = std::max(1.0, sharpZoom);
    const double sharpSide = std::ceil(std::max(page.width(), page.height()) * zoom * devicePixelRatio);
    return std::min(kMaxCacheSide, std::max(kMaxMsaaCacheSide, static_cast<int>(sharpSide)));
}

ScrollAxis scrollAxis(double pan, double viewLength, double contentLength, double margin)
{
    const double extent = contentLength + 2 * margin;
    if (extent <= viewLength || extent <= 0) {
        return {};
    }
    ScrollAxis axis;
    axis.size = viewLength / extent;
    axis.position = std::clamp((margin - pan) / extent, 0.0, 1.0 - axis.size);
    return axis;
}

double panForScrollPosition(double position, double viewLength, double contentLength, double margin)
{
    const double extent = contentLength + 2 * margin;
    if (extent <= viewLength) {
        return (viewLength - contentLength) / 2;
    }
    const double p = std::clamp(position, 0.0, 1.0 - viewLength / extent);
    return margin - p * extent;
}

} // namespace actcraft::ui
