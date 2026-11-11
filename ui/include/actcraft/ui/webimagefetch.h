#pragma once

#include <actcraft/ui/webpolicy.h>

#include <QByteArray>
#include <QObject>
#include <QString>
#include <QUrl>

#include <functional>

class QNetworkAccessManager;
class QNetworkReply;

namespace actcraft::ui {

// Téléchargement d'une image glissée ou collée (cadrage n° 23 à 26), sur le fil de l'interface et sans
// blocage : le décodage reste au fil de travail d'Insertion. Une seule requête par objet. `decide` vide :
// gardes coupées. Une vidéo glissée (OA27) passe par le même chemin, avec une borne plus large.
class WebImageFetch final : public QObject
{
    Q_OBJECT
public:
    using Decide = std::function<WebContentPolicy::Verdict(const QUrl&)>;
    static constexpr qint64 kMaxBytes = 25 * 1024 * 1024;
    static constexpr int kTimeoutMs = 20000; // sans transfert ; comme kTransferTimeoutMs du cadre
    static constexpr qint64 kMaxVideoBytes = 100 * 1024 * 1024;
    static constexpr int kMaxRedirects = 10;
    enum class Kind
    {
        Image,
        Video
    };

    explicit WebImageFetch(QObject* parent = nullptr);
    void start(const QUrl& url, Decide decide = {}, Kind kind = Kind::Image);
    // Type MIME de la dernière réponse, en minuscules et sans paramètres ; vide sans réponse. Lu à
    // `finished`.
    QString contentType() const { return m_contentType; }

signals:
    void finished(const QByteArray& data, const QString& error); // error vide : succès

private:
    void request(const QUrl& url);
    void answered(QNetworkReply* reply);
    void fail(const QString& error);

    QNetworkAccessManager* m_network = nullptr;
    Decide m_decide;
    Kind m_kind = Kind::Image;
    QString m_contentType;
    int m_redirects = 0;
    bool m_tooLarge = false;
    bool m_tooLargeVideo = false;
};

} // namespace actcraft::ui
