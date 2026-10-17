#include <actcraft/actions/commandregistry.h>

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QSet>
#include <QSysInfo>
#include <QUrl>

#include <cmath>
#include <utility>

// Ressources du module (qt_add_resources « actions_data ») : Q_INIT_RESOURCE hors de tout espace de noms.
static void initActionsResources()
{
    Q_INIT_RESOURCE(actions_data);
}

namespace actcraft::actions {

namespace {

bool parseArgumentKind(const QString& text, ArgumentKind& out)
{
    if (text.isEmpty() || text == QLatin1String("none")) {
        out = ArgumentKind::None;
    } else if (text == QLatin1String("color")) {
        out = ArgumentKind::Color;
    } else if (text == QLatin1String("number")) {
        out = ArgumentKind::Number;
    } else if (text == QLatin1String("url")) {
        out = ArgumentKind::Url;
    } else {
        return false;
    }
    return true;
}

} // namespace

Platforms platformBit(const QString& name) noexcept
{
    if (name == QLatin1String("windows")) {
        return static_cast<Platforms>(Platform::Windows);
    }
    if (name == QLatin1String("android")) {
        return static_cast<Platforms>(Platform::Android);
    }
    if (name == QLatin1String("macos")) {
        return static_cast<Platforms>(Platform::MacOS);
    }
    if (name == QLatin1String("linux")) {
        return static_cast<Platforms>(Platform::Linux);
    }
    return 0;
}

Platform currentPlatform()
{
    const QString type = QSysInfo::productType();
    if (type == QLatin1String("windows")) {
        return Platform::Windows;
    }
    if (type == QLatin1String("android")) {
        return Platform::Android;
    }
    if (type == QLatin1String("macos") || type == QLatin1String("osx")) {
        return Platform::MacOS;
    }
    return Platform::Linux;
}

QString platformName(Platform platform)
{
    switch (platform) {
    case Platform::Windows:
        return QStringLiteral("windows");
    case Platform::Android:
        return QStringLiteral("android");
    case Platform::MacOS:
        return QStringLiteral("macos");
    case Platform::Linux:
        return QStringLiteral("linux");
    }
    return QStringLiteral("linux");
}

bool argumentIsValid(ArgumentKind kind, const QVariant& argument)
{
    switch (kind) {
    case ArgumentKind::None:
        return true;
    case ArgumentKind::Color: {
        static const QRegularExpression pattern(QStringLiteral("^#([0-9A-Fa-f]{6}|[0-9A-Fa-f]{8})$"));
        return argument.typeId() == QMetaType::QString && pattern.match(argument.toString()).hasMatch();
    }
    case ArgumentKind::Number: {
        if (!argument.isValid() || argument.typeId() == QMetaType::Bool) {
            return false;
        }
        bool ok = false;
        const double value = argument.toDouble(&ok);
        return ok && std::isfinite(value);
    }
    case ArgumentKind::Url: {
        if (argument.typeId() == QMetaType::QUrl) {
            const QUrl url = argument.toUrl();
            return url.isValid() && !url.isEmpty();
        }
        if (argument.typeId() != QMetaType::QString) {
            return false;
        }
        const QString text = argument.toString();
        return !text.isEmpty() && QUrl(text).isValid();
    }
    }
    return false;
}

bool CommandRegistry::loadDeclarations(const QByteArray& json, QString& error)
{
    QJsonParseError parseError{};
    const QJsonDocument document = QJsonDocument::fromJson(json, &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        error = QStringLiteral("JSON invalide : %1").arg(parseError.errorString());
        return false;
    }
    const QJsonObject root = document.object();
    if (root.value(QStringLiteral("format")).toString() != QLatin1String("actcraft-commands/1")) {
        error = QStringLiteral("format inconnu (attendu actcraft-commands/1)");
        return false;
    }
    const QJsonArray commands = root.value(QStringLiteral("commands")).toArray();
    std::vector<CommandInfo> parsed;
    QSet<QString> seen;
    for (qsizetype i = 0; i < commands.size(); ++i) {
        const QJsonObject o = commands.at(i).toObject();
        CommandInfo info;
        info.id = o.value(QStringLiteral("id")).toString();
        info.label = o.value(QStringLiteral("label")).toString();
        info.icon = o.value(QStringLiteral("icon")).toString();
        info.shortcut = o.value(QStringLiteral("shortcut")).toString();
        info.checkable = o.value(QStringLiteral("checkable")).toBool(false);
        const QString where = QStringLiteral("commande %1 (%2)").arg(i + 1).arg(info.id);
        if (info.id.isEmpty()) {
            error = QStringLiteral("commande %1 : id manquant").arg(i + 1);
            return false;
        }
        if (info.label.isEmpty()) {
            error = where + QStringLiteral(" : label manquant");
            return false;
        }
        if (seen.contains(info.id) || m_index.contains(info.id)) {
            error = where + QStringLiteral(" : identifiant déjà déclaré");
            return false;
        }
        if (o.contains(QStringLiteral("platforms"))) {
            info.platforms = 0;
            const QJsonArray names = o.value(QStringLiteral("platforms")).toArray();
            for (const QJsonValue& name : names) {
                const Platforms bit = platformBit(name.toString());
                if (bit == 0) {
                    error = where + QStringLiteral(" : plateforme inconnue « %1 »").arg(name.toString());
                    return false;
                }
                info.platforms = static_cast<Platforms>(info.platforms | bit);
            }
            if (info.platforms == 0) {
                error = where + QStringLiteral(" : aucune plateforme");
                return false;
            }
        }
        const QString argument = o.value(QStringLiteral("argument")).toString();
        if (!parseArgumentKind(argument, info.argument)) {
            error = where + QStringLiteral(" : argument inconnu « %1 »").arg(argument);
            return false;
        }
        info.argumentOptional = o.value(QStringLiteral("optional")).toBool(false);
        if (info.argumentOptional && info.argument == ArgumentKind::None) {
            error = where + QStringLiteral(" : argument facultatif sans argument déclaré");
            return false;
        }
        seen.insert(info.id);
        parsed.push_back(std::move(info));
    }
    for (const CommandInfo& info : parsed) {
        declare(info);
    }
    return true;
}

bool CommandRegistry::declare(const CommandInfo& info)
{
    if (info.id.isEmpty() || m_index.contains(info.id)) {
        return false;
    }
    m_index.insert(info.id, m_entries.size());
    Entry e;
    e.info = info;
    m_entries.push_back(std::move(e));
    return true;
}

CommandRegistry::Entry* CommandRegistry::entry(const QString& id)
{
    const auto it = m_index.constFind(id);
    return it == m_index.cend() ? nullptr : &m_entries[it.value()];
}

const CommandRegistry::Entry* CommandRegistry::entry(const QString& id) const
{
    const auto it = m_index.constFind(id);
    return it == m_index.cend() ? nullptr : &m_entries[it.value()];
}

const CommandInfo* CommandRegistry::find(const QString& id) const
{
    const Entry* e = entry(id);
    return e ? &e->info : nullptr;
}

QStringList CommandRegistry::ids() const
{
    QStringList out;
    out.reserve(static_cast<qsizetype>(m_entries.size()));
    for (const Entry& e : m_entries) {
        out << e.info.id;
    }
    return out;
}

bool CommandRegistry::isAvailable(const QString& id, Platform platform) const
{
    const Entry* e = entry(id);
    return e && (e->info.platforms & static_cast<Platforms>(platform)) != 0;
}

void CommandRegistry::setHandler(const QString& id, Handler handler)
{
    if (Entry* e = entry(id)) {
        e->handler = std::move(handler);
    }
}

bool CommandRegistry::hasHandler(const QString& id) const
{
    const Entry* e = entry(id);
    return e && static_cast<bool>(e->handler);
}

void CommandRegistry::setEnabled(const QString& id, bool enabled)
{
    if (Entry* e = entry(id)) {
        e->enabled = enabled;
    }
}

bool CommandRegistry::isEnabled(const QString& id) const
{
    const Entry* e = entry(id);
    return e && e->enabled;
}

void CommandRegistry::setChecked(const QString& id, bool checked)
{
    if (Entry* e = entry(id)) {
        e->checked = checked;
    }
}

bool CommandRegistry::isChecked(const QString& id) const
{
    const Entry* e = entry(id);
    return e && e->checked;
}

bool CommandRegistry::trigger(const QString& id, const QVariant& argument, Platform platform) const
{
    const Entry* e = entry(id);
    if (!e || !isAvailable(id, platform) || !e->enabled || !e->handler ||
        !(argumentIsValid(e->info.argument, argument) || (e->info.argumentOptional && !argument.isValid()))) {
        return false;
    }
    const Handler handler = e->handler; // copie : le gestionnaire peut modifier le registre
    handler(argument);
    return true;
}

QString dataResourceDir()
{
    static const bool initialised = [] {
        initActionsResources();
        return true;
    }();
    Q_UNUSED(initialised)
    return QStringLiteral(":/actcraft/actions");
}

QByteArray defaultCommandsJson()
{
    QFile file(dataResourceDir() + QStringLiteral("/commands.json"));
    return file.open(QIODevice::ReadOnly) ? file.readAll() : QByteArray();
}

} // namespace actcraft::actions
