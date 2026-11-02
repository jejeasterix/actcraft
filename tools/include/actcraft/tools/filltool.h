#pragma once

#include <actcraft/core/model.h>
#include <actcraft/tools/tool.h>
#include <actcraft/tools/toolcontext.h>

#include <cstdint>

namespace actcraft::tools {

// Outil du lot 5 (L5-T12) : membres de l'interface Tool recopiés de selecttool.h (A1).
class FillTool final : public Tool
{
public:
    explicit FillTool(ToolContext& context);
    QString id() const override; // « fill »
    void onSample(const input::Sample& sample) override;
    bool isBusy() const noexcept override;
    void cancel() override;
    void setColor(core::Argb color);

private:
    ToolContext& m_context;
    core::Argb m_color = 0xFF000000u;
};

} // namespace actcraft::tools
