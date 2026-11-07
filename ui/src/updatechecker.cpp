#include <actcraft/ui/updatechecker.h>

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QNetworkReply>
#include <QNetworkRequest>

#include <array>
#include <chrono>
#include <optional>

namespace actcraft::ui {

namespace {

constexpr const char* kReleasesPage = "https://github.com/jejeasterix/actcraft/releases/latest";
// Seule une page de release du dépôt s'ouvre dans le navigateur.
constexpr const char* kReleasePagePrefix = "https://github.com/jejeasterix/actcraft/releases/";
constexpr qint64 kMaxReplyBytes = 1024 * 1024; // une release fait quelques Kio
constexpr std::chrono::seconds kIdleTimeout(20);
constexpr std::chrono::seconds kTotalTimeout(30);

using Version = std::array<quint32, 3>;

QStringView withoutPrefix(QStringView text)
{
    text = text.trimmed();
    if (text.startsWith(u'v') || text.startsWith(u'V'))
        text = text.sliced(1);
    return text;
}

// « x.y.z » en chiffres, préfixe « v » toléré ; rien d'autre (« 2.0 », « 2.0.0-rc1 », « 2..0 » ne se lisent
// pas).
std::optional<Version> parseVersion(QStringView text)
{
    const QList<QStringView> parts = withoutPrefix(text).split(QChar(u'.'));
    if (parts.size() != 3)
        return std::nullopt;
    Version version{};
    for (std::size_t i = 0; i < version.size(); ++i) {
        const QStringView part = parts.at(static_cast<qsizetype>(i));
        if (part.isEmpty() || part.size() > 9)
            return std::nullopt;
        for (const QChar c : part) {
            if (c.unicode() < u'0' || c.unicode() > u'9')
                return std::nullopt;
        }
        version[i] = part.toUInt();
    }
    return version;
}

QString text(const Version& version)
{
    return QStringLiteral("%1.%2.%3").arg(version[0]).arg(version[1]).arg(version[2]);
}

bool isLoopback(const QUrl& url)
{
    return url.host() == QLatin1String("127.0.0.1") || url.host() == QLatin1String("localhost");
}

QUrl releasePageOf(const QString& raw)
{
    const QUrl page = QUrl(raw, QUrl::StrictMode).adjusted(QUrl::NormalizePathSegments);
    if (page.isValid() && page.toString().startsWith(QLatin1String(kReleasePagePrefix)))
        return page;
    return QUrl(QString::fromLatin1(kReleasesPage));
}

} // namespace

UpdateChecker::UpdateChecker(QObject* parent)
    : QObject(parent), m_currentVersion(QString::fromLatin1(ACTCRAFT_VERSION))
{
    // Borne de durée totale : un serveur ou un mandataire qui distille sa réponse ne garde pas Checking.
    m_deadline.setSingleShot(true);
    m_deadline.setInterval(kTotalTimeout);
    connect(&m_deadline, &QTimer::timeout, this, [this] {
        if (m_reply)
            m_reply->abort();
    });
}

UpdateChecker::~UpdateChecker()
{
    if (m_reply) {
        m_reply->disconnect(this);
        m_reply->abort();
        m_reply->deleteLater();
    }
}

bool UpdateChecker::isNewer(QStringView candidate, QStringView current)
{
    const std::optional<Version> a = parseVersion(candidate);
    const std::optional<Version> b = parseVersion(current);
    return a && b && *a > *b;
}

QUrl UpdateChecker::endpoint()
{
    const QUrl fallback(QString::fromLatin1(kDefaultEndpoint));
    const QString value = qEnvironmentVariable(kEndpointVariable).trimmed();
    if (value.isEmpty())
        return fallback;
    // Essais seulement : un fichier de la machine (sans hôte : aucun partage réseau), ou un serveur de la
    // boucle locale sur un port donné ; aucune autre adresse.
    const QUrl url(value);
    if (!url.isValid() || url.isRelative())
        return fallback;
    if (url.isLocalFile() && url.host().isEmpty())
        return url;
    if (url.scheme() == QLatin1String("http") && isLoopback(url) && url.port() > 0 &&
        url.userInfo().isEmpty())
        return url;
    return fallback;
}

void UpdateChecker::setNetworkAccessManager(QNetworkAccessManager* manager)
{
    m_manager = manager;
}

void UpdateChecker::setCurrentVersion(const QString& version)
{
    m_currentVersion = version;
}

QNetworkAccessManager* UpdateChecker::manager()
{
    if (m_manager)
        return m_manager;
    // Aucun mandataire propre à ce gestionnaire : il suit celui de l'application, que
    // NetworkSettings::apply() pose au démarrage (Main.qml) et à chaque changement ; sans hôte, la
    // configuration du système.
    if (!m_ownManager)
        m_ownManager = std::make_unique<QNetworkAccessManager>();
    return m_ownManager.get();
}

void UpdateChecker::check()
{
    if (m_state == State::Checking)
        return; // une vérification à la fois
    ++m_requestCount;
    settle(State::Checking);

    // Une lecture GET, sans jeton ni identifiant du poste ; aucun cookie lu ni gardé.
    QNetworkRequest request(endpoint());
    request.setRawHeader("Accept", "application/vnd.github+json");
    request.setAttribute(QNetworkRequest::CookieLoadControlAttribute, QNetworkRequest::Manual);
    request.setAttribute(QNetworkRequest::CookieSaveControlAttribute, QNetworkRequest::Manual);
    request.setTransferTimeout(kIdleTimeout);
    QNetworkReply* reply = manager()->get(request);
    m_reply = reply;
    connect(reply, &QNetworkReply::downloadProgress, this, [reply](qint64 received, qint64 total) {
        if (received > kMaxReplyBytes || total > kMaxReplyBytes)
            reply->abort();
    });
    connect(reply, &QNetworkReply::finished, this, [this, reply] { finish(reply); });
    m_deadline.start();
}

void UpdateChecker::start()
{
    // Aucune requête au démarrage tant que le poste ne l'a pas demandée (faux par défaut).
    if (checkAtStart())
        check();
}

void UpdateChecker::finish(QNetworkReply* reply)
{
    reply->deleteLater();
    if (reply != m_reply)
        return;
    m_reply = nullptr;
    m_deadline.stop();

    // Hors ligne, refusée (404 d'un dépôt privé sans jeton), trop longue ou illisible : impossible de
    // vérifier.
    // Une réponse coupée (taille, durée) peut finir sans code d'erreur mais fermée : ne pas la lire.
    if (reply->error() != QNetworkReply::NoError || !reply->isReadable()) {
        settle(State::Failed);
        return;
    }
    const QByteArray body = reply->read(kMaxReplyBytes + 1);
    if (body.size() > kMaxReplyBytes) {
        settle(State::Failed);
        return;
    }
    QJsonParseError parseError{};
    const QJsonDocument document = QJsonDocument::fromJson(body, &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        settle(State::Failed);
        return;
    }
    const QJsonObject release = document.object();
    if (release.value(QLatin1String("draft")).toBool() ||
        release.value(QLatin1String("prerelease")).toBool()) {
        settle(State::UpToDate); // brouillons et préversions ignorés
        return;
    }
    const std::optional<Version> latest = parseVersion(release.value(QLatin1String("tag_name")).toString());
    const std::optional<Version> current = parseVersion(m_currentVersion);
    if (!latest || !current) {
        settle(State::Failed);
        return;
    }
    if (*latest <= *current) {
        settle(State::UpToDate, text(*latest));
        return;
    }
    settle(State::Available, text(*latest),
           releasePageOf(release.value(QLatin1String("html_url")).toString()));
}

void UpdateChecker::settle(State state, const QString& version, const QUrl& page)
{
    m_state = state;
    m_latestVersion = version;
    m_releasePage = page;
    emit stateChanged();
}

Preferences* UpdateChecker::preferences() const
{
    return m_preferences;
}

void UpdateChecker::setPreferences(Preferences* preferences)
{
    if (m_preferences == preferences) {
        return;
    }
    const bool before = checkAtStart();
    m_preferences = preferences;
    emit preferencesChanged();
    if (checkAtStart() != before)
        emit checkAtStartChanged();
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
    return m_preferences && m_preferences->value(QString::fromLatin1(kCheckAtStart), false).toBool();
}

void UpdateChecker::setCheckAtStart(bool on)
{
    // Réglage du poste : sans préférences liées (Main.qml les lie), rien à garder.
    if (!m_preferences || on == checkAtStart())
        return;
    m_preferences->setValue(QString::fromLatin1(kCheckAtStart), on);
    emit checkAtStartChanged();
}

int UpdateChecker::requestCount() const
{
    return m_requestCount;
}

} // namespace actcraft::ui
