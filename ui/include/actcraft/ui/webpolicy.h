#pragma once

#include <actcraft/actions/commandregistry.h>

#include <QHash>
#include <QSet>
#include <QString>
#include <QUrl>

#include <cstdint>

namespace actcraft::ui {

// Politique de contenu du cadre web (N1-A3), en logique pure : chaque requête de la vue (adresse de départ,
// redirection, sous-ressource) passe par decide(), jamais une règle posée une fois dans le QML de la vue.
// Schémas permis : `file` local dans le dossier des ressources du document, `http`, `https`, `about:blank`,
// `data:` d'images ; tout le reste est refusé. Un contenu distant n'est chargé que si son origine a été
// saisie par l'utilisateur dans cette session, dans ce document (seule voie distante d'un document créé dans
// la session) ; dans un document reçu, il est bloqué jusqu'au déblocage de sa page, page par page.
//
// Politique par document (H8-13) : tout l'état (dossier, document reçu, origines saisies, pages débloquées)
// est rangé sous la clé du document ; rien ne passe d'un document à un autre. Emploi par le cadre (L8-T30) :
// à l'ouverture, setResourceFolder(clé, dossier) et setReceived(clé, vrai) pour un fichier ouvert ; à la
// saisie d'une adresse dans la boîte du cadre, typedByUser(clé, adresse) ; au clic de déblocage d'une page,
// unblock(clé, page) ; à chaque requête de la vue, decide(adresse, clé, page) ; à la fermeture du document,
// forget(clé). Une clé inconnue : document créé dans la session, sans dossier, rien de saisi.
//
// Risque accepté : une page débloquée, ou une origine saisie, peut viser une adresse de la machine ou du
// réseau local (127.0.0.1, 192.168.x.x) ; la politique ne distingue pas ces hôtes, l'utilisateur les a
// choisis par le déblocage ou la saisie. JavaScript coupé : une telle page ne peut pas sonder le réseau.
//
// Page : le déblocage est rangé sous un identifiant de page choisi par l'appelant, jamais sous un numéro, qui
// passe à une autre page quand on en retire ou en insère une avant elle (L8-C12). Android : aucune adresse
// distante, même saisie ou débloquée (H8-15 contre F-302 « A, local seulement »).
class WebContentPolicy
{
public:
    enum class Verdict
    {
        Load,
        Refuse,
        Blocked
    };

    void setResourceFolder(const QString& documentKey, const QString& folder); // ressources du document
    void setReceived(const QString& documentKey, bool received); // vrai : document ouvert depuis un fichier
    void typedByUser(const QString& documentKey,
                     const QUrl& address); // adresse saisie dans la boîte du cadre
    void unblock(const QString& documentKey, std::uint64_t page);
    void forget(const QString& documentKey); // document fermé : son état disparaît
    Verdict decide(const QUrl& url, const QString& documentKey, std::uint64_t page) const;
    // JavaScript coupé, sans option pour l'activer dans ce lot.
    static bool javascriptEnabled() noexcept { return false; }
    // Accès au disque faits par decide() depuis le démarrage (tests : un chemin hors du dossier n'en fait
    // aucun).
    static int diskQueries();
    // Plateforme de la politique : celle de la construction, injectable pour les tests (comme
    // Commands::setPlatform). Sous Android, toute adresse distante est refusée.
    void setPlatform(actions::Platform platform) noexcept { m_platform = platform; }
    actions::Platform platform() const noexcept { return m_platform; }
    static actions::Platform buildPlatform() noexcept;

private:
    struct DocumentState
    {
        QString folder;
        bool received = false;
        QSet<QString> typed;           // origines saisies : « schéma://hôte:port »
        QSet<std::uint64_t> unblocked; // pages débloquées (identifiants de page)
    };
    QHash<QString, DocumentState> m_documents;
    actions::Platform m_platform = buildPlatform();
};

} // namespace actcraft::ui
