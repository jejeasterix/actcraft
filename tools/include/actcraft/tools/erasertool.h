#pragma once

#include <actcraft/tools/tool.h>
#include <actcraft/tools/toolcontext.h>

namespace actcraft::tools {

// Gomme (F-230) : soulève chaque annotation touchée (une fois), la recoupe dans la couche soulevée pendant le
// geste, dépose une core::ReplaceObjectsCommand au relâché.
class EraserTool final : public Tool
{
public:
    explicit EraserTool(ToolContext& context);
    QString id() const override; // « eraser »
    void onSample(const input::Sample& sample) override;
    bool isBusy() const noexcept override;
    void cancel() override;
    double radius() const; // largeur réglée de la gomme / 2, pixels de page

private:
    ToolContext& m_context;
};

} // namespace actcraft::tools
