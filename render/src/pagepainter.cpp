#include <actcraft/render/imagesynth.h>
#include <actcraft/render/objectrenderer.h>
#include <actcraft/render/pagepainter.h>
#include <actcraft/render/painternode.h>
#include <actcraft/render/strokegeometry.h>

#include <QAbstractTextDocumentLayout>
#include <QBuffer>
#include <QFontMetricsF>
#include <QImageReader>
#include <QLineF>
#include <QLinearGradient>
#include <QPainter>
#include <QPainterPath>
#include <QPolygonF>
#include <QRadialGradient>
#include <QTextDocument>
#include <QTextFormat>
#include <QUrl>
#include <QVariant>

#include <algorithm>
#include <cmath>

namespace actcraft::render {

namespace {

constexpr int kMaxGridLines = 4000; // garde-fou : pas minuscule sur une grande page (grid.py)

// Garde-fou par page, tous niveaux confondus (rangeOf ne borne qu'un niveau).
struct GridBudget
{
    std::size_t lines = static_cast<std::size_t>(kMaxGridLines);
    std::size_t marks = static_cast<std::size_t>(kMaxGridLines) * 8;
};

// HTML des fichiers ouverts (.flipchart, format natif) : aucune ressource externe. Toute ressource demandée
// (image, fond `background`, `background-image`, feuille de style `<link>` ou `@import`, toutes URL et tous
// schémas, dont file://hôte/partage) reçoit une valeur vide mais valide : QTextDocument ne se rabat alors ni
// sur un fournisseur de ressources, ni sur la lecture directe du fichier que font l'affichage des images et
// les fonds quand la ressource est nulle.
// Images du HTML : ni lues ni dessinées, leur place seule est gardée. Remplace le gestionnaire de Qt, qui
// cherche « nom@2x.png » sur le disque avant toute ressource dès que la densité de pixels dépasse 1
// (résolution réseau d'un chemin UNC, existence d'un fichier local révélée).
class NoImageHandler final : public QObject, public QTextObjectInterface
{
    Q_OBJECT
    Q_INTERFACES(QTextObjectInterface)

public:
    QSizeF intrinsicSize(QTextDocument*, int, const QTextFormat& format) override
    {
        const QTextImageFormat image = format.toImageFormat();
        return QSizeF(std::max(0.0, image.width()), std::max(0.0, image.height()));
    }
    void drawObject(QPainter*, const QRectF&, QTextDocument*, int, const QTextFormat&) override {}
};

class IsolatedTextDocument final : public QTextDocument
{
public:
    IsolatedTextDocument()
    {
        setResourceProvider([](const QUrl&) { return QVariant(QByteArray()); });
        documentLayout()->registerHandler(QTextFormat::ImageObject, &m_images);
    }

protected:
    QVariant loadResource(int type, const QUrl&) override
    {
        if (type == QTextDocument::ImageResource) {
            QImage blank(1, 1, QImage::Format_ARGB32_Premultiplied);
            blank.fill(Qt::transparent);
            return blank;
        }
        if (type == QTextDocument::StyleSheetResource) {
            return QString();
        }
        return QByteArray();
    }

private:
    NoImageHandler m_images;
};

// rstrip() de Python : espaces de fin retirés, espaces de tête gardés.
QString withoutTrailingSpaces(const QString& text)
{
    qsizetype end = text.size();
    while (end > 0 && text.at(end - 1).isSpace()) {
        --end;
    }
    return text.left(end);
}

QColor color(core::Argb argb)
{
    return QColor::fromRgba(argb);
}

QRectF rectOf(const core::RectF& r)
{
    return QRectF(r.x, r.y, r.width, r.height);
}

QPen penOf(const core::Outline& o)
{
    if (o.style == core::PenStyle::None || o.width <= 0) {
        return QPen(Qt::NoPen);
    }
    QPen pen(color(o.color));
    pen.setWidthF(o.width);
    pen.setStyle(static_cast<Qt::PenStyle>(static_cast<int>(o.style)));
    pen.setCapStyle(o.cap == core::PenCap::Flat     ? Qt::FlatCap
                    : o.cap == core::PenCap::Square ? Qt::SquareCap
                                                    : Qt::RoundCap);
    pen.setJoinStyle(o.join == core::PenJoin::Miter   ? Qt::MiterJoin
                     : o.join == core::PenJoin::Bevel ? Qt::BevelJoin
                                                      : Qt::RoundJoin);
    return pen;
}

QBrush brushOf(const core::Fill& f, const QRectF& rect)
{
    switch (f.kind) {
    case core::FillKind::None:
        return QBrush(Qt::NoBrush);
    case core::FillKind::Solid:
        return QBrush(color(f.color));
    case core::FillKind::Pattern:
        // Fond opaque du motif (F-574) : posé par le peintre de la forme (paintShape), pas par la brosse.
        return QBrush(color(f.color), static_cast<Qt::BrushStyle>(std::clamp<int>(f.pattern, 2, 14)));
    case core::FillKind::Gradient:
        break;
    }
    QGradientStops stops{{0.0, color(f.color)}, {1.0, color(f.color2)}};
    switch (f.gradient) {
    case core::GradientKind::Radial: {
        QRadialGradient g(rect.center(), std::max(rect.width(), rect.height()) / 2);
        g.setStops(stops);
        return QBrush(g);
    }
    case core::GradientKind::Horizontal: {
        QLinearGradient g(rect.topLeft(), rect.topRight());
        g.setStops(stops);
        return QBrush(g);
    }
    case core::GradientKind::DiagonalDown: {
        QLinearGradient g(rect.topLeft(), rect.bottomRight());
        g.setStops(stops);
        return QBrush(g);
    }
    case core::GradientKind::DiagonalUp: {
        QLinearGradient g(rect.topRight(), rect.bottomLeft());
        g.setStops(stops);
        return QBrush(g);
    }
    case core::GradientKind::Vertical:
        break;
    }
    QLinearGradient g(rect.topLeft(), rect.bottomLeft());
    g.setStops(stops);
    return QBrush(g);
}

// Pixels de la couleur `transparent` (alpha ignoré) rendus transparents.
QImage withKeyColor(const QImage& source, core::Argb transparent)
{
    QImage image = source.convertToFormat(QImage::Format_ARGB32);
    const QRgb key = transparent & 0x00FFFFFFu;
    for (int y = 0; y < image.height(); ++y) {
        auto* line = reinterpret_cast<QRgb*>(image.scanLine(y));
        for (int x = 0; x < image.width(); ++x) {
            if ((line[x] & 0x00FFFFFFu) == key) {
                line[x] = 0;
            }
        }
    }
    return image;
}

QPainterPath polyline(const std::vector<core::PointF>& points)
{
    QPainterPath path;
    if (points.empty()) {
        return path;
    }
    path.moveTo(points.front().x, points.front().y);
    for (std::size_t i = 1; i < points.size(); ++i) {
        path.lineTo(points[i].x, points[i].y);
    }
    return path;
}

void drawPolyline(QPainter& p, const std::vector<core::PointF>& points)
{
    if (points.size() == 1 ||
        (points.size() == 2 && points[0].x == points[1].x && points[0].y == points[1].y)) {
        p.drawPoint(QPointF(points[0].x, points[0].y));
    } else if (!points.empty()) {
        p.drawPath(polyline(points));
    }
}

void paintStroke(QPainter& p, const core::Stroke& s)
{
    if (s.kind == core::StrokeKind::Pen && s.cap == core::PenCap::Square) {
        paintStrokeSegments(p, s.points, s.baseWidth, color(s.color));
        return;
    }
    QPen pen(color(s.color));
    pen.setWidthF(s.baseWidth);
    pen.setCapStyle(s.cap == core::PenCap::Flat     ? Qt::FlatCap
                    : s.cap == core::PenCap::Square ? Qt::SquareCap
                                                    : Qt::RoundCap);
    pen.setJoinStyle(Qt::RoundJoin);
    p.setPen(pen);
    p.setBrush(Qt::NoBrush);
    std::vector<core::PointF> points;
    points.reserve(s.points.size());
    for (const core::StrokePoint& q : s.points) {
        points.push_back(core::PointF{q.x, q.y});
    }
    drawPolyline(p, points);
}

void paintShape(QPainter& p, const core::Shape& s)
{
    if (s.kind == core::ShapeKind::Path) {
        QPainterPath path;
        path.setFillRule(s.windingFill ? Qt::WindingFill : Qt::OddEvenFill);
        for (const auto& poly : s.paths) {
            if (poly.empty()) {
                continue;
            }
            path.moveTo(poly.front().x, poly.front().y);
            for (std::size_t i = 1; i < poly.size(); ++i) {
                path.lineTo(poly[i].x, poly[i].y);
            }
            if (!s.open) {
                path.closeSubpath();
            }
        }
        if (!s.open) {
            // F-574, relevé sur ActivInspire (L2-C9) : en mode opaque, la couleur de fond est peinte sous le
            // motif, avec l'alpha de la couleur du contour. Limité au remplissage : le fond dans les trous
            // d'un contour en tirets n'est pas repris.
            const bool opaque = s.brush.kind == core::FillKind::Pattern && s.brush.opaqueBackground;
            if (opaque) {
                QColor background = color(s.brush.backgroundColor);
                background.setAlpha(qAlpha(s.pen.color));
                p.setBackgroundMode(Qt::OpaqueMode);
                p.setBackground(background);
            }
            p.fillPath(path, brushOf(s.brush, path.boundingRect()));
            if (opaque) {
                p.setBackgroundMode(Qt::TransparentMode);
            }
        }
        p.strokePath(path, penOf(s.pen));
        return;
    }
    // Formes du prototype : comme ShapePainterNode (shaperenderer.cpp).
    std::vector<core::PointF> outline;
    switch (s.kind) {
    case core::ShapeKind::Rectangle:
        outline = rectanglePolygon(s.bounds);
        break;
    case core::ShapeKind::Ellipse:
        outline = ellipsePolygon(s.bounds, 48);
        break;
    default:
        outline = s.polygon;
        break;
    }
    QPolygonF polygon;
    for (const core::PointF& q : outline) {
        polygon << QPointF(q.x, q.y);
    }
    p.setOpacity(p.opacity() * s.opacity);
    p.setBrush(color(s.fill));
    if (s.outlineWidth > 0) {
        QPen pen(color(s.outline));
        pen.setWidthF(s.outlineWidth);
        pen.setJoinStyle(Qt::MiterJoin);
        p.setPen(pen);
    } else {
        p.setPen(Qt::NoPen);
    }
    p.drawPolygon(polygon);
}

void paintImage(QPainter& p, const core::ImageObject& im, const ImageLookup& images)
{
    const QRectF rect = rectOf(im.bounds);
    QImage image = withoutColorTable(im.resource.empty() ? synthesizeImage(im)
                                                         : (images ? images(im.resource) : QImage()));
    if (image.isNull()) {
        p.fillRect(rect, QColor(255, 0, 255, 60)); // repère d'image illisible, comme la référence
        return;
    }
    if (im.hasKeyColor) {
        image = withKeyColor(image, im.keyColor);
    }
    p.setRenderHint(QPainter::SmoothPixmapTransform, true);
    p.drawImage(rect, image);
}

void paintText(QPainter& p, const core::Text& t)
{
    if (!t.runs.empty()) {
        for (const core::TextSpan& run : t.runs) {
            const QString text = QString::fromStdString(run.text);
            QFont font(QString::fromStdString(run.fontFamily));
            font.setPointSizeF(std::max(1.0, run.pointSize));
            font.setWeight(static_cast<QFont::Weight>(std::clamp(run.weight, 100, 900)));
            font.setItalic(run.italic);
            font.setUnderline(run.underline);
            font.setStrikeOut(run.strikeOut);
            p.setFont(font);
            // Les métriques de certaines polices diffèrent entre l'original et Qt 6 : on recale la taille sur
            // la largeur mesurée par l'original (rectangle du run), comme la référence (render_flipchart.py :
            // texte sans ses espaces de fin, rstrip ; 4 caractères au moins hors espaces, strip).
            const double target = run.bounds.width;
            const double advance =
                QFontMetricsF(font, p.device()).horizontalAdvance(withoutTrailingSpaces(text));
            if (target > 0 && advance > 0 && text.trimmed().size() >= 4) {
                const double ratio = target / advance;
                if (std::abs(ratio - 1.0) > 0.06 && ratio > 0.5 && ratio < 2.0) {
                    font.setPointSizeF(std::max(1.0, run.pointSize * ratio));
                    p.setFont(font);
                }
            }
            p.setPen(color(run.color));
            p.drawText(QPointF(run.baseline.x, run.baseline.y), text);
        }
        return;
    }
    if (t.html.empty()) {
        return;
    }
    IsolatedTextDocument document;
    document.setDocumentMargin(0);
    document.setHtml(QString::fromStdString(t.html));
    if (t.bounds.width > 0) {
        document.setTextWidth(t.bounds.width);
    }
    p.translate(t.bounds.x, t.bounds.y);
    document.drawContents(&p, QRectF(0, 0, t.bounds.width > 0 ? t.bounds.width : 2000,
                                     t.bounds.height > 0 ? t.bounds.height : 2000));
}

void paintMedia(QPainter& p, const core::Media& m, const ImageLookup& images)
{
    const QRectF rect = rectOf(m.bounds);
    const QImage placeholder =
        withoutColorTable(m.placeholder.empty() || !images ? QImage() : images(m.placeholder));
    if (!placeholder.isNull()) {
        p.drawImage(rect, placeholder);
        return;
    }
    p.setPen(QPen(QColor(80, 80, 80), 2, Qt::DashLine));
    p.setBrush(QColor(0, 0, 0, 30));
    p.drawRect(rect);
    p.drawText(rect, Qt::AlignCenter, QStringLiteral("média"));
}

// --- Grille (grid.py)
// -----------------------------------------------------------------------------------------

struct Range
{
    long long first = 0;
    long long last = -1;
};

Range rangeOf(double start, double step, double lo, double hi)
{
    Range r;
    r.first = static_cast<long long>(std::ceil((lo - start) / step - 1e-9));
    r.last =
        std::min(static_cast<long long>(std::floor((hi - start) / step + 1e-9)), r.first + kMaxGridLines);
    return r;
}

double cotangent(const core::GridLevel& l)
{
    const double degrees = l.pattern == core::GridPattern::Diagonal45 ? 45.0 : std::clamp(l.angle, 1.0, 89.0);
    return 1.0 / std::tan(degrees * 3.14159265358979323846 / 180.0);
}

// Liang-Barsky : segment coupé au rectangle ; faux s'il en sort entièrement.
bool clip(QLineF& line, const QRectF& r)
{
    const double x0 = line.x1();
    const double y0 = line.y1();
    const double dx = line.dx();
    const double dy = line.dy();
    double t0 = 0;
    double t1 = 1;
    const double ps[4] = {-dx, dx, -dy, dy};
    const double qs[4] = {x0 - r.left(), r.right() - x0, y0 - r.top(), r.bottom() - y0};
    for (int i = 0; i < 4; ++i) {
        if (ps[i] == 0) {
            if (qs[i] < 0) {
                return false;
            }
            continue;
        }
        const double t = qs[i] / ps[i];
        if (ps[i] < 0) {
            t0 = std::max(t0, t);
        } else {
            t1 = std::min(t1, t);
        }
        if (t0 > t1) {
            return false;
        }
    }
    line = QLineF(x0 + t0 * dx, y0 + t0 * dy, x0 + t1 * dx, y0 + t1 * dy);
    return true;
}

std::vector<QLineF> gridLines(const core::GridLevel& l, double w, double h, double scale)
{
    const double sx = std::max(l.spacingX * scale, 1.0);
    const double sy = std::max(l.spacingY * scale, 1.0);
    std::vector<QLineF> out;
    if (l.pattern == core::GridPattern::Normal) {
        const double x0 = std::max(0.0, l.start.x);
        const double y0 = std::max(0.0, l.start.y);
        if (l.linesAlongX) {
            const Range r = rangeOf(l.start.x, sx, x0, w);
            for (long long k = r.first; k <= r.last; ++k) {
                const double x = l.start.x + static_cast<double>(k) * sx;
                out.emplace_back(x, y0, x, h);
            }
        }
        if (l.linesAlongY) {
            const Range r = rangeOf(l.start.y, sy, y0, h);
            for (long long k = r.first; k <= r.last; ++k) {
                const double y = l.start.y + static_cast<double>(k) * sy;
                out.emplace_back(x0, y, w, y);
            }
        }
        return out;
    }
    const double cot = cotangent(l);
    const QRectF page(0, 0, w, h);
    const struct
    {
        double sign;
        bool on;
        double step;
    } families[2] = {{1, l.linesAlongX, sx}, {-1, l.linesAlongY, sy}};
    for (const auto& f : families) {
        if (!f.on) {
            continue;
        }
        const double reach = h * cot + f.step;
        const Range r = rangeOf(l.start.x, f.step, -reach, w + reach);
        for (long long k = r.first; k <= r.last; ++k) {
            const double x0 = l.start.x + static_cast<double>(k) * f.step;
            QLineF seg(x0 + f.sign * (0 - l.start.y) * cot, 0, x0 + f.sign * (h - l.start.y) * cot, h);
            if (clip(seg, page) && seg.length() > 1e-6) {
                out.push_back(seg);
            }
        }
    }
    return out;
}

std::vector<QPointF> gridPoints(const core::GridLevel& l, double w, double h, double scale, std::size_t limit)
{
    const double sx = std::max(l.spacingX * scale, 1.0);
    const double sy = std::max(l.spacingY * scale, 1.0);
    std::vector<QPointF> out;
    if (l.pattern == core::GridPattern::Normal) {
        if (!l.linesAlongX || !l.linesAlongY) {
            return out;
        }
        const Range rx = rangeOf(l.start.x, sx, std::max(0.0, l.start.x), w);
        const Range ry = rangeOf(l.start.y, sy, std::max(0.0, l.start.y), h);
        for (long long i = rx.first; i <= rx.last; ++i) {
            for (long long j = ry.first; j <= ry.last; ++j) {
                if (out.size() >= limit) {
                    return out;
                }
                out.emplace_back(l.start.x + static_cast<double>(i) * sx,
                                 l.start.y + static_cast<double>(j) * sy);
                if (out.size() >= limit) {
                    return out;
                }
            }
        }
        return out;
    }
    const double cot = cotangent(l);
    const double reach = h * cot + std::max(sx, sy);
    const Range ri = rangeOf(l.start.x, sx, -reach, w + reach);
    for (long long i = ri.first; i <= ri.last; ++i) {
        const double a = l.start.x + static_cast<double>(i) * sx;
        const Range rj = rangeOf(l.start.x, sy, a + 2 * (0 - l.start.y) * cot, a + 2 * (h - l.start.y) * cot);
        for (long long j = rj.first; j <= rj.last; ++j) {
            const double b = l.start.x + static_cast<double>(j) * sy;
            const double t = (b - a) / 2;
            const QPointF q(a + t, l.start.y + t / cot);
            if (q.x() >= 0 && q.x() <= w && q.y() >= 0 && q.y() <= h) {
                if (out.size() >= limit) {
                    return out;
                }
                out.push_back(q);
                if (out.size() >= limit) {
                    return out;
                }
            }
        }
    }
    return out;
}

void paintGridLevel(QPainter& p, const core::GridLevel& l, double w, double h, double scale, double width,
                    GridBudget& budget)
{
    const QColor c = color(l.color | 0xFF000000u);
    if (l.line == core::GridLine::Solid || l.line == core::GridLine::Dotted ||
        l.line == core::GridLine::Dashed) {
        QPen pen(c, width); // cosmétique : 1 px écran (0,7 px lissé en miniature)
        pen.setCosmetic(true);
        pen.setCapStyle(Qt::FlatCap);
        if (l.line == core::GridLine::Dotted) {
            pen.setDashPattern({1, 4});
        } else if (l.line == core::GridLine::Dashed) {
            pen.setDashPattern({3, 8});
        }
        p.setPen(pen);
        if (budget.lines == 0) {
            return;
        }
        const std::vector<QLineF> lines = gridLines(l, w, h, scale);
        const std::size_t count = std::min(lines.size(), budget.lines);
        for (std::size_t i = 0; i < count; ++i) {
            p.drawLine(lines[i]);
        }
        budget.lines -= count;
        return;
    }
    // Marques aux intersections, en pixels écran.
    const QTransform t = p.worldTransform();
    std::vector<QPointF> marks;
    const std::vector<QPointF> points = gridPoints(l, w, h, scale, budget.marks);
    budget.marks -= points.size();
    for (const QPointF& q : points) {
        marks.push_back(t.map(q));
    }
    p.save();
    p.resetTransform();
    for (const QPointF& q : marks) {
        const int x = static_cast<int>(std::floor(q.x()));
        const int y = static_cast<int>(std::nearbyint(q.y())); // round() de Python : au pair
        if (l.line == core::GridLine::Points) {
            p.fillRect(static_cast<int>(std::ceil(q.x() - 0.2)), y, 1, 1, c);
        } else if (l.line == core::GridLine::Cross) {
            p.fillRect(x - 2, y, 5, 1, c);
            p.fillRect(x, y - 2, 1, 5, c);
        } else {
            p.fillRect(x - 1, y - 1, 3, 3, c);
        }
    }
    p.restore();
}

// Objet d'un type inconnu (L2-C8) : repère neutre, cadre gris pointillé d'un pixel quel que soit le zoom
// (plume cosmétique, que Qt découpe au périphérique : une boîte démesurée ne coûte rien), sur sa boîte s'il
// en a une. Ni texte, ni HTML, ni ressource.
void paintUnknown(QPainter& p, const core::UnknownObject& unknown)
{
    if (!unknown.hasBounds) {
        return;
    }
    QPen pen(QColor(0x80, 0x80, 0x80), 0, Qt::DashLine);
    pen.setCosmetic(true);
    p.setPen(pen);
    p.setBrush(Qt::NoBrush);
    const core::RectF& b = unknown.bounds;
    p.drawRect(QRectF(b.x, b.y, b.width, b.height));
}

} // namespace

void DecodedImages::insert(std::string key, QImage image)
{
    m_images[std::move(key)] = std::move(image);
}

QImage DecodedImages::find(const std::string& key) const
{
    const auto it = m_images.find(key);
    return it == m_images.end() ? QImage() : it->second;
}

std::size_t DecodedImages::size() const noexcept
{
    return m_images.size();
}

ImageLookup DecodedImages::lookup() const
{
    return [this](const std::string& key) { return find(key); };
}

DecodedImages decodeImages(const core::Document& document, int maxSide)
{
    DecodedImages out;
    for (const auto& [key, resource] : document.resources()) {
        QBuffer buffer;
        buffer.setData(resource.data);
        buffer.open(QIODevice::ReadOnly);
        QImageReader reader(&buffer);
        reader.setDecideFormatFromContent(true);
        if (maxSide > 0) {
            const QSize size = reader.size();
            if (size.isValid() && (size.width() > maxSide || size.height() > maxSide)) {
                reader.setScaledSize(
                    size.scaled(maxSide, maxSide, Qt::KeepAspectRatio).expandedTo(QSize(1, 1)));
            }
        }
        // Réduite par le lecteur (maxSide), une image indexée est déjà convertie par Qt, table complétée.
        QImage image = withoutColorTable(reader.read());
        if (!image.isNull()) {
            out.insert(key, std::move(image));
        }
    }
    return out;
}

void paintObjectLocal(QPainter& p, const core::PageObject& object, const ImageLookup& images)
{
    p.save();
    p.setRenderHint(QPainter::Antialiasing, true);
    p.setRenderHint(QPainter::TextAntialiasing, true);
    if (const auto* s = std::get_if<core::Stroke>(&object)) {
        paintStroke(p, *s);
    } else if (const auto* l = std::get_if<core::Line>(&object)) {
        p.setPen(penOf(l->pen));
        p.setBrush(Qt::NoBrush);
        drawPolyline(p, l->points);
    } else if (const auto* sh = std::get_if<core::Shape>(&object)) {
        paintShape(p, *sh);
    } else if (const auto* im = std::get_if<core::ImageObject>(&object)) {
        paintImage(p, *im, images);
    } else if (const auto* t = std::get_if<core::Text>(&object)) {
        paintText(p, *t);
    } else if (const auto* m = std::get_if<core::Media>(&object)) {
        paintMedia(p, *m, images);
    } else if (const auto* c = std::get_if<core::Connector>(&object)) {
        p.setPen(penOf(c->pen));
        p.drawLine(QPointF(c->firstEnd.x, c->firstEnd.y), QPointF(c->secondEnd.x, c->secondEnd.y));
    } else if (const auto* u = std::get_if<core::UnknownObject>(&object)) {
        paintUnknown(p, *u);
    }
    p.restore();
}

void paintObject(QPainter& p, const core::PageObject& object, const ImageLookup& images)
{
    if (!object.common.visible || std::get_if<core::Group>(&object)) {
        return;
    }
    p.save();
    const core::Transform& t = object.common.transform;
    p.setTransform(QTransform(t.m11, t.m12, t.m21, t.m22, t.dx, t.dy), true);
    if (object.common.translucency != 0) {
        p.setOpacity(p.opacity() * (1.0 - object.common.translucency / 255.0));
    }
    paintObjectLocal(p, object, images);
    p.restore();
}

void paintBackground(QPainter& p, const core::Page& page, const ImageLookup& images)
{
    const QRectF rect(0, 0, page.width, page.height);
    if (page.background.fill.kind != core::FillKind::None) {
        p.fillRect(rect, brushOf(page.background.fill, rect));
    }
    if (!page.background.image.empty() && images) {
        const QImage image = withoutColorTable(images(page.background.image));
        if (!image.isNull()) {
            p.save();
            p.setRenderHint(QPainter::SmoothPixmapTransform, true);
            p.drawImage(rect, image);
            p.restore();
        }
    }
}

void paintGrid(QPainter& p, const core::Page& page, bool thumbnail)
{
    if (!page.grid.visible) {
        return;
    }
    const double scale = thumbnail ? page.grid.previewZoom : page.grid.baseZoom;
    p.save();
    p.setRenderHint(QPainter::Antialiasing, thumbnail);
    p.setClipRect(QRectF(0, 0, page.width, page.height), Qt::IntersectClip);
    if (!thumbnail) {
        // Les traits nets de l'original tombent sur floor(x + 0,2) (relevé de grid.py).
        p.setWorldTransform(p.worldTransform() * QTransform::fromTranslate(0.2, 0));
    }
    GridBudget budget;
    for (const core::GridLevel& level : page.grid.levels) {
        if (level.visible) {
            paintGridLevel(p, level, page.width, page.height, scale, thumbnail ? 0.7 : 0, budget);
        }
    }
    p.restore();
}

void paintPage(QPainter& p, const core::Page& page, const ImageLookup& images, const PaintOptions& options)
{
    if (options.background) {
        paintBackground(p, page, images);
    }
    if (options.grid && !page.grid.drawnAbove) {
        paintGrid(p, page, options.thumbnail);
    }
    for (const core::PageObject& object : page.objects) {
        paintObject(p, object, images);
    }
    if (options.grid && page.grid.drawnAbove) {
        paintGrid(p, page, options.thumbnail);
    }
}

QImage renderPage(const core::Page& page, const ImageLookup& images, double scale,
                  const PaintOptions& options)
{
    const int w = std::max(1, static_cast<int>(std::lround(page.width * scale)));
    const int h = std::max(1, static_cast<int>(std::lround(page.height * scale)));
    QImage image(w, h, QImage::Format_ARGB32_Premultiplied);
    image.setDotsPerMeterX(3780); // 96 ppp : 1 pt = 4/3 px, comme sous Windows
    image.setDotsPerMeterY(3780);
    image.fill(Qt::white);
    QPainter p(&image);
    p.setRenderHint(QPainter::Antialiasing, true);
    p.setRenderHint(QPainter::TextAntialiasing, true);
    p.scale(scale, scale);
    paintPage(p, page, images, options);
    p.end();
    return image;
}

QByteArray thumbnailPng(const core::Page& page, const ImageLookup& images)
{
    QImage image;
    if (std::isfinite(page.width) && std::isfinite(page.height) && page.width > 0 && page.height > 0) {
        PaintOptions options;
        options.thumbnail = true;
        const double scale = std::min(kThumbnailWidth / page.width, kThumbnailMaxHeight / page.height);
        image = renderPage(page, images, scale, options);
    } else {
        image = QImage(kThumbnailWidth, kThumbnailWidth * 3 / 4, QImage::Format_ARGB32_Premultiplied);
        image.fill(Qt::white);
    }
    QByteArray png;
    QBuffer buffer(&png);
    buffer.open(QIODevice::WriteOnly);
    image.save(&buffer, "PNG");
    return png;
}

} // namespace actcraft::render

#include "pagepainter.moc"
