#include <actcraft/actions/commandregistry.h>
#include <actcraft/actions/layout.h>
#include <actcraft/ui/layouteditor.h>

#include <QDir>
#include <QJsonDocument>
#include <QJsonValue>
#include <QSaveFile>
#include <QSet>
#include <QVariantMap>

#include <utility>

namespace actcraft::ui {

namespace {

const QString kLayoutFile = QStringLiteral("layout.json");
const QString kPalette = QStringLiteral("palette");
const QString kObjectBar = QStringLiteral("objectBar");
const QString kMenuPrefix = QStringLiteral("menu:");
const QString kGroup = QStringLiteral("group");
const QString kItems = QStringLiteral("items");
const QString kSubmenu = QStringLiteral("submenu");

// Listes plates du format (jetons : identifiant, « - », sous-menu en ligne ou « >id »).
bool isFlatTarget(const QString& target)
{
    return target == kObjectBar || target == QLatin1String("contextMenu") ||
           target == QLatin1String("objectMenu");
}

qsizetype menuIndex(const QJsonArray& menus, const QString& id)
{
    for (qsizetype i = 0; i < menus.size(); ++i) {
        if (menus.at(i).toObject().value(QStringLiteral("id")).toString() == id) {
            return i;
        }
    }
    return -1;
}

// Palette aplatie : chaque groupe donne son objet (en-tête, gardé entier) suivi de ses identifiants.
QJsonArray flattenPalette(const QJsonArray& groups)
{
    QJsonArray out;
    for (const QJsonValue& group : groups) {
        out.append(group);
        for (const QJsonValue& item : group.toObject().value(kItems).toArray()) {
            out.append(item);
        }
    }
    return out;
}

bool isGroupHeader(const QJsonValue& token)
{
    return token.isObject() && token.toObject().contains(kGroup);
}

QJsonArray rebuildPalette(const QJsonArray& tokens)
{
    QJsonArray groups;
    QJsonObject group;
    QJsonArray items;
    bool open = false;
    const auto close = [&]() {
        if (open) {
            if (group.contains(kItems) || !items.isEmpty()) {
                group.insert(kItems, items);
            }
            groups.append(group);
        }
    };
    for (const QJsonValue& token : tokens) {
        if (isGroupHeader(token)) {
            close();
            group = token.toObject();
            items = QJsonArray();
            open = true;
            continue;
        }
        if (!open) { // entrées remontées au-dessus du premier en-tête : un groupe à elles
            group = QJsonObject{{kGroup, QStringLiteral("groupe")}};
            items = QJsonArray();
            open = true;
        }
        items.append(token);
    }
    close();
    return groups;
}

QJsonArray tokensOf(const QJsonObject& layout, const QString& target)
{
    if (target == kPalette) {
        return flattenPalette(layout.value(kPalette).toArray());
    }
    if (isFlatTarget(target)) {
        return layout.value(target).toArray();
    }
    if (target.startsWith(kMenuPrefix)) {
        const QJsonArray menus = layout.value(QStringLiteral("menus")).toArray();
        const qsizetype index = menuIndex(menus, target.mid(kMenuPrefix.size()));
        return index < 0 ? QJsonArray() : menus.at(index).toObject().value(kItems).toArray();
    }
    return {};
}

// Remplace la seule liste de la cible ; tout le reste du JSON effectif est gardé tel quel.
void replaceTokens(QJsonObject& layout, const QString& target, const QJsonArray& tokens)
{
    if (target == kPalette) {
        layout.insert(kPalette, rebuildPalette(tokens));
    } else if (isFlatTarget(target)) {
        layout.insert(target, tokens);
    } else if (target.startsWith(kMenuPrefix)) {
        QJsonArray menus = layout.value(QStringLiteral("menus")).toArray();
        const qsizetype index = menuIndex(menus, target.mid(kMenuPrefix.size()));
        if (index >= 0) {
            QJsonObject menu = menus.at(index).toObject();
            menu.insert(kItems, tokens);
            menus.replace(index, menu);
            layout.insert(QStringLiteral("menus"), menus);
        }
    }
}

QJsonObject parseObject(const QByteArray& json)
{
    const QJsonDocument document = QJsonDocument::fromJson(json);
    return document.isObject() ? document.object() : QJsonObject();
}

QVariantMap targetEntry(const QString& id, const QString& label)
{
    return {{QStringLiteral("id"), id}, {QStringLiteral("label"), label}};
}

} // namespace

LayoutEditor::LayoutEditor(QObject* parent) : QObject(parent) {}

Commands* LayoutEditor::commands() const
{
    return m_commands;
}

void LayoutEditor::setCommands(Commands* commands)
{
    if (m_commands == commands) {
        return;
    }
    QObject::disconnect(m_layoutConnection);
    m_commands = commands;
    if (m_commands) {
        // Plateforme, commandes cachées, libellés : le catalogue et l'affichage suivent ; les jetons restent.
        m_layoutConnection = connect(m_commands, &Commands::layoutChanged, this, [this]() {
            refreshCatalog();
            refreshItems(false);
        });
    }
    emit commandsChanged();
    refreshCatalog();
    reload();
}

ProfileController* LayoutEditor::profile() const
{
    return m_profile;
}

void LayoutEditor::setProfile(ProfileController* profile)
{
    if (m_profile == profile) {
        return;
    }
    QObject::disconnect(m_profileConnection);
    m_profile = profile;
    if (m_profile) {
        m_profileConnection =
            connect(m_profile, &ProfileController::profileChanged, this, &LayoutEditor::onProfileChanged);
    }
    emit profileChanged();
    reload();
}

// profileChanged suit aussi chaque réglage du profil : seule l'activation d'un autre profil relit.
void LayoutEditor::onProfileChanged()
{
    if (m_profile && m_profile->currentId() != m_profileId) {
        reload();
    }
}

QVariantList LayoutEditor::targets() const
{
    return m_targets;
}

QString LayoutEditor::target() const
{
    return m_target;
}

void LayoutEditor::setTarget(const QString& id)
{
    if (id == m_target) {
        return;
    }
    bool known = false;
    for (const QVariant& entry : m_targets) {
        known = known || entry.toMap().value(QStringLiteral("id")).toString() == id;
    }
    if (!known) {
        return;
    }
    m_target = id;
    emit targetChanged();
    loadTarget();
}

QVariantList LayoutEditor::items() const
{
    return m_items;
}

QVariantList LayoutEditor::buildItems() const
{
    QVariantList out;
    for (const QJsonValue& token : m_tokens) {
        QVariantMap item{
            {QStringLiteral("separator"), false}, {kSubmenu, false}, {QStringLiteral("available"), true}};
        if (isGroupHeader(token)) {
            const QString group = token.toObject().value(kGroup).toString();
            item.insert(QStringLiteral("separator"), true);
            item.insert(kGroup, group);
            item.insert(QStringLiteral("label"), group);
        } else if (token.isObject()) {
            const QJsonObject submenu = token.toObject();
            item.insert(kSubmenu, true);
            item.insert(QStringLiteral("id"), submenu.value(kSubmenu).toString());
            item.insert(QStringLiteral("label"), submenu.value(QStringLiteral("label")).toString());
        } else if (token.toString() == QLatin1String("-")) {
            item.insert(QStringLiteral("separator"), true);
        } else if (token.toString().startsWith(QLatin1Char('>'))) {
            item.insert(kSubmenu, true);
            item.insert(QStringLiteral("id"), token.toString().mid(1));
            item.insert(QStringLiteral("label"), token.toString().mid(1));
        } else {
            const QString id = token.toString();
            const QString label = m_commands ? m_commands->label(id) : QString();
            item.insert(QStringLiteral("id"), id);
            item.insert(QStringLiteral("label"), label.isEmpty() ? id : label);
            item.insert(QStringLiteral("icon"), m_commands ? m_commands->iconUrl(id) : QString());
            item.insert(QStringLiteral("available"), m_commands && m_commands->isAvailable(id));
        }
        out.append(item);
    }
    return out;
}

// Commandes disponibles sur la plateforme, rangées par catégorie (préfixe de l'identifiant) dans l'ordre de
// leur première rencontre, puis dans l'ordre de déclaration.
QVariantList LayoutEditor::catalog() const
{
    return m_catalog;
}

// categoryStart : première entrée de sa catégorie (en-tête dans la liste).
QVariantList LayoutEditor::buildCatalog() const
{
    if (!m_commands) {
        return {};
    }
    QStringList categories;
    QHash<QString, QVariantList> byCategory;
    for (const QString& id : m_commands->registry().ids()) {
        if (!m_commands->isAvailable(id)) {
            continue;
        }
        const QString category = id.section(QLatin1Char('.'), 0, 0);
        if (!byCategory.contains(category)) {
            categories << category;
        }
        byCategory[category].append(QVariantMap{{QStringLiteral("id"), id},
                                                {QStringLiteral("label"), m_commands->label(id)},
                                                {QStringLiteral("icon"), m_commands->iconUrl(id)},
                                                {QStringLiteral("category"), category},
                                                {QStringLiteral("categoryStart"), false}});
    }
    QVariantList out;
    for (const QString& category : categories) {
        QVariantList entries = byCategory.value(category);
        QVariantMap first = entries.constFirst().toMap();
        first.insert(QStringLiteral("categoryStart"), true);
        entries.replace(0, first);
        out.append(entries);
    }
    return out;
}

void LayoutEditor::refreshCatalog()
{
    QVariantList catalog = buildCatalog();
    if (catalog != m_catalog) {
        m_catalog = std::move(catalog);
        emit catalogChanged();
    }
}

// Les délégués QML lisent la liste mise en cache : chaque lecture ne la reconstruit pas.
void LayoutEditor::refreshItems(bool force)
{
    QVariantList items = buildItems();
    if (force || items != m_items) {
        m_items = std::move(items);
        emit itemsChanged();
    }
}

bool LayoutEditor::separatorAllowed() const
{
    return !m_target.isEmpty() && m_target != kObjectBar;
}

bool LayoutEditor::modified() const noexcept
{
    return !m_edits.isEmpty();
}

bool LayoutEditor::add(const QString& id)
{
    if (m_target.isEmpty() || !m_commands || !m_commands->isAvailable(id) ||
        m_tokens.contains(QJsonValue(id))) {
        return false;
    }
    QJsonArray tokens = m_tokens;
    tokens.append(id);
    setTokens(tokens);
    return true;
}

bool LayoutEditor::removeAt(int index)
{
    if (index < 0 || index >= m_tokens.size()) {
        return false;
    }
    QJsonArray tokens = m_tokens;
    tokens.removeAt(index);
    setTokens(tokens);
    return true;
}

bool LayoutEditor::moveUp(int index)
{
    if (index <= 0 || index >= m_tokens.size()) {
        return false;
    }
    QJsonArray tokens = m_tokens;
    const QJsonValue moved = tokens.at(index);
    tokens.replace(index, tokens.at(index - 1));
    tokens.replace(index - 1, moved);
    setTokens(tokens);
    return true;
}

bool LayoutEditor::moveDown(int index)
{
    return index >= 0 && index < m_tokens.size() - 1 && moveUp(index + 1);
}

bool LayoutEditor::addSeparator()
{
    if (!separatorAllowed()) {
        return false;
    }
    QJsonArray tokens = m_tokens;
    if (m_target == kPalette) {
        QSet<QString> used;
        for (const QJsonValue& token : tokens) {
            if (isGroupHeader(token)) {
                used.insert(token.toObject().value(kGroup).toString());
            }
        }
        int n = 1;
        while (used.contains(QStringLiteral("groupe-%1").arg(n))) {
            ++n;
        }
        tokens.append(QJsonObject{{kGroup, QStringLiteral("groupe-%1").arg(n)}, {kItems, QJsonArray()}});
    } else {
        tokens.append(QStringLiteral("-"));
    }
    setTokens(tokens);
    return true;
}

void LayoutEditor::reload()
{
    m_profileId = m_profile ? m_profile->currentId() : QString();
    m_edits.clear();
    m_effective = readEffective();
    rebuildTargets();
    loadTarget();
}

bool LayoutEditor::apply()
{
    const QString dir = profileDir();
    if (!m_commands || dir.isEmpty()) {
        return false;
    }
    // Le patron évident (réécrire la disposition à partir du seul contenu affiché) perdrait ce que l'éditeur
    // ne connaît pas : on repart du JSON effectif et on n'y remplace que les listes modifiées.
    QJsonObject layout = readEffective();
    if (layout.isEmpty()) {
        return false;
    }
    for (auto it = m_edits.cbegin(); it != m_edits.cend(); ++it) {
        replaceTokens(layout, it.key(), it.value());
    }
    const QByteArray json = QJsonDocument(layout).toJson();
    actions::Layout parsed;
    QString error;
    if (!actions::parseLayout(json, parsed, error) || !QDir().mkpath(dir)) {
        return false;
    }
    QSaveFile file(QDir(dir).filePath(kLayoutFile));
    if (!file.open(QIODevice::WriteOnly) || file.write(json) != json.size() || !file.commit()) {
        return false;
    }
    // Commands ne lit le dossier du profil que s'il y trouvait déjà un layout.json (applyLayoutDir) :
    // setProfileDir le pose, puis Commands::reload().
    m_commands->setProfileDir(dir);
    reload();
    return true;
}

QString LayoutEditor::profileDir() const
{
    if (!m_profile || m_profile->currentId().isEmpty()) {
        return {};
    }
    return m_profile->store().directory(m_profile->currentId());
}

// Disposition effective : layout.json du profil s'il se lit, sinon celle des ressources.
QJsonObject LayoutEditor::readEffective() const
{
    const QString resourceDir = actions::dataResourceDir();
    const QJsonObject layered = parseObject(actions::readLayeredFile(kLayoutFile, profileDir(), resourceDir));
    return layered.isEmpty() ? parseObject(actions::readLayeredFile(kLayoutFile, QString(), resourceDir))
                             : layered;
}

void LayoutEditor::rebuildTargets()
{
    QVariantList targets{targetEntry(kPalette, tr("Palette principale")),
                         targetEntry(kObjectBar, tr("Barre d'objet")),
                         targetEntry(QStringLiteral("contextMenu"), tr("Menu contextuel")),
                         targetEntry(QStringLiteral("objectMenu"), tr("Menu de l'objet"))};
    for (const QJsonValue& menu : m_effective.value(QStringLiteral("menus")).toArray()) {
        const QJsonObject object = menu.toObject();
        targets.append(targetEntry(kMenuPrefix + object.value(QStringLiteral("id")).toString(),
                                   tr("Menu %1").arg(object.value(QStringLiteral("label")).toString())));
    }
    if (targets != m_targets) {
        m_targets = targets;
        emit targetsChanged();
    }
    bool known = false;
    for (const QVariant& entry : m_targets) {
        known = known || entry.toMap().value(QStringLiteral("id")).toString() == m_target;
    }
    if (!known) {
        m_target = kPalette;
        emit targetChanged();
    }
}

void LayoutEditor::loadTarget()
{
    m_tokens = m_edits.value(m_target, tokensOf(m_effective, m_target));
    refreshItems(true);
}

void LayoutEditor::setTokens(const QJsonArray& tokens)
{
    m_tokens = tokens;
    m_edits.insert(m_target, tokens);
    refreshItems(true);
}

} // namespace actcraft::ui
