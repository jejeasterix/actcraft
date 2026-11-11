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
// gardes coupées.
class WebImageFetch final : public QObject
{
    Q_OBJECT
public:
    using Decide = std::function<WebContentPolicy::Verdict(const QUrl&)>;
    static constexpr qint64 kMaxBytes = 25 * 1024 * 1024;
    static constexpr int kTimeoutMs = 20000; // sans transfert ; comme kTransferTimeoutMs du cadre
    static constexpr int kMaxRedirects = 10;

    explicit WebImageFetch(QObject* parent = nullptr);
    void start(const QUrl& url, Decide decide = {});

signals:
    void finished(const QByteArray& data, const QString& error); // error vide : succès

private:
    void request(const QUrl& url);
    void answered(QNetworkReply* reply);
    void fail(const QString& error);

    QNetworkAccessManager* m_network = nullptr;
    Decide m_decide;
    int m_redirects = 0;
    bool m_tooLarge = false;
};

} // namespace actcraft::ui
