#include "actcraft/ui/launchers.h"

namespace actcraft::ui {

// Talon rouge (L6-T29) : rien n'est encore gardé, signé ni lancé.

Launchers::Launchers(QObject* parent)
    : Launchers(nullptr, QString::fromLatin1(kKeyName), parent)
{
}

Launchers::Launchers(ProfileController* profile, QString keyName, QObject* parent)
    : QObject(parent)
    , m_profile(profile)
    , m_keyName(std::move(keyName))
{
}

ProfileController* Launchers::profile() const
{
    return m_profile;
}

void Launchers::setProfile(ProfileController* profile)
{
    m_profile = profile;
}

QVariantList Launchers::entries() const
{
    return {};
}

QString Launchers::add(const QString&, const QStringList&, const QString&)
{
    return {};
}

bool Launchers::remove(const QString&)
{
    return false;
}

int Launchers::run(const QString&)
{
    return kFailed;
}

bool Launchers::confirm(const QString&, bool)
{
    return false;
}

QByteArray Launchers::signatureOf(const QString&, const QStringList&, const QString&, const QByteArray&)
{
    return {};
}

QList<Launchers::Entry> Launchers::readEntries() const
{
    return {};
}

QByteArray Launchers::key(bool) const
{
    return {};
}

bool Launchers::trusted(const Entry&, const QByteArray&) const
{
    return false;
}

bool Launchers::sign(const QString&, const QByteArray&)
{
    return false;
}

bool Launchers::launch(const Entry&)
{
    return false;
}

} // namespace actcraft::ui
