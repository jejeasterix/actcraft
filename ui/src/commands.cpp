#include <actcraft/ui/commands.h>

#include <QCoreApplication>
#include <QJSEngine>
#include <QStandardPaths>

namespace actcraft::ui {

namespace {

QString defaultProfileDir()
{
    return QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation) +
           QStringLiteral("/disposition");
}

} // namespace

Commands::Commands(QObject* parent)
    : QObject(parent), m_platform(actions::currentPlatform()), m_profileDir(defaultProfileDir())
{
    reload();
}

void Commands::setProfileDir(const QString& dir)
{
    m_profileDir = dir;
    reload();
}

void Commands::setPlatform(actions::Platform platform)
{
    m_platform = platform;
    reload();
}

void Commands::reload()
{
    // L'état coché (outil courant, plein écran) n'est pas dans les fichiers relus : gardé pour les commandes
    // qui existent encore.
    QStringList checked;
    for (const QString& id : m_registry.ids()) {
        if (m_registry.isChecked(id)) {
            checked << id;
        }
    }
    m_registry = actions::CommandRegistry{};
    m_layout = actions::Layout{};
    m_errors.clear();
    QString error;
    if (!m_registry.loadDeclarations(actions::defaultCommandsJson(), error)) {
        m_errors << QStringLiteral("commands.json : %1").arg(error);
    }
    for (const QString& id : std::as_const(checked)) {
        m_registry.setChecked(id, true); // sans effet sur une commande disparue
    }
    const QString resourceDir = actions::dataResourceDir();
    QString used;
    const QByteArray json =
        actions::readLayeredFile(QStringLiteral("layout.json"), m_profileDir, resourceDir, &used);
    if (!actions::parseLayout(json, m_layout, error)) {
        m_errors << QStringLiteral("%1 : %2").arg(used.isEmpty() ? QStringLiteral("layout.json") : used,
                                                  error);
        // Disposition du profil illisible : celle des ressources.
        QString ignored;
        const bool fallback = actions::parseLayout(
            actions::readLayeredFile(QStringLiteral("layout.json"), QString(), resourceDir), m_layout,
            ignored);
        Q_UNUSED(fallback)
    }
    const QStringList unknown = actions::unknownCommands(m_layout, m_registry);
    for (const QString& id : unknown) {
        m_errors << QStringLiteral("commande inconnue dans la disposition : %1").arg(id);
    }
    for (auto it = m_handlers.cbegin(); it != m_handlers.cend(); ++it) {
        bindHandler(it.key());
    }
    for (const QString& message : std::as_const(m_errors)) {
        qWarning("ActCraft : %s", qPrintable(message));
    }
    ++m_revision;
    emit layoutChanged();
    emit stateChanged();
}

void Commands::bindHandler(const QString& id)
{
    m_registry.setHandler(id, [this, id](const QVariant& argument) {
        const QJSValue handler = m_handlers.value(id);
        QJSEngine* engine = qjsEngine(this);
        if (!handler.isCallable() || !engine) {
            return;
        }
        const QJSValue result = handler.call({engine->toScriptValue(argument)});
        if (result.isError()) {
            qWarning("ActCraft : commande %s : %s", qPrintable(id), qPrintable(result.toString()));
        }
    });
}

void Commands::bumpState()
{
    ++m_revision;
    emit stateChanged();
}

QVariantList Commands::menus() const
{
    QVariantList out;
    for (const actions::MenuSpec& menu : m_layout.menus) {
        QVariantList items;
        bool pendingSeparator = false;
        for (const QString& id : menu.items) {
            if (id == QLatin1String("-")) {
                pendingSeparator = !items.isEmpty();
                continue;
            }
            if (!isAvailable(id)) {
                continue;
            }
            if (pendingSeparator) {
                items.append(QVariantMap{{QStringLiteral("separator"), true}});
                pendingSeparator = false;
            }
            const actions::CommandInfo* info = m_registry.find(id);
            items.append(QVariantMap{{QStringLiteral("id"), id},
                                     {QStringLiteral("label"), label(id)},
                                     {QStringLiteral("shortcut"), shortcut(id)},
                                     {QStringLiteral("checkable"), info->checkable},
                                     {QStringLiteral("separator"), false}});
        }
        if (items.isEmpty()) {
            continue; // menu vide (lot 1 : Éditer, Insérer) : pas affiché
        }
        out.append(QVariantMap{{QStringLiteral("id"), menu.id},
                               {QStringLiteral("label"), menu.label},
                               {QStringLiteral("items"), items}});
    }
    return out;
}

QVariantList Commands::palette() const
{
    QVariantList out;
    for (const actions::PaletteGroup& group : m_layout.palette) {
        const bool special =
            group.group == QLatin1String("couleurs") || group.group == QLatin1String("epaisseur");
        QVariantList items;
        for (const QString& id : group.items) {
            if (!isAvailable(id)) {
                continue;
            }
            items.append(QVariantMap{{QStringLiteral("id"), id},
                                     {QStringLiteral("label"), label(id)},
                                     {QStringLiteral("icon"), iconUrl(id)},
                                     {QStringLiteral("checkable"), m_registry.find(id)->checkable}});
        }
        if (!special && items.isEmpty()) {
            continue;
        }
        out.append(QVariantMap{{QStringLiteral("group"), group.group}, {QStringLiteral("items"), items}});
    }
    return out;
}

QVariantList Commands::shortcuts() const
{
    QVariantList out;
    for (const QString& id : m_registry.ids()) {
        const QString sequence = shortcut(id);
        if (!isAvailable(id) || sequence.isEmpty()) {
            continue;
        }
        out.append(QVariantMap{{QStringLiteral("id"), id}, {QStringLiteral("sequence"), sequence}});
    }
    return out;
}

QStringList Commands::colors() const
{
    return m_layout.colors;
}

QVariantList Commands::penWidths() const
{
    QVariantList out;
    for (const double width : m_layout.penWidths) {
        out << width;
    }
    return out;
}

QString Commands::platformName() const
{
    return actions::platformName(m_platform);
}

QStringList Commands::errors() const
{
    return m_errors;
}

int Commands::revision() const noexcept
{
    return m_revision;
}

const actions::CommandRegistry& Commands::registry() const noexcept
{
    return m_registry;
}

bool Commands::trigger(const QString& id, const QVariant& argument)
{
    if (!m_registry.trigger(id, argument, m_platform)) {
        return false;
    }
    emit triggered(id, argument);
    return true;
}

void Commands::setHandler(const QString& id, const QJSValue& handler)
{
    if (!m_registry.find(id)) {
        qWarning("ActCraft : gestionnaire pour une commande inconnue : %s", qPrintable(id));
        return;
    }
    m_handlers.insert(id, handler);
    bindHandler(id);
}

QString Commands::label(const QString& id) const
{
    const actions::CommandInfo* info = m_registry.find(id);
    return info ? QCoreApplication::translate("commandes", info->label.toUtf8().constData()) : QString();
}

QString Commands::iconUrl(const QString& id) const
{
    const actions::CommandInfo* info = m_registry.find(id);
    if (!info || info->icon.isEmpty()) {
        return {};
    }
    return QStringLiteral("qrc:/qt/qml/ActCraft/Ui/icons/%1.svg").arg(info->icon);
}

QString Commands::shortcut(const QString& id) const
{
    const auto it = m_layout.shortcuts.constFind(id);
    if (it != m_layout.shortcuts.cend()) {
        return it.value();
    }
    const actions::CommandInfo* info = m_registry.find(id);
    return info ? info->shortcut : QString();
}

bool Commands::isAvailable(const QString& id) const
{
    return m_registry.isAvailable(id, m_platform);
}

bool Commands::isEnabled(const QString& id) const
{
    return m_registry.isEnabled(id);
}

void Commands::setEnabled(const QString& id, bool enabled)
{
    if (!m_registry.find(id) || m_registry.isEnabled(id) == enabled) {
        return;
    }
    m_registry.setEnabled(id, enabled);
    bumpState();
}

bool Commands::isChecked(const QString& id) const
{
    return m_registry.isChecked(id);
}

void Commands::setChecked(const QString& id, bool checked)
{
    if (!m_registry.find(id) || m_registry.isChecked(id) == checked) {
        return;
    }
    m_registry.setChecked(id, checked);
    bumpState();
}

} // namespace actcraft::ui
