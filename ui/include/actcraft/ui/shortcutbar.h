#pragma once

#include <actcraft/ui/commands.h>
#include <actcraft/ui/documentcontroller.h>
#include <actcraft/ui/profilecontroller.h>

#include <QList>
#include <QObject>
#include <QString>
#include <QVariantList>
#include <QtQml/qqmlregistration.h>

namespace actcraft::ui {

// Barre de raccourcis de la palette (F-140, F-141) : outils lancés hors palette, rangés en tête dans
// l'historique du profil (huit au plus), et boutons d'objets glissés (charge du presse-papiers du lot 5
// gardée dans le profil, `shortcutObjects`). Fermé par défaut : sans commandes, profil ou document, rien ne
// s'ajoute.
class ShortcutBar : public QObject
{
    Q_OBJECT
    QML_NAMED_ELEMENT(ShortcutBarModel)
    Q_PROPERTY(actcraft::ui::ProfileController* profile READ profile WRITE setProfile NOTIFY changed)
    Q_PROPERTY(actcraft::ui::Commands* commands READ commands WRITE setCommands NOTIFY changed)
    Q_PROPERTY(actcraft::ui::DocumentController* document MEMBER m_document NOTIFY changed)
    Q_PROPERTY(QVariantList buttons READ buttons NOTIFY changed) // [{kind: "tool"|"object", id, label, icon}]
public:
    explicit ShortcutBar(QObject* parent = nullptr);
    ProfileController* profile() const noexcept;
    void setProfile(ProfileController* profile);
    Commands* commands() const noexcept;
    void setCommands(Commands* commands); // écoute Commands::triggered
    QVariantList buttons() const;
    // Appelé par la palette quand elle déclenche une commande : celle-là n'entre pas dans l'historique.
    Q_INVOKABLE void notePaletteTrigger(const QString& id);
    // F-141 : objets sélectionnés de la page courante → bouton ; rend son identifiant (vide si refusé).
    Q_INVOKABLE QString addObjects(const QList<quint64>& objectIds);
    Q_INVOKABLE bool insert(const QString& buttonId); // copie au centre de la page active, une étape
    Q_INVOKABLE void removeButton(const QString& buttonId);
signals:
    void changed();

private:
    void onTriggered(const QString& id);
    bool open() const; // commandes, profil et document présents, registre non verrouillé
    ProfileController* m_profile = nullptr;
    Commands* m_commands = nullptr;
    DocumentController* m_document = nullptr;
    QString m_lastPaletteTrigger;
};

} // namespace actcraft::ui
