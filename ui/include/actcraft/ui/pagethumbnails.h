#pragma once

#include <actcraft/ui/documentcontroller.h>
#include <actcraft/ui/pageview.h>

#include <QAbstractListModel>
#include <QImage>
#include <QPointer>
#include <QQuickPaintedItem>
#include <QtQml/qqmlregistration.h>

#include <vector>

namespace actcraft::ui {

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

signals:
    void wiringChanged();
    void thumbnailWidthChanged();
    void renderingChanged();

private:
    QPointer<DocumentController> m_controller;
    QPointer<PageView> m_view;
    std::vector<QImage> m_images;
    std::vector<int> m_revisions;
    int m_width = kDefaultWidth;
    int m_pending = 0;
    int m_renders = 0;
    double m_lastCopyMs = 0;
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
