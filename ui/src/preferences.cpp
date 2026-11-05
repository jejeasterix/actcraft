#include <actcraft/ui/preferences.h>

namespace actcraft::ui {

Preferences::Preferences(QObject* parent) : QObject(parent), m_settings(std::make_unique<QSettings>()) {}

Preferences::Preferences(const QString& iniPath, QObject* parent)
    : QObject(parent), m_settings(std::make_unique<QSettings>(iniPath, QSettings::IniFormat))
{}

Preferences::~Preferences() = default;

QString Preferences::currentProfile() const
{
    return m_settings->value(QString::fromLatin1(kCurrentProfile)).toString();
}

void Preferences::setCurrentProfile(const QString& id)
{
    setValue(QString::fromLatin1(kCurrentProfile), id);
}

bool Preferences::dashboardAtStart() const
{
    return m_settings->value(QString::fromLatin1(kDashboardAtStart), true).toBool();
}

void Preferences::setDashboardAtStart(bool on)
{
    setValue(QString::fromLatin1(kDashboardAtStart), on);
}

QString Preferences::language() const
{
    return m_settings->value(QString::fromLatin1(kLanguage)).toString();
}

void Preferences::setLanguage(const QString& code)
{
    setValue(QString::fromLatin1(kLanguage), code);
}

QVariant Preferences::value(const QString& key, const QVariant& fallback) const
{
    return m_settings->value(key, fallback);
}

void Preferences::setValue(const QString& key, const QVariant& value)
{
    if (m_settings->contains(key) && m_settings->value(key) == value) {
        return;
    }
    m_settings->setValue(key, value);
    emit changed();
}

void Preferences::remove(const QString& key)
{
    if (!m_settings->contains(key)) {
        return;
    }
    m_settings->remove(key);
    emit changed();
}

QStringList Preferences::allKeys() const
{
    return m_settings->allKeys();
}

} // namespace actcraft::ui
