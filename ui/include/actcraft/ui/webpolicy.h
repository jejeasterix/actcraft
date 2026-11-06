#pragma once

#include <QSet>
#include <QString>
#include <QUrl>

namespace actcraft::ui {

// Politique de contenu du cadre web (N1-A3), en logique pure : chaque requête de la vue (adresse de départ,
// redirection, sous-ressource) passe par decide(), jamais une règle posée une fois dans le QML de la vue.
// Schémas permis : `file` local dans le dossier des ressources du document, `http`, `https`, `about:blank`,
// `data:` d'images ; tout le reste est refusé. Un contenu distant n'est chargé que si son origine a été
// saisie par l'utilisateur dans cette session (seule voie distante d'un document créé dans la session) ; dans
// un document reçu, il est bloqué jusqu'au déblocage de sa page, page par page et document par document.
class WebContentPolicy
{
public:
    enum class Verdict
    {
        Load,
        Refuse,
        Blocked
    };

    void setResourceFolder(const QString& folder); // dossier des ressources du document ouvert
    void setReceived(bool received);               // vrai : document ouvert depuis un fichier
    void typedByUser(const QUrl& address);         // adresse saisie dans la boîte du cadre (session)
    void unblock(const QString& documentKey, int page);
    Verdict decide(const QUrl& url, const QString& documentKey, int page) const;
    // JavaScript coupé, sans option pour l'activer dans ce lot.
    static bool javascriptEnabled() noexcept { return false; }

private:
    QString m_folder;
    bool m_received = false;
    QSet<QString> m_typed;     // origines saisies : « schéma://hôte:port »
    QSet<QString> m_unblocked; // « clé#page »
};

} // namespace actcraft::ui
