#include <actcraft/tools/texttool.h>

namespace actcraft::tools {

// Talon L4-T1 : rempli par L4-T22.
TextTool::TextTool(ToolContext& context, bool vertical) : m_context(context), m_vertical(vertical) {}

QString TextTool::id() const
{
    return m_vertical ? QStringLiteral("text-vertical") : QStringLiteral("text");
}

void TextTool::onSample(const input::Sample&) {}

bool TextTool::isBusy() const noexcept
{
    return false;
}

void TextTool::cancel()
{
    static_cast<void>(m_context);
}

void TextTool::deactivate() {}

} // namespace actcraft::tools
