#pragma once

#include <actcraft/tools/pentool.h>

namespace actcraft::tools {

// Encre révélatrice (F-255, D8-8), outil « reveal-ink » : les échantillons du stylo, un trait de genre
// core::StrokeKind::Revealer versé sur la couche du dessus (la page le pose comme tout trait d'encre). Le
// trait vivant est un aperçu gris translucide (aucune découpe en direct, H8-4) ; au relâché la page découpe
// les objets du dessus placés sous lui, redessin immédiat (canWait, lot 10).
class RevealInkTool final : public Tool
{
public:
    static constexpr double kDefaultWidth = 50;              // comme la référence
    static constexpr core::Argb kPreviewColor = 0x66808080u; // gris neutre translucide

    RevealInkTool(InkSurface& ink, DocumentSink& document);

    double baseWidth() const noexcept;
    void setBaseWidth(double width) noexcept;
    bool isDrawing() const noexcept;

    QString id() const override; // « reveal-ink »
    bool isBusy() const noexcept override;
    void cancel() override;
    void abandon() override;
    void onSample(const input::Sample& sample) override;

private:
    PenTool m_pen;
};

} // namespace actcraft::tools
