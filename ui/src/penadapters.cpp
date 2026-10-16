#include "penadapters.h"

#include <actcraft/ui/documentcontroller.h>
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
    m_view.onStrokeFrozen(); // le cache, lui, est redessiné sur contentChanged (insertion)
}

ControllerSink::ControllerSink(DocumentController& controller, PageView& view)
    : m_controller(controller), m_view(view)
{}

void ControllerSink::commitStroke(core::Stroke stroke)
{
    m_controller.commitStroke(std::move(stroke));
    m_view.onStrokeFrozen();
}

} // namespace actcraft::ui
