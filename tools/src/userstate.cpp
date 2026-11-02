#include <actcraft/tools/userstate.h>

#include <utility>

namespace actcraft::tools {

// D3, D4, D5 (docs/verifications/2026-10-18-lot3-principes.md).
InkSettings defaultSettings(const QString& toolId)
{
    if (toolId == QStringLiteral("highlighter")) {
        // Opacité 128 sur 255 de l'original : translucidité 255 − 128 = 127 (0 opaque … 255 invisible).
        return InkSettings{0xFFFFFF00u, 20, 127};
    }
    if (toolId == QStringLiteral("eraser")) {
        return InkSettings{0xFF000000u, 50, 0};
    }
    return InkSettings{0xFF000000u, 4, 0}; // stylo, et tout autre outil
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

void UserState::setCurrentTool(const QString& toolId)
{
    m_tool = toolId;
}

InkSettings UserState::settings(const QString& toolId) const
{
    const auto found = m_settings.find(toolId);
    return found != m_settings.end() ? found->second : defaultSettings(toolId);
}

void UserState::setSettings(const QString& toolId, const InkSettings& settings)
{
    m_settings.insert_or_assign(toolId, settings);
}

const std::vector<core::ObjectId>& UserState::selection() const noexcept
{
    return m_selection;
}

void UserState::setSelection(std::vector<core::ObjectId> ids)
{
    m_selection = std::move(ids);
}

const std::vector<QString>& UserState::openMathTools() const noexcept
{
    return m_mathTools;
}

void UserState::setOpenMathTools(std::vector<QString> ids)
{
    m_mathTools = std::move(ids);
}

ShapeSettings UserState::shapeSettings() const
{
    return m_shape;
}

void UserState::setShapeSettings(const ShapeSettings& settings)
{
    m_shape = settings;
}

TextSettings UserState::textSettings() const
{
    return m_text;
}

void UserState::setTextSettings(const TextSettings& settings)
{
    m_text = settings;
}

} // namespace actcraft::tools
