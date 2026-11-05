#include <actcraft/ui/settingsschema.h>

namespace actcraft::ui {

SettingsSchema::SettingsSchema(QObject* parent) : QObject(parent) {}

ProfileController* SettingsSchema::profile() const
{
    return m_profile;
}

void SettingsSchema::setProfile(ProfileController* profile)
{
    if (m_profile == profile)
        return;
    m_profile = profile;
    emit profileChanged();
}

Preferences* SettingsSchema::preferences() const
{
    return m_preferences;
}

void SettingsSchema::setPreferences(Preferences* preferences)
{
    if (m_preferences == preferences)
        return;
    m_preferences = preferences;
    emit preferencesChanged();
}

QVariantList SettingsSchema::sections() const
{
    return {};
}

QVariantList SettingsSchema::entries(const QString& section) const
{
    Q_UNUSED(section)
    return {};
}

QVariant SettingsSchema::value(const QString& section, const QString& key) const
{
    Q_UNUSED(section)
    Q_UNUSED(key)
    return {};
}

void SettingsSchema::setValue(const QString& section, const QString& key, const QVariant& value)
{
    Q_UNUSED(section)
    Q_UNUSED(key)
    Q_UNUSED(value)
}

} // namespace actcraft::ui
