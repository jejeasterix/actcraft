#include <actcraft/tools/selecttool.h>

// Talon L0-S : rempli par L3-T8.

namespace actcraft::tools {

SelectTool::SelectTool(ToolContext& context) : m_context(context)
{
    static_cast<void>(m_context);
}

QString SelectTool::id() const
{
    return {};
}

void SelectTool::activate() {}

void SelectTool::deactivate() {}

void SelectTool::onSample(const input::Sample&) {}

bool SelectTool::isBusy() const noexcept
{
    return false;
}

void SelectTool::cancel() {}

bool SelectTool::onKey(int, int)
{
    return false;
}

void SelectTool::selectAll() {}

void SelectTool::clearSelection() {}

void SelectTool::deleteSelection() {}

void SelectTool::enlarge() {}

void SelectTool::reduce() {}

void SelectTool::setTranslucency(std::uint8_t) {}

void SelectTool::refresh() {}

} // namespace actcraft::tools
