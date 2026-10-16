#pragma once

#include <actcraft/render/fixedpageitem.h>
#include <actcraft/render/inkitem.h>
#include <actcraft/tools/pentool.h>

namespace actcraft::ui {

class DocumentController;
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

// Le trait figé va au document par la pile (L2-T15). Le cache est redessiné par
// FixedPageItem::contentChanged, émis par l'insertion dans le même tour de boucle que le vidage de l'encre
// (décision 10) ; la vue ne fait qu'annoncer le trait figé (strokeFrozen).
class ControllerSink final : public tools::DocumentSink
{
public:
    ControllerSink(DocumentController& controller, PageView& view);
    void commitStroke(core::Stroke stroke) override;

private:
    DocumentController& m_controller;
    PageView& m_view;
};

} // namespace actcraft::ui
