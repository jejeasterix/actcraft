#pragma once
#include <actcraft/ui/documentcontroller.h>
#include <actcraft/ui/linklauncher.h>

#include <QAbstractListModel>
#include <QPointer>
#include <QQmlEngine>
#include <QString>
#include <QTimer>

#include <map>
#include <memory>
#include <string>
#include <vector>

class QTemporaryDir;

namespace actcraft::media {
class Player;
}

namespace actcraft::ui {

class ToolHost;

// Médias de la page affichée (S25, F-552, F-582) : modèle des surcouches de MediaLayer.qml, qui posent la
// vidéo et le GIF animé au-dessus du cache (pageOverlays), jamais dedans : la page fixe ne peint que l'espace
// réservé et rien ici ne lui demande de redessin. La relecture du document est regroupée (un tour de boucle)
// et reportée pendant un contact ; un trait ou une édition qui ne change aucun média ne touche pas le modèle,
// donc la vidéo qui joue continue. Le lecteur intégré (media::Player) appartient à ce modèle : piste 1 pour
// les sons, piste 2 pour le son de confirmation des conteneurs. Un format illisible part au système par
// `links` (LinkLauncher::SystemMedia, avec confirmation). « Lecture auto » (playOnOpen) ne vaut qu'ici, à
// l'affichage de la page.
class MediaItems : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(DocumentController* document READ document WRITE setDocument NOTIFY documentChanged)
    Q_PROPERTY(bool contactActive READ contactActive WRITE setContactActive NOTIFY contactActiveChanged)
    Q_PROPERTY(LinkLauncher* links READ links WRITE setLinks NOTIFY linksChanged)
    Q_PROPERTY(ToolHost* tools READ tools WRITE setTools NOTIFY toolsChanged)
    Q_PROPERTY(int count READ rowCountProperty NOTIFY itemsChanged)
    Q_PROPERTY(bool soundPlaying READ soundPlaying NOTIFY soundStateChanged)
    Q_PROPERTY(qint64 soundPosition READ soundPosition NOTIFY soundStateChanged)
    Q_PROPERTY(QString soundName READ soundName NOTIFY soundStateChanged)
public:
    enum Role
    {
        IdRole = Qt::UserRole + 1,
        KindRole, // "video", "sound", "gif"
        SourceRole,
        BoundsRole, // QRectF, coordonnées de page
        ControlsHiddenRole,
        LoopRole,
        AspectLockedRole,
        ClearBackgroundRole,
        WrapTextRole,
        AutoPlayRole // playOnOpen d'une vidéo, vrai seulement à l'affichage de la page
    };

    explicit MediaItems(QObject* parent = nullptr);
    ~MediaItems() override;

    DocumentController* document() const;
    void setDocument(DocumentController* document);
    bool contactActive() const;
    void setContactActive(bool active);
    LinkLauncher* links() const;
    void setLinks(LinkLauncher* links);
    ToolHost* tools() const;
    void setTools(ToolHost* tools);

    bool soundPlaying() const;
    qint64 soundPosition() const;
    QString soundName() const;
    media::Player* player() const; // tests

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    // Joue un média de la page affichée dans le lecteur intégré (son : piste 1 ; vidéo : sa surcouche).
    // Faux si l'objet n'est pas un média de la page affichée.
    Q_INVOKABLE bool play(quint64 id);
    Q_INVOKABLE void stopSound();

public slots:
    // ToolHost::containerAccepted : `chime` joue le son de confirmation du conteneur sur la piste 2.
    void onContainerAccepted(quint64 container, bool chime);

signals:
    void documentChanged();
    void contactActiveChanged();
    void linksChanged();
    void toolsChanged();
    void itemsChanged();
    void soundStateChanged();
    void soundStarted();
    void videoPlayRequested(quint64 id);

private:
    struct Entry
    {
        quint64 id = 0;
        QString kind;
        QString source;
        QRectF bounds;
        bool controlsHidden = false;
        bool loop = false;
        bool aspectLocked = false;
        bool clearBackground = false;
        bool wrapText = false;
        bool playOnOpen = false;
        bool autoPlay = false;
        bool operator==(const Entry&) const = default;
    };
    int rowCountProperty() const { return static_cast<int>(m_entries.size()); }
    void scheduleRefresh(bool pageShown);
    void refresh();
    std::vector<Entry> collect(bool pageShown);
    QString fileFor(const std::string& key); // ressource du document extraite (vide : absente)
    QString defaultChime();
    void updateSound();

    QPointer<DocumentController> m_document;
    QPointer<LinkLauncher> m_links;
    QPointer<ToolHost> m_tools;
    std::unique_ptr<media::Player> m_player;
    std::unique_ptr<QTemporaryDir> m_dir;
    std::map<std::string, QString> m_files; // clé de ressource -> fichier extrait
    std::vector<Entry> m_entries;
    QTimer m_refresh;
    bool m_contactActive = false;
    bool m_pending = false;
    bool m_pendingPageShown = false;
    int m_shownPage = -1;
    QString m_soundName;
    qint64 m_soundPosition = 0;
    bool m_soundPlaying = false;
    QString m_chime;
};

} // namespace actcraft::ui
