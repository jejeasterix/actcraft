#include <actcraft/tools/pentool.h>

namespace actcraft::tools {

PenTool::PenTool(InkSurface& ink, DocumentSink& document) : m_ink(ink), m_document(document) {}

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

void PenTool::onSample(const input::Sample& sample)
{
    switch (sample.phase) {
    case input::Phase::Press:
        if (m_drawing) {
            finish();
        }
        m_current = core::Stroke{};
        m_current.baseWidth = m_baseWidth;
        m_current.color = m_color;
        m_drawing = true;
        m_ink.beginStroke(m_color, m_baseWidth);
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
        m_document.commitStroke(std::move(m_current));
    }
    m_current = core::Stroke{};
    m_ink.clear();
}

} // namespace actcraft::tools
