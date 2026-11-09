#pragma once

#include <QByteArray>
#include <QHash>
#include <QHostAddress>
#include <QList>
#include <QObject>
#include <QPointer>
#include <QString>
#include <QStringList>
#include <QTcpServer>
#include <QTcpSocket>
#include <QUrl>

#include <utility>

// Serveur HTTP d'essai sur 127.0.0.1, port libre : compte les requêtes, sert une page ou une redirection.
class TestServer : public QObject
{
public:
    TestServer()
    {
        connect(&m_server, &QTcpServer::newConnection, this, [this] { accept(); });
        m_listening = m_server.listen(QHostAddress::LocalHost, 0);
    }
    bool listening() const { return m_listening; }
    QUrl url(const QString& path) const
    {
        return QUrl(QStringLiteral("http://127.0.0.1:%1%2").arg(m_server.serverPort()).arg(path));
    }
    QString origin() const { return QStringLiteral("127.0.0.1:%1/").arg(m_server.serverPort()); }
    void page(const QString& path, const QByteArray& html) { m_pages.insert(path, html); }
    void redirect(const QString& path, const QUrl& to) { m_redirects.insert(path, to); }
    int requests() const { return static_cast<int>(m_paths.size()); }
    QStringList paths() const { return m_paths; }
    void reset() { m_paths.clear(); }
    // Les requêtes de ce chemin sont comptées mais leur réponse est retenue jusqu'à release() : une réponse
    // en vol, que le test relâche à son heure.
    void hold(const QString& path) { m_holds.append(path); }
    void release()
    {
        m_holds.clear();
        for (const Held& held : std::as_const(m_held)) {
            if (!held.socket)
                continue;
            held.socket->write(held.response);
            held.socket->disconnectFromHost();
        }
        m_held.clear();
    }
    // Ligne d'en-tête ajoutée à la réponse de ce chemin (« Set-Cookie: k=v »).
    void header(const QString& path, const QByteArray& line) { m_headers.insert(path, line); }
    // Requête brute reçue en dernier pour ce chemin (tests de cookies).
    QByteArray lastRequest(const QString& path) const { return m_lastRequests.value(path); }
    // Réponse complète, en-têtes compris, envoyée telle quelle.
    void rawResponse(const QString& path, const QByteArray& response) { m_raw.insert(path, response); }

private:
    struct Held
    {
        QPointer<QTcpSocket> socket;
        QByteArray response;
    };

    void accept()
    {
        while (QTcpSocket* socket = m_server.nextPendingConnection()) {
            connect(socket, &QTcpSocket::disconnected, socket, &QObject::deleteLater);
            connect(socket, &QTcpSocket::readyRead, this, [this, socket] { read(socket); });
        }
    }
    void read(QTcpSocket* socket)
    {
        QByteArray& buffer = m_buffers[socket];
        buffer += socket->readAll();
        if (!buffer.contains("\r\n\r\n"))
            return;
        const QByteArray raw = buffer;
        const QList<QByteArray> line = buffer.left(buffer.indexOf("\r\n")).split(' ');
        m_buffers.remove(socket);
        const QString path = line.size() > 1 ? QString::fromLatin1(line.at(1)) : QString();
        m_paths.append(path);
        m_lastRequests.insert(path, raw);
        QByteArray response;
        if (m_raw.contains(path)) {
            response = m_raw.value(path);
        } else if (m_redirects.contains(path)) {
            response = "HTTP/1.1 302 Found\r\nLocation: " + m_redirects.value(path).toEncoded() +
                       "\r\nContent-Length: 0\r\nConnection: close\r\n\r\n";
        } else if (m_pages.contains(path)) {
            const QByteArray body = m_pages.value(path);
            const QByteArray extra =
                m_headers.contains(path) ? m_headers.value(path) + QByteArray("\r\n") : QByteArray();
            response =
                "HTTP/1.1 200 OK\r\n" + extra +
                "Content-Type: text/html; charset=utf-8\r\nCache-Control: no-store\r\nContent-Length: " +
                QByteArray::number(body.size()) + "\r\nConnection: close\r\n\r\n" + body;
        } else {
            response = "HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\nConnection: close\r\n\r\n";
        }
        if (m_holds.contains(path)) {
            m_held.append(Held{socket, response});
            return;
        }
        socket->write(response);
        socket->disconnectFromHost();
    }

    QStringList m_holds;
    QList<Held> m_held;
    QTcpServer m_server;
    bool m_listening = false;
    QHash<QString, QByteArray> m_pages;
    QHash<QString, QUrl> m_redirects;
    QHash<QTcpSocket*, QByteArray> m_buffers;
    QStringList m_paths;
    QHash<QString, QByteArray> m_headers;
    QHash<QString, QByteArray> m_lastRequests;
    QHash<QString, QByteArray> m_raw;
};
