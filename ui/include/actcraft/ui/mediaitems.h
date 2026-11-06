#pragma once
#include <actcraft/ui/documentcontroller.h>
#include <actcraft/ui/linklauncher.h>

#include <QAbstractListModel>
#include <QPointer>
#include <QQmlEngine>
#include <QString>
#include <QTimer>

#include <cstdint>
#include <functional>
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
    // Faux si l'objet n'est pas un média de la page affichée (un média masqué n'en fait pas partie). Vrai
    // veut dire « demandé » : la ressource est écrite hors du fil principal (BRIEF § 4, règle 1), puis la
    // lecture démarre (soundStarted, videoPlayRequested) au tour de boucle qui suit ; un second appel pendant
    // l'écriture ne lance rien de plus, et un document remplacé entre-temps jette le résultat. Fichier déjà
    // extrait : la lecture démarre dans l'appel.
    Q_INVOKABLE bool play(quint64 id);
    Q_INVOKABLE void stopSound();
    // Appelé par la surcouche quand la vidéo `id` ne se lit pas : repli annoncé vers le système
    // (confirmation).
    Q_INVOKABLE void videoFailed(quint64 id);
    // F-552 : son d'une action d'objet (ressource du document, ou fichier) joué par le lecteur intégré, piste
    // 1 ; le système n'est que le repli du lecteur (media::Player::setFallback, confirmation de
    // LinkLauncher).
    bool playResource(const std::string& key); // même règle d'écriture que play
    // Ressource du document qui n'est ni une vidéo ni un objet Flash : un son, jouable à part (objet masqué).
    bool isSoundResource(const std::string& key) const;
    bool playFile(const QString& path);

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
    void soundFailed(const QString& reason); // lecture de la piste 1 impossible (fichier, droits, réseau)
    void videoPlayRequested(quint64 id);

private:
    struct Entry
    {
        quint64 id = 0;
        QString kind;
        std::string resource; // clé de la ressource du document
        // Fichier extrait. Un GIF est extrait à l'affichage (il se montre aussitôt) ; une vidéo ou un son
        // seulement à la demande (play, lecture auto), jamais pour le seul affichage de la page.
        QString source;
        QRectF bounds;
        bool controlsHidden = false;
        bool loop = false;
        bool aspectLocked = false;
        bool clearBackground = false;
        bool wrapText = false;
        bool playOnOpen = false;
        bool autoPlay = false;
        // Fichier attendu dès l'affichage (GIF, vidéo en lecture auto) : écrit hors du fil principal, puis
        // `source` (et `autoPlay` pour la vidéo) arrivent par dataChanged.
        bool needsFile = false;
        bool operator==(const Entry&) const = default;
    };
    int rowCountProperty() const { return static_cast<int>(m_entries.size()); }
    void scheduleRefresh(bool pageShown);
    void refresh();
    std::vector<Entry> collect(bool pageShown);
    void applyRows(std::vector<Entry> next); // mise à jour par identifiant (insertion, retrait, rang)
    // Écriture synchrone d'une ressource (son de confirmation seulement : court, et sans attente possible).
    QString fileFor(const std::string& key);           // vide : absente, ou écriture en cours
    using Ready = std::function<void(const QString&)>; // chemin du fichier ; vide : écriture impossible
    // Faux : ressource absente ou dossier inutilisable. Vrai : `done` est appelée (peut être vide), tout de
    // suite si le fichier est déjà extrait, sinon une fois l'écriture finie, par la boucle d'événements.
    // `requester` (≠ 0 ou playResource) empêche d'inscrire deux fois la même demande pendant l'écriture.
    bool extract(const std::string& key, quint64 requester, Ready done);
    QString knownFile(const std::string& key) const;
    void onWritten(const std::string& key, const QString& path, bool ok, std::uint64_t epoch);
    void fillEntries(const std::string& key, const QString& path);
    void fetchFiles(); // lance l'écriture des fichiers attendus par les éléments (needsFile)
    Entry* entryById(quint64 id);
    void retireDirectory();
    QString defaultChime();
    bool startSound(const QString& path, bool loop);
    void updateSound();
    // Document remplacé (ou autre contrôleur) : lectures arrêtées, fichiers extraits supprimés.
    void resetForDocument();
    // Repli du lecteur et d'une vidéo illisible. Un fichier extrait du document passe par requestStored
    // (marque « venu d'ailleurs », H8-9) ; un fichier du disque, par request.
    void openWithSystem(const QString& path);

    QPointer<DocumentController> m_document;
    QPointer<LinkLauncher> m_links;
    QPointer<ToolHost> m_tools;
    std::unique_ptr<media::Player> m_player;
    std::shared_ptr<QTemporaryDir> m_dir; // partagé avec les écritures en cours : il vit jusqu'à leur fin
    struct Waiter
    {
        quint64 requester = 0;
        Ready done;
    };
    std::map<std::string, std::vector<Waiter>> m_writing; // écritures en cours, par clé de ressource
    std::uint64_t m_epoch = 0; // change à chaque document remplacé : un résultat d'écriture périmé est jeté
    std::map<std::string, QString> m_files; // clé de ressource -> fichier extrait
    std::vector<Entry> m_entries;
    QTimer m_refresh;
    bool m_contactActive = false;
    bool m_pending = false;
    bool m_pendingPageShown = false;
    int m_shownPage = -1;
    std::uint64_t m_soundRequest = 0; // jeton de la dernière demande de son (piste 1)
    std::uint64_t m_generation =
        0; // DocumentController::generation() du document dont les fichiers sont extraits
    QString m_soundName;
    qint64 m_soundPosition = 0;
    bool m_soundPlaying = false;
    QString m_chime;
};

} // namespace actcraft::ui
