#include <actcraft/tools/filltool.h>

namespace actcraft::tools {

// Talon L5-T1 : rempli par L5-T12.
FillTool::FillTool(ToolContext& context) : m_context(context) {}

// Talon L5-T1 : rempli par L5-T12.
QString FillTool::id() const
{
    return QStringLiteral("fill");
}

// Talon L5-T1 : rempli par L5-T12.
void FillTool::onSample(const input::Sample& sample)
{
    static_cast<void>(sample);
    static_cast<void>(m_context);
}

// Talon L5-T1 : rempli par L5-T12.
bool FillTool::isBusy() const noexcept
{
    return false;
}

// Talon L5-T1 : rempli par L5-T12.
void FillTool::cancel() {}

// Talon L5-T1 : rempli par L5-T12.
void FillTool::setColor(core::Argb color)
{
    m_color = color;
}

FillTool::~FillTool() {}
void FillTool::setComputeObserver(std::function<void(QThread*)>) {}
int FillTool::staleImageFillDropped() const noexcept { return 0; }
} // namespace actcraft::tools
