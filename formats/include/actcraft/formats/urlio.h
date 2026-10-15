#pragma once

#include <QByteArray>
#include <QIODevice>
#include <QString>
#include <QUrl>
#include <QVariant>

#include <optional>

namespace actcraft::formats {

// Lecture et écriture d'octets par une URL (décision 17) : fichier local, ou content:// d'Android (QFile sait
// ouvrir ces URL sous Android). Aucun autre schéma.
struct UrlReadResult
{
    bool ok = false;
    QString error;
    QByteArray data;
};

UrlReadResult readUrl(const QUrl& url, qint64 maxBytes = 1024ll * 1024 * 1024);
// Lit `device` par blocs de 1 Mio jusqu'à un bloc vide (jamais atEnd : taille inconnue d'un content://) ;
// nullopt et `error` au-delà de `maxBytes` ou sur une erreur de lecture. Ne réserve jamais `maxBytes`
// d'avance.
std::optional<QByteArray> readBounded(QIODevice& device, qint64 maxBytes, QString* error);
// Fichier local : temporaire voisin puis renommage (QSaveFile) — l'ancien fichier reste intact en cas
// d'échec. content:// : écriture complète en une passe (pas de dossier voisin inscriptible).
bool writeUrl(const QUrl& url, const QByteArray& data, QString* error);
// Argument d'une commande de fichier (décision 20) : QUrl telle quelle ; texte « content:… » ou « file:… » lu
// comme URL ; autre texte = chemin local (relatif au dossier courant). Vide si l'argument est absent ou vide.
QUrl urlFromArgument(const QVariant& argument);

} // namespace actcraft::formats
