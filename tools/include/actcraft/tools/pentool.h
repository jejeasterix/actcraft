#pragma once

#include <actcraft/core/model.h>
#include <actcraft/input/sample.h>
#include <actcraft/tools/tool.h>

#include <cstdint>

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
};

// Stylo : appui → trait en cours ; déplacements → points ; relâché → trait versé au document puis encre
// vidée, dans le même tour de boucle (décision 10). Un trait de moins de 2 points n'est pas versé. Outil
// « pen » (socle L0-S7) : occupé pendant un trait ; cancel() verse le trait en cours.
class PenTool final : public Tool
{
public:
    PenTool(InkSurface& ink, DocumentSink& document);

    void setColor(core::Argb color) noexcept;
    void setBaseWidth(double width) noexcept;
    core::Argb color() const noexcept;
    double baseWidth() const noexcept;
    bool isDrawing() const noexcept;

    QString id() const override;
    bool isBusy() const noexcept override;
    void cancel() override;
    void onSample(const input::Sample& sample) override;

private:
    void addPoint(const input::Sample& sample);
    void finish();

    InkSurface& m_ink;
    DocumentSink& m_document;
    core::Argb m_color = 0xFF000000u;
    double m_baseWidth = 3.0; // N2-2
    core::Stroke m_current;
    bool m_drawing = false;
};

} // namespace actcraft::tools
