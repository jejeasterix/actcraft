#include <actcraft/ui/commands.h>

#include <QCoreApplication>
#include <QJSEngine>
#include <QScopeGuard>
#include <QStandardPaths>

#include <algorithm>

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
    // Un gestionnaire peut recharger (activer un profil change le dossier de disposition) : remplacer le
    // registre pendant qu'il s'exécute détruirait la fonction en cours. Relu dès que trigger() rend la main.
    if (m_triggerDepth > 0) {
        m_reloadPending = true;
        return;
    }
    // L'état posé par l'interface n'est pas dans les fichiers relus : coché (outil courant, plein écran),
    // désactivé (Annuler sans historique) et verrou de classe, gardés pour les commandes qui existent encore.
    // L'état propre (isOwnEnabled), pas isEnabled() : celui-ci voit à travers le verrou, et le recopier
    // éteindrait pour de bon une commande verrouillable rechargée sous verrou.
    QStringList checked;
    QStringList disabled;
    for (const QString& id : m_registry.ids()) {
        if (m_registry.isChecked(id)) {
            checked << id;
        }
        if (!m_registry.isOwnEnabled(id)) {
            disabled << id;
        }
    }
    const bool locked = m_registry.isLocked();
    const QSet<QString> restricted = m_registry.restrictedIds();
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
    for (const QString& id : std::as_const(disabled)) {
        if (m_registry.find(id)) {
            m_registry.setEnabled(id, false);
        }
    }
    m_registry.setLocked(locked);
    m_registry.setRestricted(restricted);
    m_registry.setRefusalHandler([this](const QString& id) { emit refusedByRestriction(id); });
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
    for (auto it = m_nativeHandlers.cbegin(); it != m_nativeHandlers.cend(); ++it) {
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
    if (m_triggerDepth > 0) {
        m_reloadPending = true; // reload() relie tous les gestionnaires
        return;
    }
    if (const auto native = m_nativeHandlers.constFind(id); native != m_nativeHandlers.cend()) {
        m_registry.setHandler(id, native.value());
        return;
    }
    if (!m_handlers.contains(id)) {
        m_registry.setHandler(id, {});
        return;
    }
    m_registry.setHandler(id, [this, id](const QVariant& argument) {
        const QJSValue handler = m_handlers.value(id);
        QJSEngine* engine = qjsEngine(this);
        if (!handler.isCallable() || !engine) {
            return;
        }
        const QJSValue result = handler.call({engine->toScriptValue(argument), QJSValue(m_triggerUser)});
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

QVariantList Commands::menuItems(const QStringList& tokens,
                                 const std::vector<actions::MenuSpec>* submenus) const
{
    QVariantList items;
    bool pendingSeparator = false;
    for (const QString& token : tokens) {
        if (token == QLatin1String("-")) {
            pendingSeparator = !items.isEmpty();
            continue;
        }
        QVariantMap entry;
        if (token.startsWith(QLatin1Char('>'))) {
            if (!submenus) {
                continue;
            }
            const QString subId = token.mid(1);
            const auto spec = std::find_if(submenus->cbegin(), submenus->cend(),
                                           [&](const actions::MenuSpec& s) { return s.id == subId; });
            if (spec == submenus->cend()) {
                continue;
            }
            const QVariantList subItems = menuItems(spec->items, nullptr);
            if (subItems.isEmpty()) {
                continue; // aucune commande disponible sur cette plateforme : sous-menu omis
            }
            entry = QVariantMap{{QStringLiteral("submenu"), true},
                                {QStringLiteral("id"), spec->id},
                                {QStringLiteral("label"), spec->label},
                                {QStringLiteral("items"), subItems}};
        } else {
            if (!isAvailable(token)) {
                continue;
            }
            const actions::CommandInfo* info = m_registry.find(token);
            entry = QVariantMap{{QStringLiteral("id"), token},
                                {QStringLiteral("label"), label(token)},
                                {QStringLiteral("shortcut"), shortcut(token)},
                                {QStringLiteral("checkable"), info->checkable},
                                {QStringLiteral("separator"), false}};
        }
        if (pendingSeparator) {
            items.append(QVariantMap{{QStringLiteral("separator"), true}});
            pendingSeparator = false;
        }
        items.append(entry);
    }
    return items;
}

QVariantList Commands::menus() const
{
    QVariantList out;
    for (const actions::MenuSpec& menu : m_layout.menus) {
        const QVariantList items = menuItems(menu.items, &menu.submenus);
        if (items.isEmpty()) {
            continue; // menu vide : pas affiché
        }
        out.append(QVariantMap{{QStringLiteral("id"), menu.id},
                               {QStringLiteral("label"), menu.label},
                               {QStringLiteral("items"), items}});
    }
    return out;
}

QVariantList Commands::objectBar() const
{
    QVariantList out;
    for (const QString& id : m_layout.objectBar) {
        if (!isAvailable(id)) {
            continue;
        }
        out.append(QVariantMap{{QStringLiteral("id"), id},
                               {QStringLiteral("label"), label(id)},
                               {QStringLiteral("icon"), iconUrl(id)}});
    }
    return out;
}

QVariantList Commands::contextMenu() const
{
    return menuItems(m_layout.contextMenu, &m_layout.contextSubmenus);
}

QVariantList Commands::objectMenu() const
{
    return menuItems(m_layout.objectMenu, &m_layout.objectSubmenus);
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
    return trigger(id, argument, m_triggerUser); // L11-C6 : imbriqué, l'utilisateur en cours ; sinon 0
}

bool Commands::trigger(const QString& id, const QVariant& argument, int user)
{
    if (user != 0) {
        const actions::CommandInfo* info = m_registry.find(id);
        if (info && info->opensDialog) {
            emit refusedForSecondUser(id); // F-303 : seul le premier utilisateur ouvre une boîte
            return false;
        }
    }
    bool done = false;
    {
        // Profondeur rendue même si le gestionnaire lève une exception : sinon tout reload() serait différé
        // à jamais. L'utilisateur d'un déclenchement imbriqué est rendu au retour.
        ++m_triggerDepth;
        const int outerUser = m_triggerUser;
        m_triggerUser = user;
        const auto leave = qScopeGuard([this, outerUser] {
            --m_triggerDepth;
            m_triggerUser = outerUser;
        });
        done = m_registry.trigger(id, argument, m_platform);
    }
    if (m_triggerDepth == 0 && m_reloadPending) {
        m_reloadPending = false;
        reload();
    }
    if (!done) {
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

void Commands::setNativeHandler(const QString& id, actions::CommandRegistry::Handler handler)
{
    if (!m_registry.find(id)) {
        qWarning("ActCraft : gestionnaire pour une commande inconnue : %s", qPrintable(id));
        return;
    }
    if (handler) {
        m_nativeHandlers.insert(id, std::move(handler));
    } else {
        m_nativeHandlers.remove(id);
    }
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
    return !m_hidden.contains(id) && m_registry.isAvailable(id, m_platform);
}

void Commands::setHidden(const QString& id, bool hidden)
{
    if (hidden ? !m_hidden.contains(id) : m_hidden.contains(id)) {
        if (hidden) {
            m_hidden.insert(id);
        } else {
            m_hidden.remove(id);
        }
        emit layoutChanged();
    }
}

bool Commands::isEnabled(const QString& id) const
{
    return m_registry.isEnabled(id);
}

void Commands::setEnabled(const QString& id, bool enabled)
{
    // L'état propre, pas isEnabled() : celui-ci voit à travers le verrou ; une commande désactivée pendant
    // le verrou resterait sinon active à sa levée (amendement 1).
    if (!m_registry.find(id) || m_registry.isOwnEnabled(id) == enabled) {
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

void Commands::setLocked(bool locked)
{
    if (m_registry.isLocked() == locked) {
        return;
    }
    m_registry.setLocked(locked);
    bumpState();
}

void Commands::setRestricted(const QSet<QString>& ids)
{
    if (m_registry.restrictedIds() == ids) {
        return; // aucun redessin des menus pour des règles inchangées
    }
    m_registry.setRestricted(ids);
    bumpState();
}

bool Commands::isRestricted(const QString& id) const
{
    return m_registry.isRestricted(id);
}

void Commands::reportRestrictionRefusal(const QString& id)
{
    emit refusedByRestriction(id);
}

bool Commands::isLocked() const noexcept
{
    return m_registry.isLocked();
}

bool Commands::allowsWhileLocked(const QString& id) const
{
    return !isLocked() || isEnabled(id);
}

bool Commands::allowsWhileLocked(const Commands* commands, const char* id)
{
    return commands == nullptr || commands->allowsWhileLocked(QString::fromLatin1(id));
}

int Commands::lockedCount() const
{
    if (!m_registry.isLocked()) {
        return 0;
    }
    int count = 0;
    for (const QString& id : m_registry.ids()) {
        if (isAvailable(id) && m_registry.find(id)->lockable) {
            ++count;
        }
    }
    return count;
}

} // namespace actcraft::ui
