#pragma once

#include <actcraft/core/model.h>
#include <actcraft/input/sample.h>
#include <actcraft/tools/tool.h>

#include <cstdint>
#include <utility>

namespace actcraft::tools {

// Ce que le stylo voit du trait en cours (implémenté par ui/ sur render::InkItem).
class InkSurface
{
public:
    virtual ~InkSurface() = default;
    virtual void beginStroke(core::Argb color, double baseWidth) = 0;
    virtual void addPoint(const core::StrokePoint& point, std::uint64_t seq) = 0;
    virtual void clear() = 0;
};

// Ce que le stylo voit du document (implémenté par ui/ sur render::FixedPageItem).
class DocumentSink
{
public:
    virtual ~DocumentSink() = default;
    virtual void commitStroke(core::Stroke stroke) = 0;
    // Lot 3 : trait d'un outil d'encre avec sa translucidité (marqueur, F-225 ; 0 opaque … 255 invisible). Le
    // trait garde sa couleur opaque : la translucidité va à part, comme celle d'un trait lu
    // (common.translucency). Par défaut : commitStroke, la translucidité est perdue.
    virtual void commitAnnotation(core::Stroke stroke, std::uint8_t translucency)
    {
        static_cast<void>(translucency);
        commitStroke(std::move(stroke));
    }
};

// Stylo : appui → trait en cours ; déplacements → points ; relâché → trait versé au document puis encre
// vidée, dans le même tour de boucle (décision 10). Un trait de moins de 2 points n'est pas versé. Outil
// « pen » (socle L0-S7) : occupé pendant un trait ; cancel() verse le trait en cours.
// Le même outil sert de marqueur (genre core::StrokeKind::Highlighter, outil « highlighter ») : un seul
// chemin d'encre. Nouveaux traits à bouts ronds (D2). Couleur et translucidité d'un trait : celles de son
// appui.
class PenTool final : public Tool
{
public:
    PenTool(InkSurface& ink, DocumentSink& document, core::StrokeKind kind = core::StrokeKind::Pen);

    void setColor(core::Argb color) noexcept;
    void setBaseWidth(double width) noexcept;
    core::Argb color() const noexcept;
    double baseWidth() const noexcept;
    bool isDrawing() const noexcept;
    void setTranslucency(std::uint8_t translucency) noexcept; // 0 opaque … 255 invisible
    std::uint8_t translucency() const noexcept;
    core::StrokeKind kind() const noexcept;

    QString id() const override; // « pen » ou « highlighter »
    bool isBusy() const noexcept override;
    void cancel() override;
    void abandon() override; // trait en cours oublié, encre vidée, rien de versé
    void onSample(const input::Sample& sample) override;

private:
    void addPoint(const input::Sample& sample);
    void finish();

    InkSurface& m_ink;
    DocumentSink& m_document;
    core::StrokeKind m_kind;
    core::Argb m_color = 0xFF000000u;
    double m_baseWidth = 3.0; // N2-2
    std::uint8_t m_translucency = 0;
    core::Stroke m_current;
    std::uint8_t m_currentTranslucency = 0; // celle du trait en cours, prise à l'appui
    bool m_drawing = false;
};

} // namespace actcraft::tools
