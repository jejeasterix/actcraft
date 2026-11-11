#include <actcraft/ui/webimagefetch.h>

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QTimer>

namespace actcraft::ui {

namespace {
bool isWeb(const QUrl& url)
{
    return url.scheme() == QLatin1String("http") || url.scheme() == QLatin1String("https");
}
} // namespace

WebImageFetch::WebImageFetch(QObject* parent) : QObject(parent) {}

void WebImageFetch::start(const QUrl& url, Decide decide)
{
    m_decide = std::move(decide);
    m_redirects = 0;
    m_tooLarge = false;
    if (m_decide && m_decide(url) != WebContentPolicy::Verdict::Load) {
        // Différé : l'appelant a le temps de se brancher sur `finished`.
        QTimer::singleShot(0, this, [this] { fail(tr("Adresse refusée par la politique du cadre web.")); });
        return;
    }
    request(url);
}

void WebImageFetch::request(const QUrl& url)
{
    if (!m_network)
        m_network = new QNetworkAccessManager(this);
    QNetworkRequest request(url);
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::ManualRedirectPolicy);
    request.setAttribute(QNetworkRequest::CookieSaveControlAttribute, QNetworkRequest::Manual);
    request.setAttribute(QNetworkRequest::CookieLoadControlAttribute, QNetworkRequest::Manual);
    request.setTransferTimeout(kTimeoutMs);
    QNetworkReply* reply = m_network->get(request);
    connect(reply, &QNetworkReply::downloadProgress, this, [this, reply](qint64 received, qint64 total) {
        // Taille annoncée : refus dès le premier octet, sans attendre 25 Mo ni la fin de la connexion. Pour
        // une réponse 2xx seulement : une redirection ou une erreur annonçant un gros corps garde son sort.
        const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        const bool announcedTooLarge = status >= 200 && status < 300 && total > kMaxBytes;
        if (received > kMaxBytes || announcedTooLarge) {
            m_tooLarge = true;
            reply->abort();
        }
    });
    connect(reply, &QNetworkReply::finished, this, [this, reply] { answered(reply); });
}

void WebImageFetch::answered(QNetworkReply* reply)
{
    reply->deleteLater();
    if (m_tooLarge) {
        fail(tr("Image trop lourde (plus de 25 Mo)."));
        return;
    }
    const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    const QUrl target = reply->attribute(QNetworkRequest::RedirectionTargetAttribute).toUrl();
    if (status >= 300 && status < 400 && target.isValid()) {
        const QUrl next = reply->url().resolved(target);
        const bool lessSafe =
            reply->url().scheme() == QLatin1String("https") && next.scheme() != QLatin1String("https");
        if (++m_redirects > kMaxRedirects || !isWeb(next) || lessSafe ||
            (m_decide && m_decide(next) != WebContentPolicy::Verdict::Load)) {
            fail(tr("Redirection refusée."));
            return;
        }
        request(next);
        return;
    }
    if (status >= 400) {
        fail(tr("Image indisponible (erreur %1).").arg(status));
        return;
    }
    if (reply->error() != QNetworkReply::NoError) {
        fail(tr("Image indisponible : %1").arg(reply->errorString()));
        return;
    }
    emit finished(reply->readAll(), QString());
}

void WebImageFetch::fail(const QString& error)
{
    emit finished(QByteArray(), error);
}

} // namespace actcraft::ui
