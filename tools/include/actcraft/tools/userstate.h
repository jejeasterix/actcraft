#pragma once

#include <actcraft/core/model.h>
#include <actcraft/input/routing.h>

#include <QString>

#include <cstdint>
#include <map>
#include <vector>

namespace actcraft::tools {

struct InkSettings
{
    core::Argb color = 0xFF000000u;
    double width = 4;
    std::uint8_t translucency = 0; // 0 opaque … 255 invisible
};

struct ShapeSettings
{
    QString preset = QStringLiteral("line"); // D1 : défaut de la palette
    bool filled = true;                      // E1
    core::Argb fill = 0xFFFFAA00u;           // D2 : orange
    bool fromCenter = false;                 // F-241 : depuis le coin par défaut
};

struct TextSettings
{
    QString fontFamily; // vide : police par défaut de l'application (N2-3, D7)
    double pointSize = 14;
    core::Argb color = 0xFF000000u;
};

// Réglages par défaut (D5) : « pen » noir, 4, opaque ; « highlighter » #FFFF00, 20, 127 ; « eraser » 50 ;
// autre outil : ceux du stylo.
InkSettings defaultSettings(const QString& toolId);

// Un utilisateur (DA-A1) : outil courant, réglages par outil (F-136), sélection, outils de maths ouverts (lot
// 7).
class UserState
{
public:
    explicit UserState(input::UserId id = input::kFirstUser);
    input::UserId id() const noexcept;
    const QString& currentTool() const noexcept; // « pen » au départ
    void setCurrentTool(const QString& toolId);
    InkSettings settings(const QString& toolId) const; // jamais réglé : defaultSettings
    void setSettings(const QString& toolId, const InkSettings& settings);
    const std::vector<core::ObjectId>& selection() const noexcept;
    void setSelection(std::vector<core::ObjectId> ids);
    const std::vector<QString>& openMathTools() const noexcept; // réservé au lot 7
    void setOpenMathTools(std::vector<QString> ids);
    ShapeSettings shapeSettings() const;
    void setShapeSettings(const ShapeSettings& settings);
    TextSettings textSettings() const;
    void setTextSettings(const TextSettings& settings);

private:
    input::UserId m_id;
    QString m_tool = QStringLiteral("pen");
    std::map<QString, InkSettings> m_settings;
    std::vector<core::ObjectId> m_selection;
    std::vector<QString> m_mathTools;
    ShapeSettings m_shape;
    TextSettings m_text;
};

} // namespace actcraft::tools
