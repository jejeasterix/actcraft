#include <actcraft/render/inkbuffer.h>

#include <algorithm>

namespace actcraft::render {

namespace {
void extend(double& minX, double& minY, double& maxX, double& maxY, const core::StrokePoint& p)
{
    minX = std::min(minX, p.x);
    minY = std::min(minY, p.y);
    maxX = std::max(maxX, p.x);
    maxY = std::max(maxY, p.y);
}
} // namespace

void InkBuffer::begin(core::Argb color, double baseWidth)
{
    clear();
    m_color = color;
    m_baseWidth = baseWidth;
}

void InkBuffer::add(const core::StrokePoint& point)
{
    if (m_blocks.empty() || m_blocks.back().size() == kBlockPoints) {
        std::vector<core::StrokePoint> block;
        block.reserve(kBlockPoints);
        Extent extent{point.x, point.y, point.x, point.y, 0.0};
        if (!m_blocks.empty()) {
            const core::StrokePoint joint = m_blocks.back().back(); // jonction partagée
            block.push_back(joint);
            extent = {joint.x, joint.y, joint.x, joint.y,
                      core::strokeWidthAt(m_baseWidth, joint.pressure) / 2};
        }
        m_blocks.push_back(std::move(block));
        m_extents.push_back(extent);
    }
    m_blocks.back().push_back(point);
    Extent& e = m_extents.back();
    extend(e.minX, e.minY, e.maxX, e.maxY, point);
    e.maxHalf = std::max(e.maxHalf, core::strokeWidthAt(m_baseWidth, point.pressure) / 2);
    ++m_points;
    m_firstDirty = std::min(m_firstDirty, m_blocks.size() - 1);
}

void InkBuffer::clear() noexcept
{
    if (!m_blocks.empty()) {
        m_cleared = true;
    }
    m_blocks.clear();
    m_extents.clear();
    m_points = 0;
    m_firstDirty = 0;
}

bool InkBuffer::empty() const noexcept
{
    return m_points == 0;
}

std::size_t InkBuffer::pointCount() const noexcept
{
    return m_points;
}

std::size_t InkBuffer::blockCount() const noexcept
{
    return m_blocks.size();
}

std::span<const core::StrokePoint> InkBuffer::block(std::size_t index) const
{
    return m_blocks[index];
}

core::RectF InkBuffer::blockBounds(std::size_t index) const
{
    const Extent& e = m_extents[index];
    return {e.minX - e.maxHalf, e.minY - e.maxHalf, e.maxX - e.minX + 2 * e.maxHalf,
            e.maxY - e.minY + 2 * e.maxHalf};
}

std::size_t InkBuffer::firstDirtyBlock() const noexcept
{
    return std::min(m_firstDirty, m_blocks.size());
}

bool InkBuffer::clearedSinceClean() const noexcept
{
    return m_cleared;
}

void InkBuffer::markClean() noexcept
{
    m_firstDirty = m_blocks.size();
    m_cleared = false;
}

core::Argb InkBuffer::color() const noexcept
{
    return m_color;
}

double InkBuffer::baseWidth() const noexcept
{
    return m_baseWidth;
}

} // namespace actcraft::render
