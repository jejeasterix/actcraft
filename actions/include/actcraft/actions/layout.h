#pragma once

#include <actcraft/actions/commandregistry.h>

#include <QByteArray>
#include <QHash>
#include <QList>
#include <QString>
#include <QStringList>

#include <vector>

namespace actcraft::actions {

// Un menu de la ligne supérieure : identifiants de commandes dans l'ordre ; "-" = séparateur.
struct MenuSpec
{
    QString id;
    QString label;
    QStringList items; // identifiants, « - » (séparateur) ou « >id » (sous-menu `id` de `submenus`)
    std::vector<MenuSpec> submenus; // un seul niveau (« Éditer ▸ Effacer »)
};

// Un groupe de la palette. « couleurs » et « epaisseur » sont des groupes spéciaux, sans commandes :
// l'interface les dessine à partir de Layout::colors et Layout::penWidths.
struct PaletteGroup
{
    QString group;
    QStringList items;
};

// Disposition de l'interface (règle 3 du BRIEF, DA1), format « actcraft-layout/1 ».
struct Layout
{
    std::vector<MenuSpec> menus;
    std::vector<PaletteGroup> palette;
    QHash<QString, QString> shortcuts; // id → séquence qui remplace celle de la commande ("" la retire)
    QStringList colors;                // pastilles « #RRGGBB » (F-133)
    QList<double> penWidths;           // préréglages du stylo, pixels de page (F-137)
    QStringList objectBar;             // barre d'objet de la sélection (F-521), dans l'ordre
    QStringList contextMenu;           // menu contextuel de la page (F-202), dans l'ordre ; "-" séparateur ; « >id » : sous-menu
    std::vector<MenuSpec> contextSubmenus; // sous-menus du menu contextuel (jeton « >id »)
};

// Tout ou rien : `out` n'est modifiée qu'en cas de succès.
bool parseLayout(const QByteArray& json, Layout& out, QString& error);
// Contenu de `fileName` lu dans `profileDir` s'il y existe, sinon dans `resourceDir` (N2-5) ; vide si absent
// des deux. `usedPath` reçoit le chemin lu (vide si rien n'a été lu).
QByteArray readLayeredFile(const QString& fileName, const QString& profileDir, const QString& resourceDir,
                           QString* usedPath = nullptr);
// Identifiants de la disposition (menus et sous-menus, palette, raccourcis, barre d'objet, menu contextuel)
// que le registre ne déclare pas, sans doublon, dans l'ordre de rencontre.
QStringList unknownCommands(const Layout& layout, const CommandRegistry& registry);

} // namespace actcraft::actions
