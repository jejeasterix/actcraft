#pragma once

#include <QString>
#include <QUrl>

namespace actcraft::core {

// « \\hôte\… » ou « //hôte/… » : un chemin réseau, quel que soit le système (un document fait sous Windows
// porte des antislashs). Aucun accès au disque : sous Windows, toucher un chemin réseau envoie
// l'authentification du compte à l'hôte et peut figer l'interface.
inline bool isNetworkPath(QString path)
{
    path.replace(u'\\', u'/');
    return path.startsWith(QStringLiteral("//"));
}

// Idem pour une adresse de fichier : « file://hôte/… » et « file:////hôte/… » comptent aussi.
inline bool isNetworkPath(const QUrl& url)
{
    return !url.host().isEmpty() || isNetworkPath(url.toLocalFile());
}

} // namespace actcraft::core
