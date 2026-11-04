#pragma once

#include <actcraft/core/model.h>
#include <actcraft/tools/tool.h>
#include <actcraft/tools/toolcontext.h>

#include <cstdint>
#include <optional>

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
    void setCaps(core::ConnectorCap capA, core::ConnectorCap capB); // L5-C18 (D11) : extrémités A et B

private:
    void showAttachPoints(core::PointF point);
    void clearAttachPoints();
    core::PageObject currentConnector(core::PointF point) const;
    bool dragged(core::PointF point) const;

    ToolContext& m_context;
    std::uint8_t m_style = 0;
    core::ConnectorCap m_capA = core::ConnectorCap::None;
    core::ConnectorCap m_capB = core::ConnectorCap::None;
    bool m_active = false;
    core::PointF m_rawPress; // presse brute : seuil de clic
    core::PointF m_start;    // bout de départ, accroché ou libre
    core::ObjectId m_objectA = 0;
    bool m_shownPoints = false;
};

} // namespace actcraft::tools
