#pragma once

#include <actcraft/tools/tool.h>
#include <actcraft/tools/toolcontext.h>

#include <QStringList>

#include <functional>
#include <memory>
#include <utility>
#include <vector>

namespace actcraft::tools {

class ToolRegistry
{
public:
    using Factory = std::function<std::unique_ptr<Tool>(ToolContext& context)>;
    bool add(const QString& id, Factory factory); // faux : identifiant vide ou déjà pris
    bool contains(const QString& id) const;
    QStringList ids() const;                                                     // ordre d'ajout
    std::unique_ptr<Tool> create(const QString& id, ToolContext& context) const; // nullptr : inconnu

private:
    std::vector<std::pair<QString, Factory>> m_factories;
};

// « select », « pen », « highlighter », « eraser », dans cet ordre.
void registerBuiltinTools(ToolRegistry& registry);

} // namespace actcraft::tools
