#include <actcraft/tools/erasertool.h>

// Talon L0-S : rempli par L3-T7.

namespace actcraft::tools {

EraserTool::EraserTool(ToolContext& context) : m_context(context)
{
    static_cast<void>(m_context);
}

QString EraserTool::id() const
{
    return {};
}

void EraserTool::onSample(const input::Sample&) {}

bool EraserTool::isBusy() const noexcept
{
    return false;
}

void EraserTool::cancel() {}

double EraserTool::radius() const
{
    return 0.0;
}

} // namespace actcraft::tools
