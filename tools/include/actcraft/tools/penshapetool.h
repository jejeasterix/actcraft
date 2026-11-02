#pragma once

#include <actcraft/core/model.h>
#include <actcraft/core/penmodifiers.h>
#include <actcraft/tools/tool.h>
#include <actcraft/tools/toolcontext.h>

#include <cstdint>

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
    void setShape(core::PenShape shape);
    void setPen(const core::Outline& pen, bool highlighter);

private:
    ToolContext& m_context;
    core::PenShape m_shape = core::PenShape::Free;
    core::Outline m_pen;
    bool m_highlighter = false;
};

} // namespace actcraft::tools
