#pragma once

#include <actcraft/core/document.h>
#include <actcraft/core/edits.h>
#include <actcraft/core/model.h>
#include <actcraft/core/objectactions.h>
#include <actcraft/core/undo.h>

#include <QByteArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QString>

#include <cmath>
#include <cstddef>
#include <memory>
#include <optional>
#include <string>

// Aides communes aux deux moteurs d'actions d'objet (objectactions_geometry.cpp, objectactions_edit.cpp) :
// lecture des paramètres JSON et recherche de la cible. Privé à core/src.
namespace actcraft::core::detail {

// Nombre fini du paramètre `name` ; rien s'il manque, n'est pas un nombre ou n'est pas fini.
inline std::optional<double> number(const QJsonObject& parameters, const char* name)
{
    const QJsonValue v = parameters.value(QString::fromLatin1(name));
    if (!v.isDouble() || !std::isfinite(v.toDouble())) {
        return std::nullopt;
    }
    return v.toDouble();
}

// Paramètres (objet JSON compact) : objet vide si la chaîne est vide ; rien si ce n'est pas un objet.
inline std::optional<QJsonObject> parseParameters(const std::string& parameters)
{
    if (parameters.empty()) {
        return QJsonObject{};
    }
    const QJsonDocument json = QJsonDocument::fromJson(QByteArray::fromStdString(parameters));
    if (!json.isObject()) {
        return std::nullopt;
    }
    return json.object();
}

// Indice de la cible de l'action (`target`, à défaut `source`) sur la page du contexte ; page.objects.size()
// si elle est absente.
inline std::size_t targetIndex(const Page& page, const ActionContext& context)
{
    const ObjectId targetId = context.target != 0 ? context.target : context.source;
    const std::size_t index = indexOf(page, targetId);
    return targetId == 0 || index >= page.objects.size() ? page.objects.size() : index;
}

// Définie dans objectactions_edit.cpp, appelée par objectActionCommand (objectactions_geometry.cpp).
std::unique_ptr<UndoCommand> editActionCommand(const Document& document, const std::string& id,
                                               const std::string& parameters, const ActionContext& context);

} // namespace actcraft::core::detail
