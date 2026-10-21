#include <actcraft/tools/shapetool.h>

namespace actcraft::tools {

// Talon L4-T1 : rempli par L4-T8.
ShapeTool::ShapeTool(ToolContext& context) : m_context(context) {}

QString ShapeTool::id() const
{
    return QStringLiteral("shape");
}

void ShapeTool::onSample(const input::Sample&) {}

bool ShapeTool::isBusy() const noexcept
{
    return m_press.has_value();
}

void ShapeTool::cancel()
{
    static_cast<void>(m_context);
    static_cast<void>(m_current);
    static_cast<void>(m_shift);
}

bool ShapeTool::onKey(int, int)
{
    return false;
}

} // namespace actcraft::tools
