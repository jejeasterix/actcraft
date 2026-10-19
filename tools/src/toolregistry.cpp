#include <actcraft/tools/erasertool.h>
#include <actcraft/tools/pentool.h>
#include <actcraft/tools/selecttool.h>
#include <actcraft/tools/toolregistry.h>
#include <actcraft/tools/userstate.h>

#include <algorithm>
#include <utility>

namespace actcraft::tools {

namespace {

// Stylo et marqueur : un seul outil d'encre, de deux genres (pas de classe de marqueur à part). Il naît avec
// les réglages de son utilisateur (F-136).
std::unique_ptr<Tool> inkTool(ToolContext& context, core::StrokeKind kind)
{
    auto tool = std::make_unique<PenTool>(context.ink, context.strokes, kind);
    const InkSettings settings = context.user.settings(tool->id());
    tool->setColor(settings.color);
    tool->setBaseWidth(settings.width);
    tool->setTranslucency(settings.translucency);
    return tool;
}

} // namespace

bool ToolRegistry::add(const QString& id, Factory factory)
{
    // Une fabrique vide est refusée aussi : create() ne saurait rien en faire.
    if (id.isEmpty() || !factory || contains(id)) {
        return false;
    }
    m_factories.emplace_back(id, std::move(factory));
    return true;
}

bool ToolRegistry::contains(const QString& id) const
{
    return std::any_of(m_factories.begin(), m_factories.end(),
                       [&id](const std::pair<QString, Factory>& entry) { return entry.first == id; });
}

QStringList ToolRegistry::ids() const
{
    QStringList out;
    out.reserve(static_cast<qsizetype>(m_factories.size()));
    for (const auto& entry : m_factories) {
        out.push_back(entry.first);
    }
    return out;
}

std::unique_ptr<Tool> ToolRegistry::create(const QString& id, ToolContext& context) const
{
    for (const auto& entry : m_factories) {
        if (entry.first == id) {
            return entry.second(context);
        }
    }
    return nullptr;
}

void registerBuiltinTools(ToolRegistry& registry)
{
    registry.add(QStringLiteral("select"),
                 [](ToolContext& context) { return std::make_unique<SelectTool>(context); });
    registry.add(QStringLiteral("pen"),
                 [](ToolContext& context) { return inkTool(context, core::StrokeKind::Pen); });
    registry.add(QStringLiteral("highlighter"),
                 [](ToolContext& context) { return inkTool(context, core::StrokeKind::Highlighter); });
    registry.add(QStringLiteral("eraser"),
                 [](ToolContext& context) { return std::make_unique<EraserTool>(context); });
}

} // namespace actcraft::tools
