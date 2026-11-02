#pragma once

#include <actcraft/core/model.h>
#include <actcraft/tools/tool.h>
#include <actcraft/tools/toolcontext.h>

#include <cstdint>

namespace actcraft::tools {

// Outil du lot 5 (L5-T13) : membres de l'interface Tool recopiés de selecttool.h (A1).
class ConnectorTool final : public Tool
{
public:
    explicit ConnectorTool(ToolContext& context);
    QString id() const override; // « connector »
    void onSample(const input::Sample& sample) override;
    bool isBusy() const noexcept override;
    void cancel() override;
    void setStyle(std::uint8_t style);

private:
    ToolContext& m_context;
    std::uint8_t m_style = 0;
};

} // namespace actcraft::tools
