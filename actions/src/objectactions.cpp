#include <actcraft/actions/commandregistry.h>
#include <actcraft/actions/objectactions.h>

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSet>

namespace actcraft::actions {

namespace {

bool categoryFromName(const QString& name, ActionCategory& out)
{
    if (name == QLatin1String("command"))
        out = ActionCategory::Command;
    else if (name == QLatin1String("page"))
        out = ActionCategory::Page;
    else if (name == QLatin1String("object"))
        out = ActionCategory::Object;
    else if (name == QLatin1String("document"))
        out = ActionCategory::Document;
    else
        return false;
    return true;
}

} // namespace

bool ObjectActionCatalog::load(const QByteArray& json, QString& error)
{
    QJsonParseError parse;
    const QJsonDocument doc = QJsonDocument::fromJson(json, &parse);
    if (!doc.isObject()) {
        error = QStringLiteral("catalogue des actions : JSON illisible (%1)").arg(parse.errorString());
        return false;
    }
    const QJsonObject root = doc.object();
    if (root.value(QStringLiteral("format")).toString() != QLatin1String("actcraft-object-actions/1")) {
        error = QStringLiteral("catalogue des actions : format inconnu");
        return false;
    }
    QList<ObjectActionInfo> actions;
    QSet<QString> seen;
    const QJsonArray array = root.value(QStringLiteral("actions")).toArray();
    for (qsizetype n = 0; n < array.size(); ++n) {
        const QJsonObject o = array.at(n).toObject();
        ObjectActionInfo info;
        info.id = o.value(QStringLiteral("id")).toString();
        const QString where = QStringLiteral("action %1 (%2) : ").arg(n).arg(info.id);
        if (info.id.isEmpty()) {
            error = where + QStringLiteral("identifiant manquant");
            return false;
        }
        if (seen.contains(info.id)) {
            error = where + QStringLiteral("identifiant en double");
            return false;
        }
        if (!categoryFromName(o.value(QStringLiteral("category")).toString(), info.category)) {
            error = where + QStringLiteral("catégorie inconnue");
            return false;
        }
        seen.insert(info.id);
        info.label = o.value(QStringLiteral("label")).toString();
        info.icon = o.value(QStringLiteral("icon")).toString();
        info.command = o.value(QStringLiteral("command")).toString();
        info.target = o.value(QStringLiteral("target")).toBool();
        info.dragDrop = o.value(QStringLiteral("drag-drop")).toBool();
        for (const QJsonValue& v : o.value(QStringLiteral("parameters")).toArray()) {
            const QJsonObject p = v.toObject();
            ActionParameter param;
            param.name = p.value(QStringLiteral("name")).toString();
            param.type = p.value(QStringLiteral("type")).toString();
            param.minimum = p.value(QStringLiteral("minimum")).toDouble();
            param.maximum = p.value(QStringLiteral("maximum")).toDouble();
            param.exclusiveMinimum = p.value(QStringLiteral("exclusive-minimum")).toBool();
            param.hasDefault = p.contains(QStringLiteral("default"));
            param.defaultValue = p.value(QStringLiteral("default")).toDouble();
            param.decimals = p.value(QStringLiteral("decimals")).toInt();
            param.step = p.contains(QStringLiteral("step")) ? p.value(QStringLiteral("step")).toDouble() : 1.0;
            info.parameters.append(param);
        }
        actions.append(info);
    }
    m_actions = actions;
    return true;
}

const ObjectActionInfo* ObjectActionCatalog::find(const QString& id) const
{
    for (const ObjectActionInfo& a : m_actions)
        if (a.id == id)
            return &a;
    return nullptr;
}

QList<const ObjectActionInfo*> ObjectActionCatalog::inCategory(ActionCategory category) const
{
    QList<const ObjectActionInfo*> out;
    for (const ObjectActionInfo& a : m_actions)
        if (a.category == category)
            out.append(&a);
    return out;
}

QList<const ObjectActionInfo*> ObjectActionCatalog::dragDropList() const
{
    QList<const ObjectActionInfo*> out;
    for (const ObjectActionInfo& a : m_actions)
        if (a.dragDrop)
            out.append(&a);
    return out;
}

QByteArray defaultObjectActionsJson()
{
    QFile file(dataResourceDir() + QStringLiteral("/object-actions.json"));
    return file.open(QIODevice::ReadOnly) ? file.readAll() : QByteArray();
}

} // namespace actcraft::actions
