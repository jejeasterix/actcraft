#pragma once

#include <actcraft/tools/tool.h>
#include <actcraft/tools/toolcontext.h>

namespace actcraft::tools {

// Outils Texte et Texte de haut en bas (F-250, F-301 b) : un appui demande l'édition au point touché par la
// commande « text.edit-at » ({x, y, vertical}) ; l'édition elle-même est dans ui::TextEditing.
class TextTool final : public Tool
{
public:
    TextTool(ToolContext& context, bool vertical);
    QString id() const override; // « text » ou « text-vertical »
    void onSample(const input::Sample& sample) override;
    bool isBusy() const noexcept override;
    void cancel() override;
    void deactivate() override; // valide l'édition en cours (F-252) : commande « text.edit-at » sans argument

private:
    ToolContext& m_context;
    bool m_vertical;
};

} // namespace actcraft::tools
