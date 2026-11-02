#include <actcraft/tools/pentool.h>

#include <utility>

namespace actcraft::tools {

namespace {

// Couleur de l'encre vive : l'élément d'encre n'a pas d'opacité propre, la translucidité passe dans l'alpha.
// Même calcul que le rendu d'un trait versé (alpha de la couleur × (1 − t / 255)) : 255 − t pour une couleur
// opaque.
core::Argb inkColor(core::Argb color, std::uint8_t translucency) noexcept
{
    const std::uint32_t alpha = color >> 24;
    const std::uint32_t opacity = 255u - static_cast<std::uint32_t>(translucency);
    const std::uint32_t seen = (alpha * opacity + 127u) / 255u;
    return (seen << 24) | (color & 0x00FFFFFFu);
}

} // namespace

PenTool::PenTool(InkSurface& ink, DocumentSink& document, core::StrokeKind kind)
    : m_ink(ink), m_document(document), m_kind(kind)
{}

void PenTool::setColor(core::Argb color) noexcept
{
    m_color = color;
}

void PenTool::setBaseWidth(double width) noexcept
{
    m_baseWidth = width;
}

core::Argb PenTool::color() const noexcept
{
    return m_color;
}

double PenTool::baseWidth() const noexcept
{
    return m_baseWidth;
}

bool PenTool::isDrawing() const noexcept
{
    return m_drawing;
}

void PenTool::setTranslucency(std::uint8_t translucency) noexcept
{
    m_translucency = translucency;
}

std::uint8_t PenTool::translucency() const noexcept
{
    return m_translucency;
}

core::StrokeKind PenTool::kind() const noexcept
{
    return m_kind;
}

QString PenTool::id() const
{
    return m_kind == core::StrokeKind::Highlighter ? QStringLiteral("highlighter") : QStringLiteral("pen");
}

bool PenTool::isBusy() const noexcept
{
    return m_drawing;
}

void PenTool::cancel()
{
    if (m_drawing) {
        finish(); // versé tel quel (2 points au moins), encre vidée
    }
}

void PenTool::abandon()
{
    if (!m_drawing) {
        return;
    }
    m_drawing = false;
    m_current = core::Stroke{};
    m_ink.clear();
}

void PenTool::onSample(const input::Sample& sample)
{
    switch (sample.phase) {
    case input::Phase::Press:
        if (m_drawing) {
            finish();
        }
        m_current = core::Stroke{};
        m_current.baseWidth = m_baseWidth;
        m_current.color = m_color; // opaque : la translucidité va à part (commitAnnotation)
        m_current.kind = m_kind;
        m_current.cap = core::PenCap::Round; // D2
        m_currentTranslucency = m_translucency;
        m_drawing = true;
        m_ink.beginStroke(inkColor(m_color, m_currentTranslucency), m_baseWidth);
        addPoint(sample);
        break;
    case input::Phase::Move:
        if (m_drawing) {
            addPoint(sample);
        }
        break;
    case input::Phase::Release:
        if (!m_drawing) {
            break;
        }
        if (const core::StrokePoint& last = m_current.points.back();
            last.x != sample.x || last.y != sample.y) {
            addPoint(sample);
        }
        finish();
        break;
    }
}

void PenTool::addPoint(const input::Sample& sample)
{
    const core::StrokePoint point{sample.x, sample.y, sample.pressure};
    m_current.points.push_back(point);
    m_ink.addPoint(point, sample.seq);
}

void PenTool::finish()
{
    m_drawing = false;
    if (m_current.points.size() >= 2) {
        m_document.commitAnnotation(std::move(m_current), m_currentTranslucency);
    }
    m_current = core::Stroke{};
    m_ink.clear();
}

} // namespace actcraft::tools
