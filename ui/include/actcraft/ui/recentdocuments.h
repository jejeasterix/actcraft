#pragma once

#include <actcraft/ui/documentportal.h>

#include <QHash>
#include <QObject>
#include <QPointer>
#include <QStringList>
#include <QThreadPool>
#include <QUrl>
#include <QVariantList>
#include <QVariantMap>
#include <QtQml/qqmlregistration.h>

#include <deque>

namespace actcraft::ui {

class DocumentController;
class Preferences;

// Documents récents du poste (F-700, D6-1) : `workstation/recent-documents`, dix au plus, le dernier ouvert
// ou enregistré en tête. Un fichier local absent est caché (non oublié). Une URL content:// (D11-9) garde sa
// permission par le portail dès l'ajout ; à la relecture de la liste, une entrée dont la permission est
// perdue est retirée (oubliée, permission rendue) et `staleRemoved` donne son nom affiché. Une entrée effacée
// ou poussée au-delà de dix rend sa permission. `portal` nul : celui de la plateforme. `preferences` nul :
// liste en mémoire seulement. Vignettes (L6-T23, amendement 11) : chaque entrée locale porte la page 1 de
// l'archive
// (`thumbnails/page-1.png`) en URL `data:`, lue hors du fil principal (patron de PageThumbnails::pump : une
// lecture à la fois, aucune pendant un contact). Fichier absent, illisible ou sans miniature : `thumbnail`
// reste vide, sans erreur.
class RecentDocuments : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(actcraft::ui::Preferences* preferences READ preferences WRITE setPreferences NOTIFY
                   preferencesChanged)
    Q_PROPERTY(actcraft::ui::DocumentPortal* portal READ portal WRITE setPortal NOTIFY portalChanged)
    Q_PROPERTY(QVariantList entries READ entries NOTIFY changed) // [{url, name, when}]
    Q_PROPERTY(int count READ count NOTIFY changed)
    // Vrai pendant un contact (stylet, doigt) sur la page : aucune lecture de vignette ne part alors.
    Q_PROPERTY(bool contactActive READ contactActive WRITE setContactActive NOTIFY contactActiveChanged)
    Q_PROPERTY(int thumbnailsRead READ thumbnailsRead NOTIFY changed)                // tests
    Q_PROPERTY(bool lastReadOffMainThread READ lastReadOffMainThread NOTIFY changed) // tests

public:
    static constexpr int kMax = 10;

    explicit RecentDocuments(QObject* parent = nullptr);
    ~RecentDocuments() override;

    Preferences* preferences() const { return m_preferences; }
    void setPreferences(Preferences* preferences);
    DocumentPortal* portal() const { return m_portal; }
    void setPortal(DocumentPortal* portal);
    QVariantList entries() const; // [{url, name, when, thumbnail}]
    int count() const;
    bool contactActive() const noexcept { return m_contactActive; }
    void setContactActive(bool active);
    int thumbnailsRead() const noexcept { return m_thumbnailsRead; }
    bool lastReadOffMainThread() const noexcept { return m_lastReadOffMainThread; }

    // Met `url` en tête (une URL déjà présente remonte) ; une URL vide est ignorée.
    Q_INVOKABLE void note(const QUrl& url);
    Q_INVOKABLE void clear();
    // Relit les permissions des URL content:// et retire les entrées perdues (affichage du panneau, reprise).
    Q_INVOKABLE void refresh();
    // Nom, pages, objets, ressources et taille du fichier, lus sur le modèle en mémoire (jamais en relisant
    // le fichier) ; `bytes` vaut 0 pour une URL qui n'est pas un fichier local.
    Q_INVOKABLE QVariantMap summary(actcraft::ui::DocumentController* document, const QUrl& url) const;

signals:
    void changed();
    void preferencesChanged();
    void contactActiveChanged();
    void portalChanged();
    // Entrée content:// retirée faute de permission ; `name` est le nom affiché, jamais l'URL.
    void staleRemoved(const QString& name);

private:
    struct Thumbnail
    {
        qint64 stamp = 0; // date de modification et taille du fichier lu : une autre valeur, relecture
        QString dataUrl;  // vide : pas de vignette
    };

    void reload();
    void prune();
    void forgetDropped(const QStringList& dropped);
    DocumentPortal& activePortal() const;
    void persist();
    void requestThumbnails();
    void pumpThumbnails();

    // Date et taille du fichier local `url`, 0 si absent.
    static qint64 stampOf(const QUrl& url);

    QPointer<Preferences> m_preferences;
    QPointer<DocumentPortal> m_portal;
    DocumentPortal* m_platformPortal = nullptr; // enfant, quand `portal` est nul
    QStringList m_urls;                         // les plus récentes d'abord, au plus kMax
    bool m_contactActive = false;
    bool m_thumbnailInFlight = false;
    QString m_thumbnailLoading; // URL en cours de lecture
    int m_thumbnailsRead = 0;
    bool m_lastReadOffMainThread = false;
    QHash<QString, Thumbnail> m_thumbnails;
    std::deque<QString> m_thumbnailQueue; // URL à lire, une à la fois, hors contact
    QThreadPool m_pool;
};

} // namespace actcraft::ui
