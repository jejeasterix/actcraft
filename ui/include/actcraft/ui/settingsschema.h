#pragma once

#include <actcraft/ui/preferences.h>
#include <actcraft/ui/profilecontroller.h> // types complets exigés par moc pour les propriétés

#include <QObject>
#include <QPointer>
#include <QString>
#include <QVariant>
#include <QVariantList>
#include <QtQml/qqmlregistration.h>

namespace actcraft::ui {

// Onglet paramètres de l'éditeur de profils (F-930, F-935) : sections des fonctions incluses, réglages
// décrits en données à nous. Magasin de chaque réglage :
// - `profile` : ProfileController::setting(section, clé), dans le profil courant ;
// - `workstation` : la clé QSettings que son propriétaire emploie déjà (lot 7, Preferences, NetworkSettings),
//   reprise telle quelle et lue par Preferences::value : le schéma n'en crée aucune.
class SettingsSchema : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(actcraft::ui::ProfileController* profile READ profile WRITE setProfile NOTIFY profileChanged)
    Q_PROPERTY(actcraft::ui::Preferences* preferences READ preferences WRITE setPreferences NOTIFY
                   preferencesChanged)
    Q_PROPERTY(QVariantList sections READ sections CONSTANT) // [{id, label, platforms}]

public:
    static constexpr const char* kProfileStore = "profile";
    static constexpr const char* kWorkstationStore = "workstation";

    explicit SettingsSchema(QObject* parent = nullptr);

    ProfileController* profile() const;
    void setProfile(ProfileController* profile);
    Preferences* preferences() const;
    void setPreferences(Preferences* preferences);

    QVariantList sections() const;
    // [{key, label, type, defaultValue, min, max, choices, store}] ; type : bool, int, text, choice
    Q_INVOKABLE QVariantList entries(const QString& section) const;
    // Valeur du magasin du réglage, son défaut s'il n'y en a pas (ou si le magasin manque).
    Q_INVOKABLE QVariant value(const QString& section, const QString& key) const;
    // Sans effet pour un réglage inconnu ou un magasin absent.
    Q_INVOKABLE void setValue(const QString& section, const QString& key, const QVariant& value);

signals:
    void profileChanged();
    void preferencesChanged();
    void settingChanged(const QString& section, const QString& key);

private:
    QPointer<ProfileController> m_profile;
    QPointer<Preferences> m_preferences;
};

} // namespace actcraft::ui
