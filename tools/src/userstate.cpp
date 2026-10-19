#include <actcraft/tools/userstate.h>

#include <utility>

// Réel au socle (L0-S6, D-S10) : identifiant et outils de maths ouverts. Le reste : Talon L0-S : rempli par
// L3-T6.
namespace actcraft::tools {

InkSettings defaultSettings(const QString&)
{
    return {};
}

UserState::UserState(input::UserId id) : m_id(id) {}

input::UserId UserState::id() const noexcept
{
    return m_id;
}

const QString& UserState::currentTool() const noexcept
{
    return m_tool;
}

void UserState::setCurrentTool(const QString&) {}

InkSettings UserState::settings(const QString&) const
{
    return {};
}

void UserState::setSettings(const QString&, const InkSettings&)
{
    static_cast<void>(m_settings);
}

const std::vector<core::ObjectId>& UserState::selection() const noexcept
{
    return m_selection;
}

void UserState::setSelection(std::vector<core::ObjectId>) {}

const std::vector<QString>& UserState::openMathTools() const noexcept
{
    return m_mathTools;
}

void UserState::setOpenMathTools(std::vector<QString> ids)
{
    m_mathTools = std::move(ids);
}

} // namespace actcraft::tools
