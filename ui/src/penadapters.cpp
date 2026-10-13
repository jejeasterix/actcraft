#include "penadapters.h"

#include <actcraft/ui/pageview.h>

#include <utility>

namespace actcraft::ui {

InkSurfaceAdapter::InkSurfaceAdapter(render::InkItem& ink) : m_ink(ink) {}

void InkSurfaceAdapter::beginStroke(core::Argb color, double baseWidth)
{
    m_ink.beginStroke(color, baseWidth);
}

void InkSurfaceAdapter::addPoint(const core::StrokePoint& point, std::uint64_t seq)
{
    m_ink.addPoint(point, seq);
}

void InkSurfaceAdapter::clear()
{
    m_ink.clear();
}

FixedPageSink::FixedPageSink(render::FixedPageItem& page, PageView& view) : m_page(page), m_view(view) {}

void FixedPageSink::commitStroke(core::Stroke stroke)
{
    m_page.appendStroke(std::move(stroke));
    m_view.onStrokeFrozen(); // cache redessiné dans le même tour de boucle que le vidage de l'encre
}

} // namespace actcraft::ui
