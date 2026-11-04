#include <actcraft/ui/profilecontroller.h>

#include <QFileInfo>
#include <QJsonObject>
#include <QJsonValue>
#include <QUrl>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QVariantMap>

#include <utility>

namespace actcraft::ui {

namespace {

const QString kActivateCommand = QStringLiteral("profile.activate");
const QString kSaveAsCommand = QStringLiteral("profile.save-as");
const QString kDeleteCommand = QStringLiteral("profile.delete");
const QString kResetCommand = QStringLiteral("profile.reset");
const QString kImportCommand = QStringLiteral("profile.import");
const QString kExportCommand = QStringLiteral("profile.export");

// Chemin local d'une URL de fichier (QUrl ou texte « file:… ») ou chemin tel quel ; vide sans argument.
QString localPath(const QVariant& argument)
{
    const QUrl url = argument.metaType().id() == QMetaType::QUrl ? argument.toUrl() : QUrl(argument.toString());
    if (url.isLocalFile()) {
        return url.toLocalFile();
    }
    return argument.toString();
}

// « Mon profil 2 » -> « mon-profil-2 » ; vide si rien n'est utilisable.
QString slugFor(const QString& name)
{
    static const QRegularExpression invalid(QStringLiteral("[^a-z0-9]+"));
    QString slug = name.trimmed().toLower();
    slug.replace(invalid, QStringLiteral("-"));
    while (slug.startsWith(QLatin1Char('-'))) {
        slug.remove(0, 1);
    }
    while (slug.endsWith(QLatin1Char('-'))) {
        slug.chop(1);
    }
    return slug;
}
constexpr int kRecentToolsMax = 8; // F-140

QString builtinRoot()
{
    return QStringLiteral(":/actcraft/actions/profiles");
}

bool validId(const QString& id)
{
    static const QRegularExpression pattern(QStringLiteral("^[a-z0-9-]+$"));
    return pattern.match(id).hasMatch();
}

bool validColor(const QString& color)
{
    static const QRegularExpression pattern(QStringLiteral("^#[0-9A-Fa-f]{6}$"));
    return pattern.match(color).hasMatch();
}

// Identifiants de `BrowserPanel.navigators` (amendement 4, A8).
const QStringList& navigatorIds()
{
    static const QStringList ids{QStringLiteral("pages"),      QStringLiteral("ressources"),
                                 QStringLiteral("objets"),     QStringLiteral("notes"),
                                 QStringLiteral("proprietes"), QStringLiteral("actions")};
    return ids;
}

} // namespace

ProfileController::ProfileController(QObject* parent)
    : QObject(parent), m_store(defaultUserRoot(), builtinRoot(), builtinOrder())
{
    m_saveTimer.setSingleShot(true);
    m_saveTimer.setInterval(kSaveDelayMs);
    connect(&m_saveTimer, &QTimer::timeout, this, &ProfileController::onSaveTimeout);
}

ProfileController::ProfileController(actions::ProfileStore store, Preferences* preferences,
                                     Commands* commands, QObject* parent)
    : QObject(parent), m_store(std::move(store)), m_preferences(preferences), m_commands(commands)
{
    m_saveTimer.setSingleShot(true);
    m_saveTimer.setInterval(kSaveDelayMs);
    connect(&m_saveTimer, &QTimer::timeout, this, &ProfileController::onSaveTimeout);
    bindCommands();
}

ProfileController::~ProfileController()
{
    saveNow(); // modification encore en attente : écrite à la fermeture
    unbindCommands();
}

QStringList ProfileController::builtinOrder()
{
    return {QStringLiteral("en-classe"), QStringLiteral("preparation"), QStringLiteral("lecture-ecriture"),
            QStringLiteral("calcul-geometrie"), QStringLiteral("son-image")};
}

QString ProfileController::defaultProfileId()
{
    return QStringLiteral("preparation");
}

QString ProfileController::defaultUserRoot()
{
    return QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation) + QStringLiteral("/profiles");
}

void ProfileController::classBegin() {}

void ProfileController::componentComplete()
{
    start();
}

void ProfileController::start()
{
    ++m_startCount;
    emit started();
    const QString wanted = m_preferences ? m_preferences->currentProfile() : QString();
    if (!wanted.isEmpty() && activate(wanted)) {
        return;
    }
    if (activate(defaultProfileId())) {
        return;
    }
    // Même le profil par défaut ne se lit pas : profil vide, disposition des ressources.
    m_profile = actions::Profile{};
    m_profile.id = defaultProfileId();
    m_dirty = false;
    applyLayoutDir(QString());
    emit profileChanged();
    emit listChanged();
}

bool ProfileController::activate(const QString& id)
{
    if (!validId(id)) {
        emit failed(tr("Identifiant de profil refusé : « %1 »").arg(id));
        return false;
    }
    // Le profil quitté garde sa dernière modification : écrite tout de suite, mais jamais pendant un contact
    // (mise de côté jusqu'au relâché).
    if (!m_contactActive) {
        saveNow();
    }
    // Version la plus récente : le profil courant s'il reste modifié, un profil mis de côté, sinon le disque.
    QString error;
    std::optional<actions::Profile> loaded;
    bool carried = false;
    if (id == m_profile.id && m_dirty) {
        loaded = m_profile;
        carried = true;
    } else if (const auto it = m_departed.constFind(id); it != m_departed.cend()) {
        loaded = it.value();
        carried = true;
    } else {
        loaded = m_store.load(id, error);
    }
    if (!loaded) {
        emit failed(error);
        return false;
    }
    m_departed.remove(id);
    if (m_dirty && id != m_profile.id) {
        m_departed.insert(m_profile.id, m_profile);
    }
    m_profile = std::move(*loaded);
    m_dirty = carried;
    applyLayoutDir(id);
    if (m_preferences) {
        m_preferences->setCurrentProfile(id);
    }
    emit profileChanged();
    emit listChanged();
    return true;
}

void ProfileController::applyLayoutDir(const QString& id)
{
    if (!m_commands) {
        return;
    }
    const QString dir = id.isEmpty() ? QString() : m_store.directory(id);
    // Sans disposition propre au profil : celle des ressources (Commands lit alors le seul fichier fourni).
    m_commands->setProfileDir(
        !dir.isEmpty() && QFileInfo::exists(dir + QStringLiteral("/layout.json")) ? dir : QString());
}

void ProfileController::saveNow()
{
    m_saveTimer.stop();
    // Une écriture refusée reste en attente : réessayée au prochain enregistrement, au plus tard à la
    // fermeture. Copie : un récepteur de saved() peut réactiver un profil pendant la boucle.
    const QHash<QString, actions::Profile> departed = std::exchange(m_departed, {});
    for (const actions::Profile& profile : departed) {
        if (!write(profile) && !m_departed.contains(profile.id) && profile.id != m_profile.id) {
            m_departed.insert(profile.id, profile);
        }
    }
    if (m_dirty && write(m_profile)) {
        m_dirty = false;
    }
}

bool ProfileController::write(const actions::Profile& profile)
{
    QString error;
    if (!m_store.save(profile, error)) {
        emit failed(error);
        return false;
    }
    ++m_saveCount;
    emit saved();
    emit listChanged(); // « modifié » d'un profil fourni
    return true;
}

bool ProfileController::hasPendingWrite() const noexcept
{
    return m_dirty || !m_departed.isEmpty();
}

const actions::Profile& ProfileController::profile() const noexcept
{
    return m_profile;
}

actions::Profile& ProfileController::editProfile()
{
    return m_profile;
}

void ProfileController::touch()
{
    emit profileChanged();
    scheduleSave();
}

actions::ProfileStore& ProfileController::store() noexcept
{
    return m_store;
}

void ProfileController::scheduleSave()
{
    m_dirty = true;
    if (!m_contactActive) {
        m_saveTimer.start(); // relancée à chaque modification : un seul enregistrement par rafale
    }
}

void ProfileController::onSaveTimeout()
{
    if (m_contactActive) {
        return; // le relâché relancera l'attente
    }
    saveNow();
}

void ProfileController::bindCommands()
{
    if (!m_commands) {
        return;
    }
    m_commands->setNativeHandler(kActivateCommand,
                                 [this](const QVariant& argument) { activate(argument.toString()); });
    m_commands->setNativeHandler(kSaveAsCommand,
                                 [this](const QVariant& argument) { saveAs(argument.toString()); });
    m_commands->setNativeHandler(kDeleteCommand, [this](const QVariant&) { deleteCurrent(); });
    m_commands->setNativeHandler(kResetCommand, [this](const QVariant&) { resetCurrent(); });
    m_commands->setNativeHandler(kImportCommand, [this](const QVariant& argument) { importFrom(argument); });
    m_commands->setNativeHandler(kExportCommand,
                                 [this](const QVariant& argument) { exportCurrent(argument); });
}

// Sans nom : rien (le QML ouvre alors son champ de nom). Le profil courant, modifications en attente
// comprises, est copié sous le nouvel identifiant ; l'original garde son état enregistré.
void ProfileController::saveAs(const QString& name)
{
    const QString label = name.trimmed();
    if (label.isEmpty()) {
        return;
    }
    const QString id = slugFor(label);
    QString error;
    if (!validId(id)) {
        emit failed(tr("Nom de profil refusé : « %1 »").arg(label));
        return;
    }
    if (!m_store.saveAs(m_profile, id, label, error)) {
        emit failed(error);
        return;
    }
    m_saveTimer.stop();
    m_dirty = false;
    activate(id);
}

void ProfileController::deleteCurrent()
{
    const QString id = m_profile.id;
    QString error;
    if (!m_store.remove(id, error)) {
        emit failed(error);
        return;
    }
    m_saveTimer.stop();
    m_dirty = false;
    m_departed.remove(id);
    activate(defaultProfileId());
}

void ProfileController::resetCurrent()
{
    const QString id = m_profile.id;
    QString error;
    if (!m_store.reset(id, error)) {
        emit failed(error);
        return;
    }
    m_saveTimer.stop();
    m_dirty = false;
    m_departed.remove(id);
    activate(id);
}

void ProfileController::importFrom(const QVariant& source)
{
    const QString path = localPath(source);
    if (path.isEmpty()) {
        return; // le QML ouvre son sélecteur de fichier
    }
    QString error;
    const std::optional<QString> id = m_store.importFrom(path, error);
    if (!id) {
        emit failed(error);
        return;
    }
    activate(*id);
}

void ProfileController::exportCurrent(const QVariant& target)
{
    const QString path = localPath(target);
    if (path.isEmpty()) {
        return;
    }
    saveNow(); // le fichier exporté relit le profil enregistré
    QString error;
    if (!m_store.exportTo(m_profile.id, path, error)) {
        emit failed(error);
    }
}

void ProfileController::unbindCommands()
{
    if (m_commands) {
        for (const QString& id : {kActivateCommand, kSaveAsCommand, kDeleteCommand, kResetCommand,
                                  kImportCommand, kExportCommand}) {
            m_commands->setNativeHandler(id, {});
        }
    }
}

Commands* ProfileController::commands() const
{
    return m_commands;
}

void ProfileController::setCommands(Commands* commands)
{
    if (m_commands == commands) {
        return;
    }
    unbindCommands();
    m_commands = commands;
    bindCommands();
    emit commandsChanged();
}

Preferences* ProfileController::preferences() const
{
    return m_preferences;
}

void ProfileController::setPreferences(Preferences* preferences)
{
    if (m_preferences == preferences) {
        return;
    }
    m_preferences = preferences;
    emit preferencesChanged();
}

QString ProfileController::userRoot() const
{
    return m_userRoot;
}

void ProfileController::setUserRoot(const QString& root)
{
    if (m_userRoot == root) {
        return;
    }
    m_userRoot = root;
    m_store = actions::ProfileStore(root.isEmpty() ? defaultUserRoot() : root, builtinRoot(), builtinOrder());
    emit userRootChanged();
    emit listChanged();
}

QString ProfileController::currentId() const
{
    return m_profile.id;
}

QString ProfileController::label() const
{
    return m_profile.label;
}

QVariantList ProfileController::profiles() const
{
    QVariantList out;
    for (const QString& id : m_store.ids()) {
        QString error;
        const std::optional<actions::Profile> p = m_store.load(id, error);
        const bool builtin = m_store.isBuiltin(id);
        out.append(QVariantMap{{QStringLiteral("id"), id},
                               {QStringLiteral("label"), p ? p->label : id},
                               {QStringLiteral("builtin"), builtin},
                               {QStringLiteral("modified"), builtin && m_store.hasUserCopy(id)}});
    }
    return out;
}

int ProfileController::iconPixels() const
{
    return m_profile.iconPixels;
}

void ProfileController::setIconPixels(int pixels)
{
    const int clamped = actions::clampIconPixels(pixels);
    if (m_profile.iconPixels == clamped) {
        return;
    }
    m_profile.iconPixels = clamped;
    touch();
}

QString ProfileController::paletteDock() const
{
    return actions::dockName(m_profile.palette.dock);
}

void ProfileController::setPaletteDock(const QString& dock)
{
    const std::optional<actions::Dock> parsed = actions::dockFromName(dock);
    if (!parsed || *parsed == m_profile.palette.dock) {
        return;
    }
    m_profile.palette.dock = *parsed;
    touch();
}

QPointF ProfileController::paletteFloat() const
{
    return {m_profile.palette.floatX, m_profile.palette.floatY};
}

void ProfileController::setPaletteFloat(QPointF point)
{
    if (paletteFloat() == point) {
        return;
    }
    m_profile.palette.floatX = point.x();
    m_profile.palette.floatY = point.y();
    touch();
}

bool ProfileController::paletteCollapsed() const
{
    return m_profile.palette.collapsed;
}

void ProfileController::setPaletteCollapsed(bool on)
{
    if (m_profile.palette.collapsed == on) {
        return;
    }
    m_profile.palette.collapsed = on;
    touch();
}

bool ProfileController::shortcutsHidden() const
{
    return m_profile.palette.shortcutsHidden;
}

void ProfileController::setShortcutsHidden(bool on)
{
    if (m_profile.palette.shortcutsHidden == on) {
        return;
    }
    m_profile.palette.shortcutsHidden = on;
    touch();
}

bool ProfileController::palettePinned() const
{
    return m_profile.palette.pinned;
}

void ProfileController::setPalettePinned(bool on)
{
    if (m_profile.palette.pinned == on) {
        return;
    }
    m_profile.palette.pinned = on;
    touch();
}

int ProfileController::swatches() const
{
    return m_profile.palette.swatches;
}

void ProfileController::setSwatches(int count)
{
    if ((count != 8 && count != 16 && count != 24) || m_profile.palette.swatches == count) {
        return; // F-900 : 8, 16 ou 24
    }
    m_profile.palette.swatches = count;
    touch();
}

bool ProfileController::browsersVisible() const
{
    return m_profile.browsers.visible;
}

void ProfileController::setBrowsersVisible(bool on)
{
    if (m_profile.browsers.visible == on) {
        return;
    }
    m_profile.browsers.visible = on;
    touch();
}

bool ProfileController::browsersPinned() const
{
    return m_profile.browsers.pinned;
}

void ProfileController::setBrowsersPinned(bool on)
{
    if (m_profile.browsers.pinned == on) {
        return;
    }
    m_profile.browsers.pinned = on;
    touch();
}

QString ProfileController::browsersSide() const
{
    return m_profile.browsers.side;
}

void ProfileController::setBrowsersSide(const QString& side)
{
    if ((side != QLatin1String("left") && side != QLatin1String("right")) ||
        m_profile.browsers.side == side) {
        return;
    }
    m_profile.browsers.side = side;
    touch();
}

QString ProfileController::browsersCurrent() const
{
    return m_profile.browsers.current;
}

void ProfileController::setBrowsersCurrent(const QString& tab)
{
    if (!navigatorIds().contains(tab) || m_profile.browsers.current == tab) {
        return;
    }
    m_profile.browsers.current = tab;
    touch();
}

QStringList ProfileController::extraColors() const
{
    return m_profile.extraColors;
}

void ProfileController::addColor(const QString& color)
{
    if (!validColor(color)) {
        return;
    }
    const QString normalized = color.toUpper();
    if (m_profile.extraColors.contains(normalized, Qt::CaseInsensitive)) {
        return;
    }
    m_profile.extraColors.append(normalized);
    touch();
}

QStringList ProfileController::recentTools() const
{
    return m_profile.recentTools;
}

void ProfileController::pushRecentTool(const QString& id)
{
    if (id.isEmpty() || (!m_profile.recentTools.isEmpty() && m_profile.recentTools.constFirst() == id)) {
        return;
    }
    m_profile.recentTools.removeAll(id);
    m_profile.recentTools.prepend(id);
    while (m_profile.recentTools.size() > kRecentToolsMax) {
        m_profile.recentTools.removeLast();
    }
    touch();
}

QVariant ProfileController::setting(const QString& section, const QString& key,
                                    const QVariant& fallback) const
{
    const QJsonValue value = m_profile.settings.value(section).toObject().value(key);
    return value.isUndefined() ? fallback : value.toVariant();
}

void ProfileController::setSetting(const QString& section, const QString& key, const QVariant& value)
{
    QJsonObject entries = m_profile.settings.value(section).toObject();
    const QJsonValue updated = QJsonValue::fromVariant(value);
    if (entries.value(key) == updated) {
        return;
    }
    entries.insert(key, updated);
    m_profile.settings.insert(section, entries);
    touch();
}

QStringList ProfileController::pinned() const
{
    return m_profile.pinned;
}

bool ProfileController::fitPage() const
{
    return m_profile.fitPage;
}

bool ProfileController::trashVisible() const
{
    return m_profile.trashVisible;
}

void ProfileController::setTrashVisible(bool on)
{
    if (m_profile.trashVisible == on) {
        return;
    }
    m_profile.trashVisible = on;
    touch();
}

bool ProfileController::contactActive() const noexcept
{
    return m_contactActive;
}

void ProfileController::setContactActive(bool active)
{
    if (m_contactActive == active) {
        return;
    }
    m_contactActive = active;
    if (active) {
        m_saveTimer.stop(); // aucune écriture pendant un contact
    } else if (hasPendingWrite()) {
        m_saveTimer.start(); // le relâché relance l'attente entière
    }
    emit contactActiveChanged();
}

int ProfileController::saveCount() const noexcept
{
    return m_saveCount;
}

int ProfileController::startCount() const noexcept
{
    return m_startCount;
}

} // namespace actcraft::ui
