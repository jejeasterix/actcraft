#include <actcraft/tools/texttool.h>

#include <QVariant>

namespace actcraft::tools {

TextTool::TextTool(ToolContext& context, bool vertical) : m_context(context), m_vertical(vertical) {}

QString TextTool::id() const
{
    return m_vertical ? QStringLiteral("text-vertical") : QStringLiteral("text");
}

void TextTool::onSample(const input::Sample& sample)
{
    if (sample.phase != input::Phase::Press) {
        return;
    }
    // Argument texte « x;y;v » : ArgumentKind::Text n'accepte qu'une chaîne non vide.
    m_context.commands.trigger(
        QStringLiteral("text.edit-at"),
        QStringLiteral("%1;%2;%3").arg(sample.x, 0, 'f', 2).arg(sample.y, 0, 'f', 2).arg(m_vertical ? 1 : 0));
}

bool TextTool::isBusy() const noexcept
{
    return false; // l'édition vit dans l'interface
}

void TextTool::cancel()
{
    // Rien à terminer : Échap, changement de page et Annuler valident l'édition côté interface.
}

void TextTool::deactivate()
{
    // Argument absent : valide l'édition en cours (F-252).
    m_context.commands.trigger(QStringLiteral("text.edit-at"), QVariant());
}

} // namespace actcraft::tools
