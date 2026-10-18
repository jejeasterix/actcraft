#include <actcraft/tools/toolregistry.h>

// Talon L0-S : rempli par L3-T6.

namespace actcraft::tools {

bool ToolRegistry::add(const QString&, Factory)
{
    return false;
}

bool ToolRegistry::contains(const QString&) const
{
    return false;
}

QStringList ToolRegistry::ids() const
{
    return {};
}

std::unique_ptr<Tool> ToolRegistry::create(const QString&, ToolContext&) const
{
    return nullptr;
}

void registerBuiltinTools(ToolRegistry&) {}

} // namespace actcraft::tools
