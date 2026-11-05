#include <actcraft/actions/commandregistry.h>
#include <actcraft/actions/layout.h>
#include <actcraft/ui/layouteditor.h>

#include <QJsonDocument>
#include <QJsonValue>
#include <QSet>
#include <QVariantMap>

#include <algorithm>
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

// Groupes que ToolPalette ne dessine pas en boutons (il y charge le nuancier et l'épaisseur) : blocs fixes.
bool isFixedGroup(const QJsonValue& token)
{
    if (!isGroupHeader(token)) {
        return false;
    }
    const QString group = token.toObject().value(kGroup).toString();
    return group == QLatin1String("couleurs") || group == QLatin1String("epaisseur");
}

// Commandes rangées sous un bloc fixe : absentes de la palette dessinée. Toujours 0 hors de la palette.
qsizetype hiddenCount(const QJsonArray& tokens)
{
    qsizetype count = 0;
    bool fixed = false;
    for (const QJsonValue& token : tokens) {
        if (isGroupHeader(token)) {
            fixed = isFixedGroup(token);
        } else if (fixed) {
            ++count;
        }
    }
    return count;
}

void swapAt(QJsonArray& tokens, int a, int b)
{
    const QJsonValue first = tokens.at(a);
    tokens.replace(a, tokens.at(b));
    tokens.replace(b, first);
}

// Pousse le jeton en `index` dans le sens `step` (par-dessus les blocs fixes) tant que la liste cache plus de
// commandes que `hidden` ; rend sa place, ou -1 s'il bute sur un bord.
int settle(QJsonArray& tokens, int index, int step, qsizetype hidden)
{
    while (hiddenCount(tokens) > hidden) {
        const int next = index + step;
        if (next < 0 || next >= tokens.size()) {
            return -1;
        }
        swapAt(tokens, index, next);
        index = next;
    }
    return index;
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
    if (m_profile) {
        QObject::disconnect(m_profile, nullptr, this, nullptr);
    }
    m_profile = profile;
    if (m_profile) {
        connect(m_profile, &ProfileController::profileChanged, this, &LayoutEditor::onProfileChanged);
        // Écriture de la disposition du profil courant ou « Revenir à l'origine » (même identifiant) :
        // relire.
        connect(m_profile, &ProfileController::profileLayoutChanged, this, &LayoutEditor::reload);
    }
    emit profileChanged();
    reload();
}

// profileChanged suit aussi chaque réglage du profil : seule l'activation d'un autre profil relit. Les
// modifications pas encore appliquées vont au profil quitté, comme ses réglages
// (ProfileController::activate), sauf s'il n'existe plus.
void LayoutEditor::onProfileChanged()
{
    if (!m_profile || m_profile->currentId() == m_profileId) {
        return;
    }
    if (!m_edits.isEmpty() && m_profile->store().ids().contains(m_profileId)) {
        write(m_profileId);
    }
    reload();
}

QVariantList LayoutEditor::targets() const
{
    return m_targets;
}

QString LayoutEditor::target() const
{
    return m_target;
}

bool LayoutEditor::hasTarget(const QString& id) const
{
    return std::any_of(m_targets.cbegin(), m_targets.cend(), [&id](const QVariant& entry) {
        return entry.toMap().value(QStringLiteral("id")).toString() == id;
    });
}

void LayoutEditor::setTarget(const QString& id)
{
    if (id == m_target || !hasTarget(id)) {
        return;
    }
    m_target = id;
    m_current = -1;
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
            item.insert(QStringLiteral("label"), groupLabel(group));
            item.insert(QStringLiteral("fixed"), isFixedGroup(token));
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

// Nom affiché d'un groupe de la palette ; « groupe-3 » (groupe neuf) : « Groupe 3 » ; inconnu : tel quel.
QString LayoutEditor::groupLabel(const QString& group)
{
    const QHash<QString, QString> labels{{QStringLiteral("commandes"), tr("Commandes")},
                                         {QStringLiteral("pages"), tr("Pages")},
                                         {QStringLiteral("couleurs"), tr("Couleurs")},
                                         {QStringLiteral("epaisseur"), tr("Épaisseur")},
                                         {QStringLiteral("outils"), tr("Outils")},
                                         {QStringLiteral("edition"), tr("Édition")},
                                         {QStringLiteral("presentation"), tr("Présentation")},
                                         {QStringLiteral("formes-texte"), tr("Formes et texte")},
                                         {QStringLiteral("groupe"), tr("Groupe")}};
    if (const auto it = labels.constFind(group); it != labels.cend()) {
        return it.value();
    }
    const QString prefix = QStringLiteral("groupe-");
    return group.startsWith(prefix) ? tr("Groupe %1").arg(group.mid(prefix.size())) : group;
}

// Nom affiché d'une catégorie (préfixe de l'identifiant) ; inconnue : telle quelle.
QString LayoutEditor::categoryLabel(const QString& category)
{
    const QHash<QString, QString> labels{{QStringLiteral("file"), tr("Fichier")},
                                         {QStringLiteral("view"), tr("Affichage")},
                                         {QStringLiteral("page"), tr("Pages")},
                                         {QStringLiteral("tool"), tr("Outils")},
                                         {QStringLiteral("pen"), tr("Stylo")},
                                         {QStringLiteral("menu"), tr("Menus")},
                                         {QStringLiteral("help"), tr("Aide")},
                                         {QStringLiteral("edit"), tr("Édition")},
                                         {QStringLiteral("clear"), tr("Effacer")},
                                         {QStringLiteral("object"), tr("Objets")},
                                         {QStringLiteral("present"), tr("Présentation")},
                                         {QStringLiteral("math"), tr("Mathématiques")},
                                         {QStringLiteral("insert"), tr("Insertion")},
                                         {QStringLiteral("equation"), tr("Équations")},
                                         {QStringLiteral("recognition"), tr("Reconnaissance")},
                                         {QStringLiteral("desktop"), tr("Bureau")},
                                         {QStringLiteral("shape"), tr("Formes")},
                                         {QStringLiteral("text"), tr("Texte")},
                                         {QStringLiteral("browsers"), tr("Navigateurs")},
                                         {QStringLiteral("grid"), tr("Grille")},
                                         {QStringLiteral("arrange"), tr("Disposition")},
                                         {QStringLiteral("align"), tr("Alignement")},
                                         {QStringLiteral("transform"), tr("Transformation")},
                                         {QStringLiteral("connector"), tr("Connecteurs")},
                                         {QStringLiteral("tab"), tr("Onglets")},
                                         {QStringLiteral("export"), tr("Exporter")},
                                         {QStringLiteral("palette"), tr("Palette")},
                                         {QStringLiteral("color"), tr("Couleurs")},
                                         {QStringLiteral("browser"), tr("Navigateur")},
                                         {QStringLiteral("profile"), tr("Profils")},
                                         {QStringLiteral("mode"), tr("Modes")},
                                         {QStringLiteral("resources"), tr("Ressources")},
                                         {QStringLiteral("classroom"), tr("Classe")},
                                         {QStringLiteral("spell"), tr("Orthographe")},
                                         {QStringLiteral("launcher"), tr("Raccourcis vers des programmes")},
                                         {QStringLiteral("shortcutbar"), tr("Barre de raccourcis")},
                                         {QStringLiteral("settings"), tr("Paramètres")}};
    return labels.value(category, category);
}

// Commandes disponibles sur la plateforme, rangées par catégorie (préfixe de l'identifiant) dans l'ordre de
// leur première rencontre, puis dans l'ordre de déclaration.
QVariantList LayoutEditor::catalog() const
{
    return m_catalog;
}

// categoryStart : première entrée de sa catégorie (en-tête dans la liste, categoryLabel).
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
        first.insert(QStringLiteral("categoryLabel"), categoryLabel(category));
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

// Les délégués QML lisent la liste mise en cache : chaque lecture ne la reconstruit pas. Un nouveau contenu
// signale aussi `current` : le ListView vient de remettre son currentIndex à 0.
void LayoutEditor::refreshItems(bool force)
{
    QVariantList items = buildItems();
    if (force || items != m_items) {
        m_items = std::move(items);
        emit itemsChanged();
        emit currentChanged();
    }
}

int LayoutEditor::current() const noexcept
{
    return m_current;
}

void LayoutEditor::setCurrent(int index)
{
    const int value = index >= 0 && index < m_tokens.size() ? index : -1;
    if (value != m_current) {
        m_current = value;
        emit currentChanged();
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

QString LayoutEditor::error() const
{
    return m_error;
}

bool LayoutEditor::add(const QString& id)
{
    if (m_target.isEmpty() || !m_commands || !m_commands->isAvailable(id) ||
        m_tokens.contains(QJsonValue(id))) {
        return false;
    }
    return insert(id);
}

bool LayoutEditor::removeAt(int index)
{
    if (index < 0 || index >= m_tokens.size()) {
        return false;
    }
    QJsonArray tokens = m_tokens;
    const qsizetype hidden = hiddenCount(tokens);
    tokens.removeAt(index);
    if (hiddenCount(tokens) > hidden) {
        return false; // les commandes du groupe passeraient sous un bloc fixe
    }
    setTokens(tokens, std::min(index, static_cast<int>(tokens.size()) - 1));
    return true;
}

bool LayoutEditor::moveUp(int index)
{
    return move(index, -1);
}

bool LayoutEditor::moveDown(int index)
{
    return move(index, 1);
}

bool LayoutEditor::move(int index, int step)
{
    const int next = index + step;
    if (index < 0 || index >= m_tokens.size() || next < 0 || next >= m_tokens.size()) {
        return false;
    }
    QJsonArray tokens = m_tokens;
    const qsizetype hidden = hiddenCount(tokens);
    swapAt(tokens, index, next);
    const int at = settle(tokens, next, step, hidden);
    if (at < 0) {
        return false;
    }
    setTokens(tokens, at);
    return true;
}

bool LayoutEditor::addSeparator()
{
    if (!separatorAllowed()) {
        return false;
    }
    if (m_target != kPalette) {
        return insert(QStringLiteral("-"));
    }
    QSet<QString> used;
    for (const QJsonValue& token : m_tokens) {
        if (isGroupHeader(token)) {
            used.insert(token.toObject().value(kGroup).toString());
        }
    }
    int n = 1;
    while (used.contains(QStringLiteral("groupe-%1").arg(n))) {
        ++n;
    }
    return insert(QJsonObject{{kGroup, QStringLiteral("groupe-%1").arg(n)}, {kItems, QJsonArray()}});
}

// Juste après l'entrée choisie (en fin de liste sans choix) ; sous un bloc fixe, la commande passe au groupe
// suivant, ou à la fin du précédent si le bloc est le dernier.
bool LayoutEditor::insert(const QJsonValue& token)
{
    QJsonArray tokens = m_tokens;
    const qsizetype hidden = hiddenCount(tokens);
    const int at = m_current >= 0 ? m_current + 1 : static_cast<int>(tokens.size());
    tokens.insert(at, token);
    QJsonArray down = tokens;
    int placed = settle(down, at, 1, hidden);
    if (placed >= 0) {
        tokens = down;
    } else {
        placed = settle(tokens, at, -1, hidden);
    }
    if (placed < 0) {
        return false;
    }
    setTokens(tokens, placed);
    return true;
}

void LayoutEditor::reload()
{
    m_profileId = m_profile ? m_profile->currentId() : QString();
    m_edits.clear();
    m_current = -1;
    m_effective = readEffective(m_profileId);
    rebuildTargets();
    loadTarget();
}

bool LayoutEditor::apply()
{
    if (m_edits.isEmpty()) {
        return true; // rien d'écrit : sans modification, la disposition suit toujours celle des ressources
    }
    // Succès : ProfileController::writeLayout signale profileLayoutChanged, qui relit (reload).
    return write(m_profile ? m_profile->currentId() : QString());
}

// Le patron évident (réécrire la disposition à partir du seul contenu affiché) perdrait ce que l'éditeur ne
// connaît pas : on repart du JSON effectif du profil et on n'y remplace que les listes modifiées.
bool LayoutEditor::write(const QString& profileId)
{
    if (!m_commands || !m_profile || profileId.isEmpty()) {
        return fail(tr("Disposition non enregistrée : aucun profil."));
    }
    QJsonObject layout = readEffective(profileId);
    if (layout.isEmpty()) {
        return fail(tr("Disposition non enregistrée : disposition actuelle illisible."));
    }
    for (auto it = m_edits.cbegin(); it != m_edits.cend(); ++it) {
        replaceTokens(layout, it.key(), it.value());
    }
    const QByteArray json = QJsonDocument(layout).toJson();
    actions::Layout parsed;
    QString error;
    if (!actions::parseLayout(json, parsed, error)) {
        return fail(tr("Disposition non enregistrée : %1").arg(error));
    }
    if (!m_profile->writeLayout(profileId, json)) {
        return fail(
            tr("Disposition non enregistrée dans « %1 ».").arg(m_profile->store().directory(profileId)));
    }
    setError(QString());
    return true;
}

bool LayoutEditor::fail(const QString& message)
{
    setError(message);
    return false;
}

void LayoutEditor::setError(const QString& message)
{
    if (message != m_error) {
        m_error = message;
        emit errorChanged();
    }
}

// Disposition effective du profil : son layout.json s'il passe actions::parseLayout (Commands retombe aussi
// sur les ressources sinon), sinon celle des ressources.
QJsonObject LayoutEditor::readEffective(const QString& profileId) const
{
    const QString resourceDir = actions::dataResourceDir();
    const QString dir =
        m_profile && !profileId.isEmpty() ? m_profile->store().directory(profileId) : QString();
    if (!dir.isEmpty()) {
        const QByteArray own = actions::readLayeredFile(kLayoutFile, dir, resourceDir);
        actions::Layout parsed;
        QString error;
        if (actions::parseLayout(own, parsed, error)) {
            return parseObject(own);
        }
    }
    return parseObject(actions::readLayeredFile(kLayoutFile, QString(), resourceDir));
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
    if (!hasTarget(m_target)) {
        m_target = kPalette;
        emit targetChanged();
    }
}

void LayoutEditor::loadTarget()
{
    m_tokens = m_edits.value(m_target, tokensOf(m_effective, m_target));
    refreshItems(true);
}

void LayoutEditor::setTokens(const QJsonArray& tokens, int current)
{
    m_tokens = tokens;
    m_edits.insert(m_target, tokens);
    m_current = current;
    refreshItems(true);
}

} // namespace actcraft::ui
