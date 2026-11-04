#pragma once

#include <QObject>
#include <QSettings>
#include <QStringList>
#include <QVariant>
#include <QtQml/qqmlregistration.h>

#include <memory>

namespace actcraft::ui {

// Réglages propres au poste (D6-1), clés kebab-case à nous. Le profil, lui, est dans ProfileController.
// Créable en QML (Main.qml : `Preferences { id: appPreferences }`) : QSettings au format par défaut.
class Preferences : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QString currentProfile READ currentProfile WRITE setCurrentProfile NOTIFY changed)
    Q_PROPERTY(bool dashboardAtStart READ dashboardAtStart WRITE setDashboardAtStart NOTIFY changed)
    Q_PROPERTY(QString language READ language WRITE setLanguage NOTIFY changed)

public:
    static constexpr const char* kCurrentProfile = "workstation/current-profile";
    static constexpr const char* kDashboardAtStart = "workstation/dashboard-at-start";
    static constexpr const char* kLanguage = "workstation/language";
    static constexpr const char* kRecent = "workstation/recent-documents";

    // QSettings() : format par défaut (jamais NativeFormat explicite), organisation et application de
    // l'application. Le joueur, le banc et les tests posent le format INI et son dossier avant tout objet.
    explicit Preferences(QObject* parent = nullptr);
    Preferences(const QString& iniPath, QObject* parent); // tests : fichier INI
    ~Preferences() override;

    QString currentProfile() const;
    void setCurrentProfile(const QString& id);
    bool dashboardAtStart() const; // vrai par défaut (F-090)
    void setDashboardAtStart(bool on);
    QString language() const; // vide : langue du système
    void setLanguage(const QString& code);

    QVariant value(const QString& key, const QVariant& fallback = {}) const;
    void setValue(const QString& key, const QVariant& value);
    void remove(const QString& key);
    QStringList allKeys() const;

signals:
    void changed();

private:
    std::unique_ptr<QSettings> m_settings;
};

} // namespace actcraft::ui
