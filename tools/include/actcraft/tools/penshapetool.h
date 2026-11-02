#pragma once

#include <actcraft/core/model.h>
#include <actcraft/core/penmodifiers.h>
#include <actcraft/tools/tool.h>
#include <actcraft/tools/toolcontext.h>

#include <cstdint>
#include <vector>

namespace actcraft::tools {

// Outil du lot 5 (L5-T12) : membres de l'interface Tool recopiés de selecttool.h (A1).
class PenShapeTool final : public Tool
{
public:
    explicit PenShapeTool(ToolContext& context);
    QString id() const override; // « pen-shape »
    void onSample(const input::Sample& sample) override;
    bool isBusy() const noexcept override;
    void cancel() override;
    bool onKey(int key, int modifiers) override; // Entrée termine la ligne brisée, Échap l'abandonne
    void setShape(core::PenShape shape);
    void setPen(const core::Outline& pen, bool highlighter);

private:
    bool dragged(core::PointF point) const;
    void showPreview(const std::vector<core::PointF>& points);
    void commit(const std::vector<core::PointF>& points);
    void reset();

    ToolContext& m_context;
    core::PenShape m_shape = core::PenShape::Free;
    core::Outline m_pen;
    bool m_highlighter = false;
    bool m_active = false;
    core::PointF m_press;
    std::vector<core::PointF> m_clicks; // sommets de la ligne brisée
    std::int64_t m_lastClickNs = 0;
};

} // namespace actcraft::tools
