#pragma once

#include <actcraft/core/model.h>
#include <actcraft/core/shapegeometry.h>
#include <actcraft/tools/tool.h>
#include <actcraft/tools/toolcontext.h>

#include <cstdint>
#include <optional>
#include <vector>

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
    void abandon() override; // rien n'est versé
    // Maj et Ctrl relayés par ToolHost (lot 3), et le relais d'avant appui : sans effet sur le tracé (D5,
    // L4-O2). Échap n'arrive pas ici : raccourci de tool.select, qui appelle cancel() si l'outil est occupé.
    bool onKey(int key, int modifiers) override;

private:
    bool boxGesture() const;
    bool dragged() const;
    core::ShapeStyle style() const;
    std::optional<core::PageObject> currentObject() const;
    core::PointF snapped(core::PointF point) const;
    void showPreview();
    void onChainSample(const input::Sample& sample);
    bool chainExtentReached() const;
    void showChain(const std::vector<core::PointF>& points);
    void finishChain(); // verse la ligne si elle a deux points distincts, puis remet l'état à zéro
    void resetChain();

    ToolContext& m_context;
    std::optional<core::PointF> m_press; // point brut de l'appui (page)
    core::PointF m_current;
    std::vector<core::PointF> m_chain; // sommets posés (Clicks) ou points du geste (Freehand)
    std::int64_t m_lastPressNs = 0;
};

} // namespace actcraft::tools
