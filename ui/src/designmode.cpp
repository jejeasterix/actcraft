#include <actcraft/ui/designmode.h>

#include <QString>
#include <QVariant>

namespace actcraft::ui {

namespace {

// Clés de la section `design-mode` du profil, dans l'ordre de DesignMode::Setting.
constexpr std::array<const char*, DesignMode::kSettingCount> kKeys = {
    "editLocked",      "showHidden",  "highlightActions", "disableActions", "disableRestrictions",
    "disableBlockers", "disableSnap", "disableDesktop",   "showTips"};

QString commandId()
{
    return QStringLiteral("mode.design");
}

QString section()
{
    return QStringLiteral("design-mode");
}

std::size_t at(DesignMode::Setting which)
{
    return static_cast<std::size_t>(which);
}

} // namespace

DesignMode::DesignMode(QObject* parent) : QObject(parent)
{
    m_settings.fill(true);
}

DesignMode::~DesignMode()
{
    unbindCommands();
}

bool DesignMode::active() const noexcept
{
    return m_active;
}

void DesignMode::setActive(bool active)
{
    if (m_contactActive) {
        m_requested = active; // amendement 1 : appliqué au relâché, avec son seul redessin
        return;
    }
    apply(active);
}

void DesignMode::toggle()
{
    setActive(!m_requested.value_or(m_active));
}

bool DesignMode::contactActive() const noexcept
{
    return m_contactActive;
}

void DesignMode::setContactActive(bool active)
{
    if (m_contactActive == active) {
        return;
    }
    m_contactActive = active;
    emit contactActiveChanged();
    if (!active && m_requested) {
        apply(*m_requested);
    }
}

Commands* DesignMode::commands() const noexcept
{
    return m_commands;
}

void DesignMode::setCommands(Commands* commands)
{
    if (m_commands == commands) {
        return;
    }
    unbindCommands();
    m_commands = commands;
    bindCommands();
    emit wiringChanged();
}

ProfileController* DesignMode::profile() const noexcept
{
    return m_profile;
}

void DesignMode::setProfile(ProfileController* profile)
{
    if (m_profile == profile) {
        return;
    }
    if (m_profile) {
        disconnect(m_profile, nullptr, this, nullptr);
    }
    m_profile = profile;
    if (m_profile) {
        connect(m_profile, &ProfileController::profileChanged, this, &DesignMode::loadSettings);
    }
    loadSettings();
    emit wiringChanged();
}

bool DesignMode::setting(Setting which) const noexcept
{
    return m_settings[at(which)];
}

void DesignMode::setSetting(Setting which, bool value)
{
    Settings values = m_settings;
    values[at(which)] = value;
    if (m_profile) {
        m_profile->setSetting(section(), QString::fromLatin1(kKeys[at(which)]), value);
    }
    changeSettings(values);
}

unsigned DesignMode::effectMask() const noexcept
{
    if (!m_active) {
        return 0;
    }
    unsigned mask = 1U << kSettingCount; // transitionsSuspended
    for (std::size_t i = 0; i < kSettingCount; ++i) {
        if (m_settings[i]) {
            mask |= 1U << i;
        }
    }
    return mask;
}

void DesignMode::apply(bool active)
{
    m_requested.reset();
    if (m_active == active) {
        return;
    }
    const unsigned before = effectMask();
    m_active = active;
    if (m_commands) {
        m_commands->setChecked(commandId(), m_active);
    }
    emit activeChanged();
    if (effectMask() != before) {
        emit effectsChanged();
    }
}

void DesignMode::loadSettings()
{
    Settings values{};
    for (std::size_t i = 0; i < kSettingCount; ++i) {
        values[i] = !m_profile || m_profile->setting(section(), QString::fromLatin1(kKeys[i]), true).toBool();
    }
    changeSettings(values);
}

void DesignMode::changeSettings(const Settings& values)
{
    if (values == m_settings) {
        return;
    }
    const unsigned before = effectMask();
    m_settings = values;
    emit settingsChanged();
    if (effectMask() != before) {
        emit effectsChanged();
    }
}

void DesignMode::bindCommands()
{
    if (!m_commands) {
        return;
    }
    m_commands->setNativeHandler(commandId(), [this](const QVariant&) { toggle(); });
    m_commands->setChecked(commandId(), m_active);
}

void DesignMode::unbindCommands()
{
    if (m_commands) {
        m_commands->setNativeHandler(commandId(), {});
    }
}

} // namespace actcraft::ui
