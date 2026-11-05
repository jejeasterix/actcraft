#include "actcraft/ui/networksettings.h"

#include "actcraft/ui/preferences.h"
#include "actcraft/ui/secretstore.h"

#include <QNetworkProxy>

namespace actcraft::ui {

namespace {

QString key(const char* name)
{
    return QString::fromLatin1(name);
}

// Une valeur vide efface la clé : « aucun mandataire » ne laisse rien au poste.
void store(Preferences& preferences, const char* name, const QString& value)
{
    if (value.isEmpty())
        preferences.remove(key(name));
    else
        preferences.setValue(key(name), value);
}

} // namespace

NetworkSettings::NetworkSettings(QObject* parent)
    : NetworkSettings(nullptr, QString::fromLatin1(kSecret), parent)
{}

NetworkSettings::NetworkSettings(Preferences* preferences, const QString& secretName, QObject* parent)
    : QObject(parent), m_ownPreferences(preferences ? nullptr : std::make_unique<Preferences>()),
      m_preferences(preferences ? preferences : m_ownPreferences.get()), m_secretName(secretName)
{
    apply(); // au démarrage : le mandataire gardé, ou celui du système
}

NetworkSettings::~NetworkSettings() = default;

QString NetworkSettings::host() const
{
    return m_preferences->value(key(kHost)).toString();
}

void NetworkSettings::setHost(const QString& value)
{
    const QString trimmed = value.trimmed();
    if (trimmed == host())
        return;
    store(*m_preferences, kHost, trimmed);
    emit changed();
    apply();
}

int NetworkSettings::port() const
{
    bool ok = false;
    const int value = m_preferences->value(key(kPort)).toInt(&ok);
    return ok && value >= 1 && value <= 65535 ? value : kDefaultPort;
}

void NetworkSettings::setPort(int value)
{
    if (value < 1 || value > 65535 || value == port())
        return;
    m_preferences->setValue(key(kPort), value);
    emit changed();
    apply();
}

QString NetworkSettings::user() const
{
    return m_preferences->value(key(kUser)).toString();
}

void NetworkSettings::setUser(const QString& value)
{
    if (value == user())
        return;
    store(*m_preferences, kUser, value);
    emit changed();
    apply();
}

bool NetworkSettings::available() const
{
    return SecretStore::available();
}

bool NetworkSettings::setPassword(const QString& password)
{
    // Trousseau seulement : sans trousseau (hors Windows), le mot de passe n'est gardé nulle part, et le
    // dire. Un mot de passe vide n'a rien à garder : l'effacer n'échoue jamais.
    bool kept = true;
    if (password.isEmpty())
        SecretStore::remove(m_secretName);
    else
        kept = SecretStore::write(m_secretName, password.toUtf8());
    apply();
    return kept;
}

void NetworkSettings::apply()
{
    const QString proxyHost = host();
    if (proxyHost.isEmpty()) {
        // Aucun mandataire posé par ActCraft : la configuration du système (GPO, WPAD) reprend la main.
        QNetworkProxyFactory::setUseSystemConfiguration(true);
        return;
    }
    const QString password = QString::fromUtf8(SecretStore::read(m_secretName).value_or(QByteArray()));
    QNetworkProxy::setApplicationProxy(
        QNetworkProxy(QNetworkProxy::HttpProxy, proxyHost, static_cast<quint16>(port()), user(), password));
}

void NetworkSettings::clear()
{
    m_preferences->remove(key(kHost));
    m_preferences->remove(key(kPort));
    m_preferences->remove(key(kUser));
    SecretStore::remove(m_secretName);
    emit changed();
    apply();
}

} // namespace actcraft::ui
