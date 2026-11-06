#pragma once

#include <actcraft/actions/commandregistry.h>
#include <actcraft/actions/layout.h>

#include <QHash>
#include <QJSValue>
#include <QObject>
#include <QSet>
#include <QStringList>
#include <QVariant>
#include <QtQml/qqmlregistration.h>

#include <vector>

namespace actcraft::ui {

// Pont QML du registre de commandes (DA1) : déclarations et disposition lues au démarrage ; menus, palette et
// raccourcis filtrés par plateforme ; déclenchement par nom. Les gestionnaires sont des fonctions JavaScript
// (setHandler), appelées avec l'argument de la commande.
class Commands : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QVariantList menus READ menus NOTIFY layoutChanged)
    Q_PROPERTY(QVariantList palette READ palette NOTIFY layoutChanged)
    Q_PROPERTY(QVariantList objectBar READ objectBar NOTIFY layoutChanged)
    Q_PROPERTY(QVariantList contextMenu READ contextMenu NOTIFY layoutChanged)
    Q_PROPERTY(QVariantList objectMenu READ objectMenu NOTIFY layoutChanged)
    Q_PROPERTY(QVariantList shortcuts READ shortcuts NOTIFY layoutChanged)
    Q_PROPERTY(QStringList colors READ colors NOTIFY layoutChanged)
    Q_PROPERTY(QVariantList penWidths READ penWidths NOTIFY layoutChanged)
    Q_PROPERTY(QString platformName READ platformName NOTIFY layoutChanged)
    Q_PROPERTY(QStringList errors READ errors NOTIFY layoutChanged)
    Q_PROPERTY(int revision READ revision NOTIFY stateChanged)
    // Verrou de classe (D6-7), en lecture seule : seul ClassroomLock le pose et le lève (setLocked, C++).
    Q_PROPERTY(bool locked READ isLocked NOTIFY stateChanged)
    // Commandes disponibles grisées par le verrou (0 hors du verrou), lue par les scénarios.
    Q_PROPERTY(int lockedCount READ lockedCount NOTIFY stateChanged)

public:
    explicit Commands(QObject* parent = nullptr);

    // Tests : dossier du profil (défaut : <AppConfigLocation>/disposition) et plateforme ; rechargent.
    void setProfileDir(const QString& dir);
    void setPlatform(actions::Platform platform);
    // Relit déclarations et disposition ; garde les gestionnaires posés.
    Q_INVOKABLE void reload();

    // [{id, label, items: [{id, label, shortcut, checkable, separator: false} | {separator: true} |
    // {submenu: true, id, label, items: […]}]}] ; menus et sous-menus sans commande disponible omis ;
    // séparateurs jamais en tête, en fin ni doublés.
    QVariantList menus() const;
    // [{id, label, icon}] des commandes disponibles de la barre d'objet, dans l'ordre.
    QVariantList objectBar() const;
    // Menu contextuel de la page : mêmes éléments qu'un menu (séparateurs nettoyés comme au premier niveau).
    QVariantList contextMenu() const;
    QVariantList objectMenu() const;
    // [{group, items: [{id, label, icon, checkable}]}] ; « couleurs » et « epaisseur » gardés sans items.
    QVariantList palette() const;
    // [{id, sequence}] des commandes disponibles qui ont un raccourci.
    QVariantList shortcuts() const;
    QStringList colors() const;
    QVariantList penWidths() const;
    QString platformName() const;
    QStringList errors() const;
    int revision() const noexcept;
    const actions::CommandRegistry& registry() const noexcept;

    Q_INVOKABLE bool trigger(const QString& id, const QVariant& argument = QVariant());
    Q_INVOKABLE void setHandler(const QString& id, const QJSValue& handler);
    // Gestionnaire C++ (L6-T5 : `profile.activate`, posé par ProfileController), gardé à travers reload() et
    // prioritaire sur un gestionnaire JavaScript ; un gestionnaire vide le retire.
    void setNativeHandler(const QString& id, actions::CommandRegistry::Handler handler);
    Q_INVOKABLE QString label(const QString& id) const;
    // « qrc:/qt/qml/ActCraft/Ui/icons/<icône>.svg », ou vide.
    Q_INVOKABLE QString iconUrl(const QString& id) const;
    Q_INVOKABLE QString shortcut(const QString& id) const;
    Q_INVOKABLE bool isAvailable(const QString& id) const;
    // Retire (ou rend) une commande aux menus, barres et raccourcis sans l'ôter du registre : une
    // fonction que le poste ne peut pas rendre (convertisseur absent) n'est pas offerte. Les scénarios et
    // `trigger` la jouent encore ; son gestionnaire refuse visiblement.
    Q_INVOKABLE void setHidden(const QString& id, bool hidden);
    Q_INVOKABLE bool isEnabled(const QString& id) const;
    Q_INVOKABLE void setEnabled(const QString& id, bool enabled);
    Q_INVOKABLE bool isChecked(const QString& id) const;
    Q_INVOKABLE void setChecked(const QString& id, bool checked);
    // Verrou de classe du registre (D6-7) : désactive les commandes verrouillables sans toucher leur état
    // propre ; gardé à travers reload(). C++ seulement (L6-T31) : aucune voie QML ne lève le verrou sans le
    // mot de passe ; seul ClassroomLock l'appelle.
    void setLocked(bool locked);
    bool isLocked() const noexcept;
    int lockedCount() const;
    // Amendement 12 : un geste qui ne passe pas par le registre (barre d'onglets, navigateur de pages) suit
    // sa commande sous verrou ; hors du verrou, il ne dépend pas d'elle. C++ seulement.
    bool allowsWhileLocked(const QString& id) const;
    // Idem pour un possesseur dont les commandes peuvent manquer (absentes : permis).
    static bool allowsWhileLocked(const Commands* commands, const char* id);
    // Diffusion protégée (L8-T32) : identifiants refusés par les règles du document ouvert, posés par la
    // seule BroadcastGate (C++), gardés à travers reload() ; l'état propre des commandes n'est pas touché.
    void setRestricted(const QSet<QString>& ids);
    bool isRestricted(const QString& id) const;
    // Un geste hors registre refusé par la diffusion (glisser vers les ressources) : même signal qu'un essai.
    void reportRestrictionRefusal(const QString& id);

signals:
    void layoutChanged();
    void stateChanged();
    void triggered(const QString& id, const QVariant& argument);
    // Refus de la diffusion protégée : essai d'une commande restreinte par trigger (menu, raccourci), ou
    // geste hors trigger signalé par reportRestrictionRefusal (action d'objet, ressources, onglets).
    void refusedByRestriction(const QString& id);

private:
    // Éléments d'un menu : `submenus` nul = pas de sous-menu possible (sous-menu lui-même, menu contextuel).
    QVariantList menuItems(const QStringList& tokens, const std::vector<actions::MenuSpec>* submenus) const;
    void bindHandler(const QString& id);
    void bumpState();

    actions::CommandRegistry m_registry;
    actions::Layout m_layout;
    actions::Platform m_platform;
    QString m_profileDir;
    QStringList m_errors;
    QSet<QString> m_hidden;
    QHash<QString, QJSValue> m_handlers;
    QHash<QString, actions::CommandRegistry::Handler> m_nativeHandlers;
    // Gestionnaire en cours : le registre qui le porte n'est ni remplacé ni modifié avant son retour.
    int m_triggerDepth = 0;
    bool m_reloadPending = false;
    int m_revision = 0;
};

} // namespace actcraft::ui
