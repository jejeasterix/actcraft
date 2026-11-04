#include "actcraft/ui/networksettings.h"

#include "actcraft/ui/preferences.h"
#include "actcraft/ui/secretstore.h"

namespace actcraft::ui {

// Talon rouge (L6-T28, étape 2) : rien n'est gardé, rien n'est appliqué.

NetworkSettings::NetworkSettings(QObject* parent)
    : QObject(parent)
    , m_ownPreferences(std::make_unique<Preferences>())
    , m_preferences(m_ownPreferences.get())
    , m_secretName(QString::fromLatin1(kSecret))
{
}

NetworkSettings::NetworkSettings(Preferences* preferences, const QString& secretName, QObject* parent)
    : QObject(parent)
    , m_preferences(preferences)
    , m_secretName(secretName)
{
}

NetworkSettings::~NetworkSettings() = default;

QString NetworkSettings::host() const
{
    return {};
}

void NetworkSettings::setHost(const QString&) {}

int NetworkSettings::port() const
{
    return kDefaultPort;
}

void NetworkSettings::setPort(int) {}

QString NetworkSettings::user() const
{
    return {};
}

void NetworkSettings::setUser(const QString&) {}

bool NetworkSettings::available() const
{
    return SecretStore::available();
}

void NetworkSettings::setPassword(const QString&) {}

void NetworkSettings::apply() {}

void NetworkSettings::clear() {}

} // namespace actcraft::ui
