#include <actcraft/core/pagezones.h>

#include <algorithm>
#include <cmath>

namespace actcraft::core {

namespace {

// Vu par l'interface (surcouche des séparations, miniature), ignoré par applyToCache (I3, L0-S3).
Change pageSettingsChanged(std::size_t page) noexcept
{
    return Change{Change::Kind::PageSettingsChanged, page, 0, RectF{}};
}

// 0 : non donnée (négative ou non finie comprise) ; sinon au moins kMinFrameSide (H4-20).
double clampFrameSide(double side) noexcept
{
    if (!std::isfinite(side) || side <= 0) {
        return 0;
    }
    return std::max(side, kMinFrameSide);
}

} // namespace

PageZones clampZones(PageZones zones) noexcept
{
    auto& [horizontal, vertical, frameWidth, frameHeight] = zones;
    horizontal = std::clamp(horizontal, std::int32_t{0}, kMaxFramesAcross);
    vertical = std::clamp(vertical, std::int32_t{0}, kMaxFramesDown);
    frameWidth = clampFrameSide(frameWidth);
    frameHeight = clampFrameSide(frameHeight);
    return zones;
}

SetPageZonesCommand::SetPageZonesCommand(std::size_t page, PageZones zones)
    : m_page(page), m_after(clampZones(zones))
{}

void SetPageZonesCommand::redo(Document& document, Changes& changes)
{
    Page& page = document.page(m_page);
    m_before = page.zones;
    page.zones = m_after;
    changes.push_back(pageSettingsChanged(m_page));
}

void SetPageZonesCommand::undo(Document& document, Changes& changes)
{
    document.page(m_page).zones = m_before;
    changes.push_back(pageSettingsChanged(m_page));
}

} // namespace actcraft::core
