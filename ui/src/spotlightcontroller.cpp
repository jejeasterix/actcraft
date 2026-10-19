#include "spotlightcontroller.h"

#include "presentationdocument.h"

#include <actcraft/features/camera.h>

#include <QBuffer>
#include <QByteArray>
#include <QRect>

#include <cmath>
#include <numbers>

namespace actcraft::ui {

namespace {

using Shape = features::Spotlight::Shape;
using CoreShape = core::SpotlightShape;

// Mêmes valeurs des deux côtés : la conversion est un simple transtypage, vérifié valeur par valeur.
static_assert(static_cast<int>(Shape::Off) == static_cast<int>(CoreShape::Off));
static_assert(static_cast<int>(Shape::Circle) == static_cast<int>(CoreShape::Circle));
static_assert(static_cast<int>(Shape::Square) == static_cast<int>(CoreShape::Square));
static_assert(static_cast<int>(Shape::SolidCircle) == static_cast<int>(CoreShape::SolidCircle));
static_assert(static_cast<int>(Shape::SolidSquare) == static_cast<int>(CoreShape::SolidSquare));

constexpr int kEllipseVertices = 96;

bool isRound(Shape shape) noexcept
{
    return shape == Shape::Circle || shape == Shape::SolidCircle;
}

} // namespace

SpotlightController::SpotlightController(QObject* parent) : QObject(parent), m_spot(QSizeF()) {}

core::SpotlightShape SpotlightController::toCore(features::Spotlight::Shape shape) noexcept
{
    return static_cast<CoreShape>(static_cast<int>(shape));
}

features::Spotlight::Shape SpotlightController::fromCore(core::SpotlightShape shape) noexcept
{
    return static_cast<Shape>(static_cast<int>(shape));
}

int SpotlightController::shape() const noexcept
{
    return static_cast<int>(toCore(m_spot.shape()));
}

bool SpotlightController::active() const noexcept
{
    return m_spot.shape() != Shape::Off;
}

bool SpotlightController::hidesInside() const noexcept
{
    return m_spot.hidesInside();
}

QColor SpotlightController::color() const noexcept
{
    return m_color;
}

void SpotlightController::setColor(const QColor& color)
{
    if (!color.isValid() || m_color == color) {
        return;
    }
    m_color = color;
    emit colorChanged();
}

QSizeF SpotlightController::area() const noexcept
{
    return m_area;
}

void SpotlightController::setArea(QSizeF area)
{
    if (m_area == area) {
        return;
    }
    const bool first = m_area.isEmpty();
    m_area = area;
    m_spot.setArea(area);
    if (first) {
        m_spot.setFrame(QRectF()); // le cadre par défaut se centre sur la zone connue, pas sur (0, 0)
    }
    emit changed();
}

QRectF SpotlightController::frame() const noexcept
{
    return m_spot.frame();
}

qreal SpotlightController::centerX() const noexcept
{
    return m_spot.frame().center().x();
}

qreal SpotlightController::centerY() const noexcept
{
    return m_spot.frame().center().y();
}

qreal SpotlightController::frameWidth() const noexcept
{
    return m_spot.frame().width();
}

qreal SpotlightController::frameHeight() const noexcept
{
    return m_spot.frame().height();
}

qreal SpotlightController::outline() const noexcept
{
    return features::Spotlight::kDefaultOutline;
}

bool SpotlightController::dragging() const noexcept
{
    return m_spot.dragging();
}

bool SpotlightController::press(QPointF point)
{
    const bool grabbed = m_spot.press(point);
    emit changed();
    return grabbed;
}

void SpotlightController::move(QPointF point)
{
    if (!m_spot.dragging()) {
        return;
    }
    m_spot.move(point);
    emit changed();
}

void SpotlightController::release()
{
    m_spot.release();
    emit changed();
}

void SpotlightController::toggle(int shape)
{
    if (shape < static_cast<int>(CoreShape::Circle) || shape > static_cast<int>(CoreShape::SolidSquare)) {
        return;
    }
    const Shape wanted = fromCore(static_cast<CoreShape>(shape));
    if (m_spot.shape() == wanted) {
        m_spot.setShape(Shape::Off);
    } else {
        if (m_spot.shape() == Shape::Off) {
            m_spot.setFrame(QRectF());
        }
        m_spot.setShape(wanted);
    }
    emit changed();
}

bool SpotlightController::covers(QPointF point) const
{
    features::Spotlight probe = m_spot;
    return probe.press(point);
}

int SpotlightController::zoneAt(QPointF point) const
{
    if (!covers(point)) {
        return None;
    }
    // Zone lue sur le store lui-même, sans second algorithme : on rejoue l'appui sur une copie, tiré très
    // loin dans un sens puis dans l'autre. Un côté du pourtour change la largeur ou la hauteur (dans l'un des
    // deux sens au moins, même au cadre minimal) ; le déplacement ne change ni l'une ni l'autre.
    const QRectF before = m_spot.frame();
    bool horizontal = false;
    bool vertical = false;
    for (const double reach : {1.0e5, -1.0e5}) {
        features::Spotlight probe = m_spot;
        probe.press(point);
        probe.move(point + QPointF(reach, reach));
        horizontal = horizontal || probe.frame().width() != before.width();
        vertical = vertical || probe.frame().height() != before.height();
    }
    if (horizontal) {
        return ResizeHorizontal;
    }
    return vertical ? ResizeVertical : Move;
}

int SpotlightController::cursorAt(QPointF point) const
{
    switch (zoneAt(point)) {
    case Move:
        return Qt::SizeAllCursor;
    case ResizeHorizontal:
        return Qt::SizeHorCursor;
    case ResizeVertical:
        return Qt::SizeVerCursor;
    default:
        return Qt::ArrowCursor;
    }
}

QPolygonF SpotlightController::litPolygon() const
{
    if (!active() || m_spot.hidesInside()) {
        return {};
    }
    const QRectF f = m_spot.frame();
    if (!isRound(m_spot.shape())) {
        return QPolygonF(f);
    }
    QPolygonF polygon;
    polygon.reserve(kEllipseVertices);
    for (int i = 0; i < kEllipseVertices; ++i) {
        const double angle = 2.0 * std::numbers::pi * i / kEllipseVertices;
        polygon << QPointF(f.center().x() + f.width() / 2 * std::cos(angle),
                           f.center().y() + f.height() / 2 * std::sin(angle));
    }
    return polygon;
}

bool SpotlightController::insertCapture(const QImage& page, DocumentController* document)
{
    const QPolygonF lit = litPolygon();
    if (!document || page.isNull() || lit.isEmpty() || m_area.isEmpty()) {
        return false;
    }
    // Image de la page entière : un pixel par unité de page si elle a la taille de la zone, sinon à
    // l'échelle.
    const double scale = page.width() / m_area.width();
    QPolygonF inImage;
    inImage.reserve(lit.size());
    for (const QPointF& vertex : lit) {
        inImage << vertex * scale;
    }
    const QImage cropped = features::cropToPolygon(page, inImage);
    if (cropped.isNull()) {
        return false;
    }
    const QRect box = inImage.boundingRect().toAlignedRect() & page.rect();
    QByteArray png;
    QBuffer buffer(&png);
    if (!buffer.open(QIODevice::WriteOnly) || !cropped.save(&buffer, "PNG")) {
        return false;
    }
    core::ImageObject image;
    image.bounds = core::RectF{box.x() / scale, box.y() / scale, box.width() / scale, box.height() / scale};
    image.sourceWidth = cropped.width();
    image.sourceHeight = cropped.height();
    DocumentInsertions insertions(*document);
    return insertions.insertImage(png, "png", image, false) != 0;
}

} // namespace actcraft::ui
