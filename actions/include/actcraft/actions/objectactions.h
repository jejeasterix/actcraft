#pragma once
#include <QByteArray>
#include <QList>
#include <QString>

namespace actcraft::actions {

enum class ActionCategory
{
    Command,
    Page,
    Object,
    Document
};

struct ActionParameter
{
    QString name; // clé dans l'objet JSON des paramètres de core::Action
    QString type; // "number", "angle", "text", "url", "file", "page", "translucency"
    double minimum = 0;
    double maximum = 0;            // 0 : sans borne
    bool exclusiveMinimum = false; // vrai : la valeur doit dépasser `minimum` (un facteur est > 0)
    bool hasDefault = false;       // vrai : `defaultValue` remplace `minimum` comme valeur initiale
    double defaultValue = 0;
    int decimals = 0; // 0 : entier ; > 0 : réel, éditeur à `decimals` décimales
    double step = 1;  // pas de l'éditeur
};

struct ObjectActionInfo
{
    QString id; // identifiant à nous, porté par core::Action::catalogId
    ActionCategory category = ActionCategory::Command;
    QString label;
    QString icon;
    QString command;       // commande du registre déclenchée ; vide : exécutée par le moteur de core/
    bool target = false;   // vise un objet (choisi par le sélecteur ; défaut : l'objet cliqué)
    bool dragDrop = false; // proposée dans l'onglet glisser-déposer
    QList<ActionParameter> parameters;
};

class ObjectActionCatalog
{
public:
    bool load(const QByteArray& json, QString& error);
    const ObjectActionInfo* find(const QString& id) const;
    QList<const ObjectActionInfo*> inCategory(ActionCategory category) const;
    QList<const ObjectActionInfo*> dragDropList() const;
    qsizetype size() const noexcept { return m_actions.size(); }

private:
    QList<ObjectActionInfo> m_actions;
};

QByteArray defaultObjectActionsJson(); // actions/data/object-actions.json, ressource intégrée

// Catalogue de la ressource intégrée, chargé une fois (premier appel) et partagé : l'explorateur d'actions,
// les propriétés d'interactivité, les glisser-déposer et l'exécuteur lisent le même.
const ObjectActionCatalog& builtInObjectActionCatalog();

} // namespace actcraft::actions
