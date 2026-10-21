#pragma once

#include <actcraft/tools/tool.h>
#include <actcraft/tools/toolcontext.h>

#include <optional>

namespace actcraft::tools {

// Outil des formes (F-240, F-241) : aperçu par la couche soulevée, une AddObjectCommand au relâché.
class ShapeTool final : public Tool
{
public:
    explicit ShapeTool(ToolContext& context);
    QString id() const override;
    void onSample(const input::Sample& sample) override;
    bool isBusy() const noexcept override;
    void cancel() override;
    // Maj relayée par ToolHost (M9) : (Qt::Key_Shift, Qt::ShiftModifier) à l'appui, (Qt::Key_Shift,
    // Qt::NoModifier) au relâché ; l'aperçu est refait aussitôt. Échap annule.
    bool onKey(int key, int modifiers) override;

private:
    ToolContext& m_context;
    std::optional<core::PointF> m_press;
    core::PointF m_current;
    bool m_shift = false;
};

} // namespace actcraft::tools
