#pragma once

#include <actcraft/ui/commands.h>
#include <actcraft/ui/documentcontroller.h>
#include <actcraft/ui/insertion.h>
#include <actcraft/ui/profilecontroller.h>
#include <actcraft/ui/toolhost.h>

#include <QAbstractListModel>
#include <QHash>
#include <QPointF>
#include <QString>
#include <QThreadPool>
#include <QUrl>
#include <QVariantList>
#include <QtQml/qqmlregistration.h>

#include <atomic>
#include <cstdint>
#include <deque>
#include <functional>
#include <memory>
#include <vector>

namespace actcraft::ui {

// Bibliothèque de ressources (F-810 à F-813, F-772, D6-14) : trois racines (mes ressources, ressources
// partagées, autre dossier), arbre des dossiers de la racine, éléments du dossier courant (fragments
// .actcraft, images .png .jpg .svg), recherche, vignettes, renommer, supprimer, créer un dossier, ajouter la
// sélection, insérer dans la page courante. Rien n'est lu avant setRoot. Listage, recherche et vignettes
// (cache compris) tournent hors du fil principal : jamais d'indexation bloquante, et aucun rendu de vignette
// n'est lancé pendant un contact (les demandes partent au relâché, comme PageThumbnails).
class ResourceLibrary : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(
        actcraft::ui::DocumentController* document READ document WRITE setDocument NOTIFY documentChanged)
    Q_PROPERTY(actcraft::ui::ProfileController* profile READ profile WRITE setProfile NOTIFY profileChanged)
    Q_PROPERTY(actcraft::ui::Commands* commands READ commands WRITE setCommands NOTIFY commandsChanged)
    Q_PROPERTY(actcraft::ui::ToolHost* tools READ tools WRITE setTools NOTIFY toolsChanged) // sélection
    Q_PROPERTY(bool contactActive READ contactActive WRITE setContactActive NOTIFY contactActiveChanged)
    Q_PROPERTY(QString root READ root NOTIFY rootChanged)               // « mine », « shared » ou « other »
    Q_PROPERTY(QString folder READ folder NOTIFY folderChanged)         // chemin absolu du dossier courant
    Q_PROPERTY(QVariantList folders READ folders NOTIFY foldersChanged) // [{path, name, depth}] de la racine
    Q_PROPERTY(int itemCount READ itemCount NOTIFY itemCountChanged)
    Q_PROPERTY(bool searching READ searching NOTIFY searchingChanged)
    Q_PROPERTY(bool grid READ grid WRITE setGrid NOTIFY viewChanged)    // réglage du profil : grille ou liste
    Q_PROPERTY(bool large READ large WRITE setLarge NOTIFY viewChanged) // réglage du profil : grande taille
    Q_PROPERTY(int thumbnailWidth READ thumbnailWidth NOTIFY viewChanged) // 96 ou 160 pixels
    Q_PROPERTY(bool usesTrash READ usesTrash CONSTANT) // faux : remove() supprime, la commande QML confirme
    Q_PROPERTY(int thumbnailsRendered READ thumbnailsRendered NOTIFY thumbnailsRenderedChanged) // tests

public:
    enum Role
    {
        NameRole = Qt::UserRole + 1,
        PathRole,
        KindRole,     // « fragment » ou « image »
        ThumbnailRole // URL file: de la vignette en cache, vide tant qu'elle n'est pas rendue
    };
    Q_ENUM(Role)

    static constexpr int kSmallThumbnail = 96;
    static constexpr int kLargeThumbnail = 160;

    explicit ResourceLibrary(QObject* parent = nullptr);
    ~ResourceLibrary() override;

    int rowCount(const QModelIndex& parent = {}) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    DocumentController* document() const noexcept;
    void setDocument(DocumentController* document);
    ProfileController* profile() const noexcept;
    void setProfile(ProfileController* profile);
    Commands* commands() const noexcept;
    void setCommands(Commands* commands);
    ToolHost* tools() const noexcept;
    void setTools(ToolHost* tools);
    bool contactActive() const noexcept;
    void setContactActive(bool active);

    QString root() const;
    QString folder() const;
    QVariantList folders() const;
    int itemCount() const noexcept;
    bool searching() const noexcept;
    bool grid() const;
    void setGrid(bool grid);
    bool large() const;
    void setLarge(bool large);
    int thumbnailWidth() const;
    static bool usesTrash();
    int thumbnailsRendered() const noexcept;

    Q_INVOKABLE void setRoot(const QString& root);
    Q_INVOKABLE void setOtherFolder(const QUrl& folder); // passe aussi à la racine « other »
    Q_INVOKABLE void openFolder(const QString& path);
    // Dans toute la racine : nom de fichier et (ou) mots-clés des fragments, sans casse ; annulable.
    Q_INVOKABLE void search(const QString& text, bool byName, bool byKeywords);
    // Rend la main sans attendre ; le dossier courant est relu, recherche finie ou non. Tout dossier relu
    // (ouverture, ajout, renommage, suppression) termine aussi une recherche en cours.
    Q_INVOKABLE void cancelSearch();
    // Les opérations suivantes rendent faux (ou vide) sans `commands`. insert et addSelection aussi sous
    // verrou ; toutes aussi pendant un contact (aucun accès disque sur le fil principal pendant un contact).
    // Dossier courant (ou un de ses parents) renommé ou supprimé : le dossier courant passe au parent de
    // l'élément touché. La racine elle-même ne se renomme ni ne se supprime.
    Q_INVOKABLE bool rename(const QString& path,
                            const QString& newName); // extension gardée ; nom pris : faux
    // Corbeille si usesTrash et pas `permanently`, sinon suppression définitive. Corbeille en échec : faux et
    // trashFailed(path) ; le QML peut alors proposer la suppression définitive.
    Q_INVOKABLE bool remove(const QString& path, bool permanently = false);
    Q_INVOKABLE bool createFolder(const QString& name);
    Q_INVOKABLE bool hasItem(const QString& path) const; // dans la liste affichée (mémoire, sans disque)
    Q_INVOKABLE QString addSelection(); // Ressource<N>.actcraft dans le dossier courant, N premier libre
    Q_INVOKABLE bool insert(const QString& path, QPointF pagePoint); // dans la page courante, une étape

    static QString mineRoot();   // Documents/ActCraft/Ressources
    static QString sharedRoot(); // <AppDataLocation>/ressources-partagees (réglable par le profil)

    // Essais : racines, corbeille (`fails` : moveToTrash simulé en échec), observateurs du rendu (fil de la
    // tâche de vignette, à chaque rendu) et de la recherche (fil de la recherche, à chaque fichier visité).
    static void setRootPathsForTests(const QString& mine, const QString& shared);
    static void setTrashForTests(bool useTrash, bool fails = false);
    static void setRenderObserverForTests(std::function<void()> observer);
    static void setSearchObserverForTests(std::function<void()> observer);

signals:
    void documentChanged();
    void profileChanged();
    void commandsChanged();
    void toolsChanged();
    void contactActiveChanged();
    void rootChanged();
    void folderChanged();
    void foldersChanged();
    void itemCountChanged();
    void searchingChanged();
    void viewChanged();
    void thumbnailsRenderedChanged();
    void trashFailed(const QString& path); // remove : corbeille en échec, rien n'est supprimé

public:
    struct Item
    {
        QString name;
        QString path;
        QString kind;
        QUrl thumbnail;
    };

private:
    bool mayEdit() const; // commandes présentes, document présent, pas de verrou
    QString rootPath(const QString& root) const;
    void refresh();
    void setItems(std::vector<Item> items);
    void setSearching(bool searching);
    void endSearch();
    void requestThumbnails();
    void dropThumbnails();
    void pumpThumbnails();
    void leaveFolder(const QString& path);

    DocumentController* m_document = nullptr;
    ProfileController* m_profile = nullptr;
    Commands* m_commands = nullptr;
    ToolHost* m_tools = nullptr;
    bool m_contactActive = false;
    QString m_root;
    QString m_rootPath;
    QString m_folder;
    QString m_other;
    QVariantList m_folders;
    std::vector<Item> m_items;
    bool m_searching = false;
    int m_thumbnailsRendered = 0;
    int m_thumbnailWidth = kSmallThumbnail;
    std::uint64_t m_itemsGeneration = 0; // un listage ou une recherche périmés ne s'appliquent pas
    std::uint64_t m_treeGeneration = 0;
    std::shared_ptr<std::atomic<bool>> m_searchCancelled;
    std::uint64_t m_thumbnailGeneration = 0; // une vignette d'éléments remplacés ne s'applique pas
    std::deque<Item> m_thumbnailQueue;       // demandes en attente : une à la fois, hors contact
    bool m_thumbnailInFlight = false;
    Insertion m_insertion; // image : le chemin du dépôt sur la page (lot 5, A11)
    QThreadPool m_pool;    // vignettes : à part, pour ne retarder ni la recherche ni l'insertion
};

} // namespace actcraft::ui
