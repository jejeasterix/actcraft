#include <actcraft/tools/revealinktool.h>

namespace actcraft::tools {

RevealInkTool::RevealInkTool(InkSurface& ink, DocumentSink& document)
    : m_pen(ink, document, core::StrokeKind::Revealer)
{
    m_pen.setColor(kPreviewColor); // couleur de l'aperçu seulement : la découpe ne lit pas la couleur
    m_pen.setBaseWidth(kDefaultWidth);
}

double RevealInkTool::baseWidth() const noexcept
{
    return m_pen.baseWidth();
}

void RevealInkTool::setBaseWidth(double width) noexcept
{
    m_pen.setBaseWidth(width);
}

bool RevealInkTool::isDrawing() const noexcept
{
    return m_pen.isDrawing();
}

QString RevealInkTool::id() const
{
    return QStringLiteral("reveal-ink");
}

bool RevealInkTool::isBusy() const noexcept
{
    return m_pen.isBusy();
}

void RevealInkTool::cancel()
{
    m_pen.cancel();
}

void RevealInkTool::abandon()
{
    m_pen.abandon();
}

void RevealInkTool::onSample(const input::Sample& sample)
{
    m_pen.onSample(sample);
}

} // namespace actcraft::tools
