#pragma once

#include <actcraft/ui/documentcontroller.h>
#include <actcraft/ui/pageview.h>

#include <QAbstractListModel>
#include <QImage>
#include <QPointer>
#include <QQuickPaintedItem>
#include <QTimer>
#include <QtQml/qqmlregistration.h>

#include <cstdint>
#include <memory>
#include <vector>

class QThread;

namespace actcraft::ui {

struct ThumbnailRelay; // pagethumbnails.cpp

// Miniatures des pages (F-800), à jour après chaque modification sans figer l'écriture : copie de la page sur
// le fil principal, rendu par le peintre partagé (DA5) sur un fil secondaire, kSettleMs après la dernière
// modification de la page, jamais pendant un contact sur la vue.
class PageThumbnails : public QAbstractListModel
{
    Q_OBJECT
    QML_NAMED_ELEMENT(PageThumbnails)
    Q_PROPERTY(actcraft::ui::DocumentController* documentController READ documentController WRITE
                   setDocumentController NOTIFY wiringChanged)
    Q_PROPERTY(actcraft::ui::PageView* view READ view WRITE setView NOTIFY wiringChanged)
    Q_PROPERTY(int thumbnailWidth READ thumbnailWidth WRITE setThumbnailWidth NOTIFY thumbnailWidthChanged)
    Q_PROPERTY(int pendingCount READ pendingCount NOTIFY renderingChanged)
    Q_PROPERTY(int renderCount READ renderCount NOTIFY renderingChanged)
    Q_PROPERTY(double lastCopyMs READ lastCopyMs NOTIFY renderingChanged) // fil principal, dernier rendu
    // Amendement 3 : rendus lancés (compté sur le fil principal au lancement) ; renderCount compte les
    // terminés.
    Q_PROPERTY(int launchCount READ launchCount NOTIFY renderingChanged)
    // Amendement 5 du lot 4 (L4-T19) : faux quand le panneau des navigateurs est caché ; aucun rendu n'est
    // alors lancé. Revenu à vrai : les pages modifiées entre-temps, ou jamais rendues, le sont kSettleMs plus
    // tard, comme après une modification (et jamais pendant un contact). Vrai par défaut.
    Q_PROPERTY(bool active READ active WRITE setActive NOTIFY activeChanged)

public:
    static constexpr int kSettleMs = 400;
    static constexpr int kDefaultWidth = 160;
    enum Role
    {
        PageNumberRole = Qt::UserRole + 1, // 1 à N
        RevisionRole,                      // change à chaque nouvelle image
        AspectRole,                        // hauteur / largeur de la page
        CurrentRole,                       // page affichée
    };

    explicit PageThumbnails(QObject* parent = nullptr);
    ~PageThumbnails() override;
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    DocumentController* documentController() const noexcept;
    void setDocumentController(DocumentController* controller);
    PageView* view() const noexcept;
    void setView(PageView* view);
    int thumbnailWidth() const noexcept;
    void setThumbnailWidth(int width); // 80 à 320
    int pendingCount() const noexcept;
    int renderCount() const noexcept;
    double lastCopyMs() const noexcept;
    int launchCount() const noexcept; // amendement 3
    QImage image(int page) const;     // dernière image rendue ; nulle avant le premier rendu
    Q_INVOKABLE void refreshAll();
    QThread* lastRenderThread() const noexcept; // tests : fil du dernier rendu terminé
    bool active() const noexcept;               // amendement 5 (L4-T19)
    void setActive(bool active);                // n'interrompt pas un rendu déjà lancé

signals:
    void wiringChanged();
    void thumbnailWidthChanged();
    void renderingChanged();
    void activeChanged(); // amendement 5 (L4-T19)

private:
    void attachController();
    void detachController();
    // Lignes refaites, génération avancée (tout rendu en vol sera jeté). `changes` : pages insérées ou
    // retirées, images et marques suivent leur page, seules les pages insérées et celle du rendu en vol sont
    // marquées (L4-C10) ; nullptr : document remplacé, images oubliées, toutes marquées.
    void resetRows(const core::Changes* changes);
    void onChanges(const core::Changes& changes);
    void onDocumentChanged();
    void onPagesChanged();
    void onContactChanged();
    void mark(int page);
    void markAll();
    bool contactInProgress() const;
    // Lance le rendu d'une page marquée si rien n'est en vol, le minuteur échu et aucun contact en cours.
    void pump();
    void launch(int page);
    void finish(std::uint64_t generation, int page, const QImage& image, QThread* thread);
    void updatePending();

    QPointer<DocumentController> m_controller;
    QPointer<PageView> m_view;
    std::vector<QImage> m_images;
    std::vector<int> m_revisions;
    int m_width = kDefaultWidth;
    int m_pending = 0;
    int m_renders = 0;
    double m_lastCopyMs = 0;
    std::vector<char> m_marked; // une case par ligne : page à rendre
    QTimer m_settle;            // kSettleMs après la dernière marque, minuteur unique
    std::shared_ptr<ThumbnailRelay> m_relay;
    std::uint64_t m_generation = 0;
    bool m_inFlight = false; // un rendu au plus à la fois
    int m_launches = 0;
    int m_revisionCounter = 0;
    int m_current = -1;
    int m_observerToken = 0;
    bool m_sawChanges = false; // l'observateur a parlé depuis le dernier documentChanged
    QThread* m_lastRenderThread = nullptr;
    QList<QMetaObject::Connection> m_controllerConnections;
    QMetaObject::Connection m_viewConnection;
    int m_inFlightPage = -1; // L4-C10 : ligne du rendu en vol, remarquée si les lignes sont refaites
    bool m_active = true;    // amendement 5 (L4-T19)

public:
    // Tests (L4-C10) : délai de calme réglable, kSettleMs par défaut.
    void setSettleDelay(int ms);
    int settleDelay() const noexcept;
};

// Image d'une miniature dans le QML (type ThumbnailImage) : peint PageThumbnails::image(page).
class ThumbnailItem : public QQuickPaintedItem
{
    Q_OBJECT
    QML_NAMED_ELEMENT(ThumbnailImage)
    Q_PROPERTY(actcraft::ui::PageThumbnails* source READ source WRITE setSource NOTIFY sourceChanged)
    Q_PROPERTY(int page READ page WRITE setPage NOTIFY pageChanged) // indice 0 à N − 1
    Q_PROPERTY(int revision READ revision WRITE setRevision NOTIFY revisionChanged)

public:
    explicit ThumbnailItem(QQuickItem* parent = nullptr);
    PageThumbnails* source() const noexcept;
    void setSource(PageThumbnails* source);
    int page() const noexcept;
    void setPage(int page);
    int revision() const noexcept;
    void setRevision(int revision);
    void paint(QPainter* painter) override;

signals:
    void sourceChanged();
    void pageChanged();
    void revisionChanged();

private:
    QPointer<PageThumbnails> m_source;
    int m_page = -1;
    int m_revision = 0;
};

} // namespace actcraft::ui
