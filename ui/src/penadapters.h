#pragma once

#include <actcraft/render/fixedpageitem.h>
#include <actcraft/render/inkitem.h>
#include <actcraft/tools/pentool.h>

namespace actcraft::ui {

class PageView;

// Le stylo (tools/) ne connaît que des interfaces ; ui/ les branche sur render/ (règle de modularité 2).
class InkSurfaceAdapter final : public tools::InkSurface
{
public:
    explicit InkSurfaceAdapter(render::InkItem& ink);
    void beginStroke(core::Argb color, double baseWidth) override;
    void addPoint(const core::StrokePoint& point, std::uint64_t seq) override;
    void clear() override;

private:
    render::InkItem& m_ink;
};

class FixedPageSink final : public tools::DocumentSink
{
public:
    FixedPageSink(render::FixedPageItem& page, PageView& view);
    void commitStroke(core::Stroke stroke) override;

private:
    render::FixedPageItem& m_page;
    PageView& m_view;
};

} // namespace actcraft::ui
