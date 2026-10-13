#include <actcraft/render/strokegeometry.h>

#include <algorithm>
#include <cmath>
#include <numbers>

namespace actcraft::render {

namespace {

float f(double v)
{
    return static_cast<float>(v);
}

void appendQuad(core::PointF a, core::PointF b, double width, std::vector<Vertex2D>& out)
{
    const double dx = b.x - a.x;
    const double dy = b.y - a.y;
    const double length = std::sqrt(dx * dx + dy * dy);
    if (length < 1e-9) {
        for (std::size_t i = 0; i < kVerticesPerSegment; ++i) {
            out.push_back({f(a.x), f(a.y)});
        }
        return;
    }
    const double half = width / 2;
    const double ux = dx / length;
    const double uy = dy / length;
    const double nx = -uy * half;
    const double ny = ux * half;
    const double ax = a.x - ux * half;
    const double ay = a.y - uy * half;
    const double bx = b.x + ux * half;
    const double by = b.y + uy * half;
    const Vertex2D p0{f(ax + nx), f(ay + ny)};
    const Vertex2D p1{f(ax - nx), f(ay - ny)};
    const Vertex2D p2{f(bx + nx), f(by + ny)};
    const Vertex2D p3{f(bx - nx), f(by - ny)};
    out.insert(out.end(), {p0, p1, p2, p2, p1, p3});
}

} // namespace

std::size_t appendStrokeTriangles(std::span<const core::StrokePoint> points, double baseWidth,
                                  std::vector<Vertex2D>& out)
{
    const std::size_t before = out.size();
    for (std::size_t i = 0; i + 1 < points.size(); ++i) {
        appendQuad({points[i].x, points[i].y}, {points[i + 1].x, points[i + 1].y},
                   core::strokeWidthAt(baseWidth, points[i].pressure), out);
    }
    return out.size() - before;
}

std::size_t appendOutlineTriangles(std::span<const core::PointF> polygon, double width,
                                   std::vector<Vertex2D>& out)
{
    const std::size_t before = out.size();
    for (std::size_t i = 0; i < polygon.size(); ++i) {
        appendQuad(polygon[i], polygon[(i + 1) % polygon.size()], width, out);
    }
    return out.size() - before;
}

std::size_t appendConvexFill(std::span<const core::PointF> polygon, std::vector<Vertex2D>& out)
{
    const std::size_t before = out.size();
    for (std::size_t i = 1; i + 1 < polygon.size(); ++i) {
        out.push_back({f(polygon[0].x), f(polygon[0].y)});
        out.push_back({f(polygon[i].x), f(polygon[i].y)});
        out.push_back({f(polygon[i + 1].x), f(polygon[i + 1].y)});
    }
    return out.size() - before;
}

std::vector<core::PointF> rectanglePolygon(const core::RectF& r)
{
    return {{r.x, r.y}, {r.x + r.width, r.y}, {r.x + r.width, r.y + r.height}, {r.x, r.y + r.height}};
}

std::vector<core::PointF> ellipsePolygon(const core::RectF& r, int segments)
{
    std::vector<core::PointF> out;
    out.reserve(static_cast<std::size_t>(segments));
    const double cx = r.x + r.width / 2;
    const double cy = r.y + r.height / 2;
    for (int i = 0; i < segments; ++i) {
        const double angle = 2 * std::numbers::pi * i / segments;
        out.push_back({cx + r.width / 2 * std::cos(angle), cy + r.height / 2 * std::sin(angle)});
    }
    return out;
}

core::RectF strokeBounds(std::span<const core::StrokePoint> points, double baseWidth)
{
    if (points.empty()) {
        return {};
    }
    double minX = points[0].x;
    double maxX = points[0].x;
    double minY = points[0].y;
    double maxY = points[0].y;
    double half = 0;
    for (const auto& p : points) {
        minX = std::min(minX, p.x);
        maxX = std::max(maxX, p.x);
        minY = std::min(minY, p.y);
        maxY = std::max(maxY, p.y);
        half = std::max(half, core::strokeWidthAt(baseWidth, p.pressure) / 2);
    }
    return {minX - half, minY - half, maxX - minX + 2 * half, maxY - minY + 2 * half};
}

} // namespace actcraft::render
