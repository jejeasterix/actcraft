#include <actcraft/actions/layout.h>

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

#include <utility>

namespace actcraft::actions {

namespace {

// Liste de textes ; une valeur absente donne une liste vide.
bool readStringList(const QJsonValue& value, QStringList& out)
{
    out.clear();
    if (value.isUndefined()) {
        return true;
    }
    if (!value.isArray()) {
        return false;
    }
    const QJsonArray array = value.toArray();
    for (const QJsonValue& item : array) {
        if (!item.isString()) {
            return false;
        }
        out << item.toString();
    }
    return true;
}

enum class ItemsStatus
{
    Ok,
    Invalid,        // pas un tableau, ou élément d'un type inattendu
    InvalidSubmenu, // sous-menu mal formé ou imbriqué
};

// Éléments d'un menu : identifiants, « - » ou objets `{"submenu", "label", "items"}`. `submenus` nul : on est
// déjà dans un sous-menu, où un objet est refusé (un seul niveau).
ItemsStatus readMenuItems(const QJsonValue& value, QStringList& items, std::vector<MenuSpec>* submenus)
{
    items.clear();
    if (value.isUndefined()) {
        return ItemsStatus::Ok;
    }
    if (!value.isArray()) {
        return ItemsStatus::Invalid;
    }
    for (const QJsonValue& item : value.toArray()) {
        if (item.isString()) {
            items << item.toString();
            continue;
        }
        if (!submenus) {
            return ItemsStatus::InvalidSubmenu;
        }
        if (!item.isObject()) {
            return ItemsStatus::Invalid;
        }
        const QJsonObject o = item.toObject();
        MenuSpec sub;
        sub.id = o.value(QStringLiteral("submenu")).toString();
        sub.label = o.value(QStringLiteral("label")).toString();
        if (sub.id.isEmpty() || sub.label.isEmpty() ||
            readMenuItems(o.value(QStringLiteral("items")), sub.items, nullptr) != ItemsStatus::Ok) {
            return ItemsStatus::InvalidSubmenu;
        }
        items << QStringLiteral(">") + sub.id;
        submenus->push_back(std::move(sub));
    }
    return ItemsStatus::Ok;
}

} // namespace

bool parseLayout(const QByteArray& json, Layout& out, QString& error)
{
    QJsonParseError parseError{};
    const QJsonDocument document = QJsonDocument::fromJson(json, &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        error = QStringLiteral("JSON invalide : %1").arg(parseError.errorString());
        return false;
    }
    const QJsonObject root = document.object();
    if (root.value(QStringLiteral("format")).toString() != QLatin1String("actcraft-layout/1")) {
        error = QStringLiteral("format inconnu (attendu actcraft-layout/1)");
        return false;
    }
    Layout layout;

    const QJsonArray menus = root.value(QStringLiteral("menus")).toArray();
    for (qsizetype i = 0; i < menus.size(); ++i) {
        const QJsonObject o = menus.at(i).toObject();
        MenuSpec menu;
        menu.id = o.value(QStringLiteral("id")).toString();
        menu.label = o.value(QStringLiteral("label")).toString();
        if (menu.id.isEmpty() || menu.label.isEmpty()) {
            error = QStringLiteral("menu %1 : id ou label manquant").arg(i + 1);
            return false;
        }
        const ItemsStatus status =
            readMenuItems(o.value(QStringLiteral("items")), menu.items, &menu.submenus);
        if (status == ItemsStatus::InvalidSubmenu) {
            error = QStringLiteral("menu %1 : sous-menu invalide").arg(i + 1);
            return false;
        }
        if (status != ItemsStatus::Ok) {
            error = QStringLiteral("menu %1 (%2) : items invalides").arg(i + 1).arg(menu.id);
            return false;
        }
        layout.menus.push_back(std::move(menu));
    }

    const QJsonArray palette = root.value(QStringLiteral("palette")).toArray();
    for (qsizetype i = 0; i < palette.size(); ++i) {
        const QJsonObject o = palette.at(i).toObject();
        PaletteGroup group;
        group.group = o.value(QStringLiteral("group")).toString();
        if (group.group.isEmpty() || !readStringList(o.value(QStringLiteral("items")), group.items)) {
            error = QStringLiteral("groupe %1 de la palette : nom ou items invalides").arg(i + 1);
            return false;
        }
        layout.palette.push_back(std::move(group));
    }

    const QJsonObject shortcuts = root.value(QStringLiteral("shortcuts")).toObject();
    for (auto it = shortcuts.constBegin(); it != shortcuts.constEnd(); ++it) {
        if (!it.value().isString()) {
            error = QStringLiteral("raccourci de %1 : texte attendu").arg(it.key());
            return false;
        }
        layout.shortcuts.insert(it.key(), it.value().toString());
    }

    if (!readStringList(root.value(QStringLiteral("colors")), layout.colors)) {
        error = QStringLiteral("couleur invalide : liste de textes attendue");
        return false;
    }
    for (const QString& color : std::as_const(layout.colors)) {
        if (!argumentIsValid(ArgumentKind::Color, color)) {
            error = QStringLiteral("couleur invalide « %1 »").arg(color);
            return false;
        }
    }

    const QJsonArray widths = root.value(QStringLiteral("penWidths")).toArray();
    for (const QJsonValue& width : widths) {
        const double value = width.toDouble(-1);
        if (!width.isDouble() || value <= 0 || value > 100) {
            error = QStringLiteral("épaisseur invalide (attendu un nombre de 1 à 100)");
            return false;
        }
        layout.penWidths << value;
    }

    for (const char* key : {"objectBar", "contextMenu"}) {
        QStringList& target = key[0] == 'o' ? layout.objectBar : layout.contextMenu;
        if (!readStringList(root.value(QLatin1String(key)), target)) {
            error = QStringLiteral("%1 : tableau de chaînes attendu").arg(QLatin1String(key));
            return false;
        }
    }

    out = std::move(layout);
    return true;
}

QByteArray readLayeredFile(const QString& fileName, const QString& profileDir, const QString& resourceDir,
                           QString* usedPath)
{
    for (const QString& dir : {profileDir, resourceDir}) {
        if (dir.isEmpty()) {
            continue;
        }
        const QString path = dir + QLatin1Char('/') + fileName;
        QFile file(path);
        if (file.open(QIODevice::ReadOnly)) {
            if (usedPath) {
                *usedPath = path;
            }
            return file.readAll();
        }
    }
    if (usedPath) {
        usedPath->clear();
    }
    return {};
}

QStringList unknownCommands(const Layout& layout, const CommandRegistry& registry)
{
    QStringList unknown;
    const auto check = [&](const QString& id) {
        if (id == QLatin1String("-") || registry.find(id) || unknown.contains(id)) {
            return;
        }
        unknown << id;
    };
    for (const MenuSpec& menu : layout.menus) {
        for (const QString& id : menu.items) {
            if (!id.startsWith(QLatin1Char('>'))) { // « >id » désigne un sous-menu, pas une commande
                check(id);
            }
        }
        for (const MenuSpec& submenu : menu.submenus) {
            for (const QString& id : submenu.items) {
                check(id);
            }
        }
    }
    for (const QString& id : layout.objectBar) {
        check(id);
    }
    for (const QString& id : layout.contextMenu) {
        check(id);
    }
    for (const PaletteGroup& group : layout.palette) {
        for (const QString& id : group.items) {
            check(id);
        }
    }
    for (auto it = layout.shortcuts.cbegin(); it != layout.shortcuts.cend(); ++it) {
        check(it.key());
    }
    return unknown;
}

} // namespace actcraft::actions
