#include <actcraft/tools/connectortool.h>

namespace actcraft::tools {

// Talon L5-T1 : rempli par L5-T13.
ConnectorTool::ConnectorTool(ToolContext& context) : m_context(context) {}

// Talon L5-T1 : rempli par L5-T13.
QString ConnectorTool::id() const
{
    return QStringLiteral("connector");
}

// Talon L5-T1 : rempli par L5-T13.
void ConnectorTool::onSample(const input::Sample& sample)
{
    static_cast<void>(sample);
    static_cast<void>(m_context);
}

// Talon L5-T1 : rempli par L5-T13.
bool ConnectorTool::isBusy() const noexcept
{
    return false;
}

// Talon L5-T1 : rempli par L5-T13.
void ConnectorTool::cancel() {}

// Talon L5-T1 : rempli par L5-T13.
void ConnectorTool::setStyle(std::uint8_t style)
{
    m_style = style;
}

} // namespace actcraft::tools
