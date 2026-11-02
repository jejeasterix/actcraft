#include <actcraft/tools/penshapetool.h>

namespace actcraft::tools {

// Talon L5-T1 : rempli par L5-T12.
PenShapeTool::PenShapeTool(ToolContext& context) : m_context(context) {}

// Talon L5-T1 : rempli par L5-T12.
QString PenShapeTool::id() const
{
    return QStringLiteral("pen-shape");
}

// Talon L5-T1 : rempli par L5-T12.
void PenShapeTool::onSample(const input::Sample& sample)
{
    static_cast<void>(sample);
    static_cast<void>(m_context);
}

// Talon L5-T1 : rempli par L5-T12.
bool PenShapeTool::isBusy() const noexcept
{
    return false;
}

// Talon L5-T1 : rempli par L5-T12.
void PenShapeTool::cancel() {}

// Talon L5-T1 : rempli par L5-T12.
void PenShapeTool::setShape(core::PenShape shape)
{
    m_shape = shape;
}

// Talon L5-T1 : rempli par L5-T12.
void PenShapeTool::setPen(const core::Outline& pen, bool highlighter)
{
    m_pen = pen;
    m_highlighter = highlighter;
}

} // namespace actcraft::tools
