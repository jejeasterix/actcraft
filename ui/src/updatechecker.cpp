#include <actcraft/ui/updatechecker.h>

#include <QNetworkReply>

namespace actcraft::ui {

// Jalon rouge (L11-T21) : squelette qui reprend le patron interdit, une requête au démarrage sans condition.

UpdateChecker::UpdateChecker(QObject* parent) : QObject(parent) {}

UpdateChecker::~UpdateChecker() = default;

bool UpdateChecker::isNewer(QStringView, QStringView)
{
    return false;
}

QUrl UpdateChecker::endpoint()
{
    return QUrl(QString::fromLatin1(kDefaultEndpoint));
}

void UpdateChecker::setNetworkAccessManager(QNetworkAccessManager* manager)
{
    m_manager = manager;
}

QNetworkAccessManager* UpdateChecker::manager()
{
    return m_manager;
}

void UpdateChecker::check()
{
    ++m_requestCount;
    emit stateChanged();
}

void UpdateChecker::start()
{
    check();
}

void UpdateChecker::finish(QNetworkReply*) {}

void UpdateChecker::settle(State, const QString&, const QUrl&) {}

Preferences* UpdateChecker::preferences() const
{
    return m_preferences;
}

void UpdateChecker::setPreferences(Preferences* preferences)
{
    if (m_preferences == preferences) {
        return;
    }
    m_preferences = preferences;
    emit preferencesChanged();
}

UpdateChecker::State UpdateChecker::state() const
{
    return m_state;
}

QString UpdateChecker::latestVersion() const
{
    return m_latestVersion;
}

QUrl UpdateChecker::releasePage() const
{
    return m_releasePage;
}

bool UpdateChecker::checkAtStart() const
{
    return false;
}

void UpdateChecker::setCheckAtStart(bool) {}

int UpdateChecker::requestCount() const
{
    return m_requestCount;
}

} // namespace actcraft::ui
