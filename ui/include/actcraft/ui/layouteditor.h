#pragma once

#include <actcraft/ui/commands.h>          // types complets exigés par moc pour les propriétés
#include <actcraft/ui/profilecontroller.h> // idem

#include <QHash>
#include <QJsonArray>
#include <QJsonObject>
#include <QMetaObject>
#include <QObject>
#include <QPointer>
#include <QString>
#include <QVariantList>
#include <QtQml/qqmlregistration.h>

namespace actcraft::ui {

// Onglet Commandes de l'éditeur de profils (F-910). Lit la disposition effective (layout.json du profil
// courant, sinon celle des ressources ; A1), présente le catalogue des commandes disponibles sur la
// plateforme et le contenu ordonné d'une cible : palette principale (un en-tête par groupe, présenté en
// séparateur), barre d'objet, menu contextuel, menu de l'objet, chaque menu. apply() (« Terminé ») relit le
// JSON effectif, n'y remplace que les listes des cibles modifiées, écrit seulement <dossier du
// profil>/layout.json (jamais le fichier des ressources), puis recharge les commandes. Les clés et les
// entrées que l'éditeur ne connaît pas (sous-menus en ligne, clés ajoutées, commandes d'une autre plateforme)
// sont gardées telles qu'écrites.
class LayoutEditor : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(actcraft::ui::Commands* commands READ commands WRITE setCommands NOTIFY commandsChanged)
    Q_PROPERTY(actcraft::ui::ProfileController* profile READ profile WRITE setProfile NOTIFY profileChanged)
    Q_PROPERTY(QVariantList targets READ targets NOTIFY targetsChanged) // [{id, label}]
    Q_PROPERTY(QString target READ target WRITE setTarget NOTIFY targetChanged)
    // [{id, label, icon, available, separator, submenu, group}] : contenu de la cible, dans l'ordre.
    Q_PROPERTY(QVariantList items READ items NOTIFY itemsChanged)
    Q_PROPERTY(QVariantList catalog READ catalog NOTIFY catalogChanged) // [{id, label, icon, category}]
    Q_PROPERTY(bool separatorAllowed READ separatorAllowed NOTIFY targetChanged)
    Q_PROPERTY(bool modified READ modified NOTIFY itemsChanged)

public:
    explicit LayoutEditor(QObject* parent = nullptr);

    Commands* commands() const;
    void setCommands(Commands* commands);
    ProfileController* profile() const;
    void setProfile(ProfileController* profile);

    QVariantList targets() const;
    QString target() const;
    void setTarget(const QString& id);
    QVariantList items() const;
    QVariantList catalog() const;
    bool separatorAllowed() const;
    bool modified() const noexcept;

    // Faux, rien de changé : commande inconnue, absente de la plateforme, cachée ou déjà dans la cible.
    Q_INVOKABLE bool add(const QString& id);
    Q_INVOKABLE bool removeAt(int index);
    Q_INVOKABLE bool moveUp(int index);
    Q_INVOKABLE bool moveDown(int index);
    // Menus : « - » ; palette : un groupe neuf ; barre d'objet : refusé (le format n'y a pas de séparateur).
    Q_INVOKABLE bool addSeparator();
    // Relit la disposition effective et abandonne les modifications.
    Q_INVOKABLE void reload();
    // Faux, rien d'écrit : ni profil ni commandes, disposition effective illisible, résultat refusé par
    // actions::parseLayout ou écriture refusée.
    Q_INVOKABLE bool apply();

signals:
    void commandsChanged();
    void profileChanged();
    void targetsChanged();
    void targetChanged();
    void itemsChanged();
    void catalogChanged();

private:
    QString profileDir() const;
    QJsonObject readEffective() const;
    void rebuildTargets();
    void loadTarget();
    void setTokens(const QJsonArray& tokens);
    void onProfileChanged();

    QPointer<Commands> m_commands;
    QPointer<ProfileController> m_profile;
    QMetaObject::Connection m_layoutConnection;
    QMetaObject::Connection m_profileConnection;
    QString m_profileId;
    QJsonObject m_effective;
    QVariantList m_targets;
    QString m_target;
    QJsonArray m_tokens;                // jetons de la cible, tels qu'écrits dans le JSON
    QHash<QString, QJsonArray> m_edits; // cibles modifiées depuis la dernière lecture
};

} // namespace actcraft::ui
