#pragma once

#include <actcraft/tools/tool.h>
#include <actcraft/tools/toolcontext.h>

#include <cstdint>

namespace actcraft::tools {

// Sélection (F-210, F-520 à F-528, F-541, F-050).
class SelectTool final : public Tool
{
public:
    explicit SelectTool(ToolContext& context);
    QString id() const override; // « select »
    void activate() override;
    void deactivate() override; // garde la sélection, masque le cadre
    void onSample(const input::Sample& sample) override;
    bool isBusy() const noexcept override;
    void cancel() override;
    bool onKey(int key, int modifiers) override; // Suppr : supprimer ; Échap : abandon du geste
    void selectAll();
    void clearSelection();
    void deleteSelection(); // sans sélection : rien (F-541)
    void enlarge();         // × kEnlargeStep autour du centre (F-523)
    void reduce();          // ÷ kEnlargeStep
    void setTranslucency(std::uint8_t value);
    void refresh(); // la page a changé (annuler, page suivante) : sélection filtrée, cadre recalculé

private:
    ToolContext& m_context;
};

} // namespace actcraft::tools
