#include <actcraft/ui/language.h>
#include <actcraft/ui/preferences.h>

#include <QCoreApplication>
#include <QDir>
#include <QTranslator>

#include <algorithm>

namespace actcraft::ui {

namespace {

constexpr const char* kTranslationsDir = ":/actcraft/i18n";
const QString kFallback = QStringLiteral("fr");

} // namespace

Language::Language(QObject* parent) : QObject(parent) {}

QStringList Language::available()
{
    QStringList codes;
    const QStringList files =
        QDir(QString::fromLatin1(kTranslationsDir)).entryList({QStringLiteral("actcraft_*.qm")}, QDir::Files);
    for (const QString& file : files)
        codes << file.mid(QStringLiteral("actcraft_").size()).chopped(3); // « actcraft_<code>.qm »
    std::sort(codes.begin(), codes.end());
    return codes;
}

QString Language::resolve(const QString& setting, const QStringList& available, const QLocale& system)
{
    const QString wanted = setting.isEmpty() ? system.name().section(QLatin1Char('_'), 0, 0) : setting;
    return available.contains(wanted) ? wanted : kFallback;
}

QString Language::effective(const Preferences& preferences)
{
    return resolve(preferences.language(), available(), QLocale()); // QLocale::setDefault le fixe aux essais
}

bool Language::install(QCoreApplication& app, Preferences& preferences)
{
    const QString code = effective(preferences);
    auto* translator = new QTranslator(&app);
    if (!translator->load(QStringLiteral("actcraft_%1.qm").arg(code),
                          QString::fromLatin1(kTranslationsDir)) ||
        !QCoreApplication::installTranslator(translator)) {
        delete translator;
        return false;
    }
    return true;
}

Preferences* Language::preferences() const
{
    return m_preferences;
}

void Language::setPreferences(Preferences* preferences)
{
    if (m_preferences == preferences)
        return;
    if (m_preferences)
        disconnect(m_preferences, &Preferences::changed, this, &Language::onPreferencesChanged);
    m_preferences = preferences;
    m_atStart = m_last = preferences ? preferences->language() : QString();
    if (preferences)
        connect(preferences, &Preferences::changed, this, &Language::onPreferencesChanged);
    if (m_pending) {
        m_pending = false;
        emit restartPendingChanged();
    }
    emit preferencesChanged();
}

bool Language::restartPending() const
{
    return m_pending;
}

void Language::change(const QString& code)
{
    if (m_preferences)
        m_preferences->setLanguage(code);
}

void Language::onPreferencesChanged()
{
    const QString now = m_preferences ? m_preferences->language() : QString();
    if (now == m_last)
        return;
    m_last = now;
    const bool pending = now != m_atStart;
    if (pending != m_pending) {
        m_pending = pending;
        emit restartPendingChanged();
    }
    if (pending)
        emit restartNeeded();
}

} // namespace actcraft::ui
