#include <actcraft/core/erasegeometry.h>

#include <algorithm>
#include <cmath>

namespace actcraft::core {

namespace {

StrokePoint lerp(const StrokePoint& a, const StrokePoint& b, double t) noexcept
{
    return {a.x + t * (b.x - a.x), a.y + t * (b.y - a.y),
            static_cast<float>(a.pressure + t * (static_cast<double>(b.pressure) - a.pressure))};
}

bool insideDisk(const StrokePoint& p, PointF c, double r) noexcept
{
    const double dx = p.x - c.x;
    const double dy = p.y - c.y;
    return dx * dx + dy * dy <= r * r;
}

void closePiece(std::vector<StrokePoint>& current, std::vector<std::vector<StrokePoint>>& pieces)
{
    if (current.size() >= 2)
        pieces.push_back(std::move(current));
    current.clear();
}

} // namespace

StrokeCut cutByDisk(const std::vector<StrokePoint>& points, PointF center, double radius)
{
    StrokeCut result;
    if (points.empty())
        return result;
    std::vector<StrokePoint> current;
    if (!insideDisk(points[0], center, radius))
        current.push_back(points[0]);
    else
        result.touched = true;
    for (std::size_t i = 1; i < points.size(); ++i) {
        const StrokePoint& p0 = points[i - 1];
        const StrokePoint& p1 = points[i];
        const double dx = p1.x - p0.x;
        const double dy = p1.y - p0.y;
        const double a = dx * dx + dy * dy;
        const double fx = p0.x - center.x;
        const double fy = p0.y - center.y;
        const double b = fx * dx + fy * dy;
        const double c = fx * fx + fy * fy - radius * radius;
        double ta = 0;
        double tb = 0;
        bool crosses = false;
        if (a > 0) {
            const double disc = b * b - a * c;
            if (disc > 0) {
                const double root = std::sqrt(disc);
                ta = std::max(0.0, (-b - root) / a);
                tb = std::min(1.0, (-b + root) / a);
                crosses = ta < tb;
            }
        }
        if (!crosses) {
            if (!insideDisk(p1, center, radius))
                current.push_back(p1);
            else
                result.touched = true;
            continue;
        }
        result.touched = true;
        if (ta > 0) {
            current.push_back(lerp(p0, p1, ta));
            closePiece(current, result.pieces);
        } else {
            current.clear(); // p0 était intérieur : aucun morceau ouvert
        }
        if (tb < 1) {
            current.push_back(lerp(p0, p1, tb));
            current.push_back(p1);
        }
    }
    closePiece(current, result.pieces);
    return result;
}

StrokeCut cutBySweep(const std::vector<StrokePoint>& points, PointF from, PointF to, double radius)
{
    const double distance = std::hypot(to.x - from.x, to.y - from.y);
    const int steps = radius > 0 ? std::max(1, static_cast<int>(std::ceil(distance / (radius / 2)))) : 1;
    std::vector<std::vector<StrokePoint>> pieces{points};
    bool touched = false;
    for (int s = 0; s <= steps; ++s) {
        const double t = static_cast<double>(s) / steps;
        const PointF center{from.x + t * (to.x - from.x), from.y + t * (to.y - from.y)};
        std::vector<std::vector<StrokePoint>> next;
        for (const auto& piece : pieces) {
            StrokeCut cut = cutByDisk(piece, center, radius);
            touched = touched || cut.touched;
            for (auto& p : cut.pieces)
                next.push_back(std::move(p));
        }
        pieces = std::move(next);
    }
    return {std::move(pieces), touched};
}

std::vector<StrokePoint> strokePointsOnPage(const PageObject& object)
{
    const auto* stroke = std::get_if<Stroke>(&body(object));
    if (!stroke)
        return {};
    std::vector<StrokePoint> out;
    out.reserve(stroke->points.size());
    for (const StrokePoint& p : stroke->points) {
        const PointF m = object.common.transform.map({p.x, p.y});
        out.push_back({m.x, m.y, p.pressure});
    }
    return out;
}

} // namespace actcraft::core
