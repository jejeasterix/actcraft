#pragma once

#include <actcraft/tools/tool.h>
#include <actcraft/tools/toolcontext.h>

#include <vector>

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
    // Un trait soulevé pendant le geste : copie d'origine et morceaux courants (points de page).
    struct Lifted
    {
        core::ObjectId id = 0;
        core::PageObject original;
        std::vector<std::vector<core::StrokePoint>> pieces;
        bool touched = false; // le disque a retiré une partie du trait
    };

    void begin(const input::Sample& sample);
    void sweep(core::PointF from, core::PointF to, bool first = false);
    void finish();
    void reset();
    std::vector<core::PageObject> pieceObjects(const Lifted& lifted) const;
    void showLifted();

    ToolContext& m_context;
    bool m_active = false;
    core::PointF m_last;
    std::vector<Lifted> m_lifted;
};

} // namespace actcraft::tools
