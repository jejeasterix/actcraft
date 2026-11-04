#pragma once

#include <QObject>
#include <QString>
#include <QtQml/qqmlregistration.h>

#include <memory>

namespace actcraft::ui {

class Preferences;

// Mandataire réseau du poste (F-930, section Réseau ; Windows). Hôte, port et compte dans les préférences du poste
// (`network/proxy-*`) ; mot de passe dans le trousseau (SecretStore) et nulle part ailleurs : ni QSettings, ni
// journal, ni message. Appliqué par QNetworkProxy::setApplicationProxy à la construction (démarrage) et à chaque
// changement ; sans hôte, aucun mandataire.
class NetworkSettings : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QString host READ host WRITE setHost NOTIFY changed)
    Q_PROPERTY(int port READ port WRITE setPort NOTIFY changed)
    Q_PROPERTY(QString user READ user WRITE setUser NOTIFY changed)
    Q_PROPERTY(bool available READ available CONSTANT)

public:
    static constexpr const char* kHost = "network/proxy-host";
    static constexpr const char* kPort = "network/proxy-port";
    static constexpr const char* kUser = "network/proxy-user";
    static constexpr const char* kSecret = "network-proxy"; // trousseau : ActCraft/network-proxy
    static constexpr int kDefaultPort = 8080;

    explicit NetworkSettings(QObject* parent = nullptr); // QML : préférences du poste, secret du poste
    // Tests : préférences et nom du secret fournis, le secret du poste n'est jamais touché.
    NetworkSettings(Preferences* preferences, const QString& secretName, QObject* parent = nullptr);
    ~NetworkSettings() override;

    QString host() const;
    void setHost(const QString& value);
    int port() const;
    void setPort(int value);
    QString user() const;
    void setUser(const QString& value);
    bool available() const; // trousseau présent (Windows)

    Q_INVOKABLE void setPassword(const QString& password); // trousseau seulement ; vide : effacé
    Q_INVOKABLE void apply();
    Q_INVOKABLE void clear(); // hôte, port, compte et mot de passe effacés : aucun mandataire

signals:
    void changed();

private:
    std::unique_ptr<Preferences> m_ownPreferences;
    Preferences* m_preferences = nullptr;
    QString m_secretName;
};

} // namespace actcraft::ui
