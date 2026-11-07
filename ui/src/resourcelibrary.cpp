#include <actcraft/core/broadcast.h>
#include <actcraft/core/clipboardedits.h>
#include <actcraft/core/edits.h>
#include <actcraft/formats/fragment.h>
#include <actcraft/render/pagepainter.h>
#include <actcraft/ui/resourcelibrary.h>

#include <QCryptographicHash>
#include <QDateTime>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QFutureWatcher>
#include <QImage>
#include <QImageReader>
#include <QMutex>
#include <QMutexLocker>
#include <QSaveFile>
#include <QStandardPaths>
#include <QVariantMap>
#include <QtConcurrent>

#include <algorithm>
#include <optional>
#include <utility>

namespace actcraft::ui {

namespace {

using Item = ResourceLibrary::Item;

constexpr auto kSettingsSection = "ressources";
constexpr auto kFragmentKind = "fragment";
constexpr auto kImageKind = "image";
constexpr int kMaxResourceNumber = 100000;

struct TestHooks
{
    QString mine;
    QString shared;
    bool useTrash = true;
    bool trashFails = false;
    QMutex observerMutex;
    std::function<void()> renderObserver;
    std::function<void()> searchObserver;
};

TestHooks& hooks()
{
    static TestHooks instance;
    return instance;
}

// Copie l'observateur sous le verrou et l'appelle hors du verrou : un observateur qui attend ne retient ni
// les autres fils ni la pose d'un autre observateur.
void notify(std::function<void()> TestHooks::* which)
{
    std::function<void()> observer;
    {
        QMutexLocker lock(&hooks().observerMutex);
        observer = hooks().*which;
    }
    if (observer)
        observer();
}

bool isImageSuffix(const QString& suffix)
{
    return suffix == QLatin1String("png") || suffix == QLatin1String("jpg") ||
           suffix == QLatin1String("jpeg") || suffix == QLatin1String("svg");
}

// Genre d'un fichier de la bibliothèque ; vide s'il n'en est pas.
QString kindOf(const QFileInfo& info)
{
    const QString suffix = info.suffix().toLower();
    if (suffix == QLatin1String("actcraft"))
        return QString::fromLatin1(kFragmentKind);
    if (isImageSuffix(suffix))
        return QString::fromLatin1(kImageKind);
    return {};
}

Item itemOf(const QFileInfo& info, const QString& kind)
{
    return Item{info.completeBaseName(), info.absoluteFilePath(), kind, {}};
}

bool nameOrder(const Item& a, const Item& b)
{
    const int order = QString::compare(a.name, b.name, Qt::CaseInsensitive);
    return order != 0 ? order < 0 : a.path < b.path;
}

bool validName(const QString& name)
{
    return !name.isEmpty() && name != QLatin1String(".") && name != QLatin1String("..") &&
           !name.contains(QLatin1Char('/')) && !name.contains(QLatin1Char('\\'));
}

std::optional<formats::Fragment> readFragmentFile(const QString& path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly))
        return std::nullopt;
    QString error;
    return formats::readFragment(file, error);
}

// --- Fil secondaire -------------------------------------------------------------------------------------

struct ListingJob
{
    std::uint64_t itemsGeneration = 0;
    std::uint64_t treeGeneration = 0;
    QVariantList folders;
    std::vector<Item> items;
};

void listFolders(const QString& dir, int depth, QVariantList& out)
{
    const QFileInfoList children = QDir(dir).entryInfoList(
        QDir::Dirs | QDir::NoDotAndDotDot | QDir::NoSymLinks, QDir::Name | QDir::IgnoreCase);
    for (const QFileInfo& child : children) {
        out.push_back(QVariantMap{{QStringLiteral("path"), child.absoluteFilePath()},
                                  {QStringLiteral("name"), child.fileName()},
                                  {QStringLiteral("depth"), depth}});
        listFolders(child.absoluteFilePath(), depth + 1, out);
    }
}

ListingJob listRoot(const QString& root, bool createRoot, const QString& folder)
{
    ListingJob listing;
    if (createRoot && !root.isEmpty())
        QDir().mkpath(root);
    if (!root.isEmpty())
        listFolders(root, 0, listing.folders);
    for (const QFileInfo& info : QDir(folder).entryInfoList(QDir::Files, QDir::Name | QDir::IgnoreCase)) {
        const QString kind = kindOf(info);
        if (!kind.isEmpty())
            listing.items.push_back(itemOf(info, kind));
    }
    std::sort(listing.items.begin(), listing.items.end(), nameOrder);
    return listing;
}

bool keywordsMatch(const QString& path, const QString& text)
{
    const std::optional<formats::Fragment> fragment = readFragmentFile(path);
    if (!fragment)
        return false;
    return std::any_of(fragment->page.keywords.begin(), fragment->page.keywords.end(),
                       [&text](const std::string& keyword) {
                           return QString::fromStdString(keyword).contains(text, Qt::CaseInsensitive);
                       });
}

// Parcourt toute la racine ; s'arrête dès que `cancelled` passe à vrai (le résultat est alors jeté).
std::vector<Item> searchRoot(const QString& root, const QString& text, bool names, bool keywords,
                             const std::shared_ptr<std::atomic<bool>>& cancelled)
{
    std::vector<Item> found;
    QDirIterator it(root, QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        if (cancelled->load())
            return {};
        const QFileInfo info(it.next());
        notify(&TestHooks::searchObserver);
        const QString kind = kindOf(info);
        if (kind.isEmpty())
            continue;
        const bool hit = (names && info.completeBaseName().contains(text, Qt::CaseInsensitive)) ||
                         (keywords && kind == QLatin1String(kFragmentKind) &&
                          keywordsMatch(info.absoluteFilePath(), text));
        if (hit)
            found.push_back(itemOf(info, kind));
    }
    std::sort(found.begin(), found.end(), nameOrder);
    return found;
}

// Fil secondaire : première page du fragment à `width` pixels de large (amendement 4, A13).
QImage fragmentThumbnail(const formats::Fragment& fragment, int width)
{
    const core::Page& page = fragment.page;
    if (width <= 0 || !(page.width > 0))
        return {};
    // `images` vit jusqu'à la fin de renderPage (DecodedImages::lookup).
    const render::DecodedImages images = render::decodeImages(fragment.resources, width);
    render::PaintOptions options;
    options.thumbnail = true;
    return render::renderPage(page, images.lookup(), static_cast<double>(width) / page.width, options);
}

// Fil secondaire : image réduite au décodage, jamais agrandie.
QImage imageThumbnail(const QString& path, int width)
{
    QImageReader reader(path);
    reader.setAutoTransform(true);
    const QSize size = reader.size();
    if (size.isValid() && size.width() > width)
        reader.setScaledSize(
            QSize(width, std::max(1, qRound(size.height() * static_cast<double>(width) / size.width()))));
    return reader.read();
}

struct ThumbnailJob
{
    std::uint64_t generation = 0;
    QString path;
    QString file;          // vignette en cache ; vide : rien à montrer
    bool rendered = false; // faux : trouvée dans le cache
};

// Fil secondaire : cache <CacheLocation>/vignettes/<empreinte>.png (chemin, taille, date, largeur), sinon
// rendu.
ThumbnailJob makeThumbnail(std::uint64_t generation, const Item& item, int width)
{
    ThumbnailJob out;
    out.generation = generation;
    out.path = item.path;
    const QFileInfo info(item.path);
    if (!info.isFile())
        return out;
    QCryptographicHash hash(QCryptographicHash::Sha256);
    hash.addData(item.path.toUtf8());
    hash.addData(QByteArray::number(info.size()));
    hash.addData(QByteArray::number(info.lastModified().toMSecsSinceEpoch()));
    hash.addData(QByteArray::number(width));
    const QString dir =
        QStandardPaths::writableLocation(QStandardPaths::CacheLocation) + QStringLiteral("/vignettes");
    const QString file =
        dir + QLatin1Char('/') + QString::fromLatin1(hash.result().toHex()) + QStringLiteral(".png");
    if (QFileInfo::exists(file)) {
        out.file = file;
        return out;
    }
    QImage image;
    if (item.kind == QLatin1String(kFragmentKind)) {
        if (const std::optional<formats::Fragment> fragment = readFragmentFile(item.path))
            image = fragmentThumbnail(*fragment, width);
    } else {
        image = imageThumbnail(item.path, width);
    }
    notify(&TestHooks::renderObserver);
    if (image.isNull() || !QDir().mkpath(dir))
        return out;
    QSaveFile save(file);
    if (!save.open(QIODevice::WriteOnly) || !image.save(&save, "PNG") || !save.commit())
        return out;
    out.file = file;
    out.rendered = true;
    return out;
}

// --- Insertion d'un fragment
// --------------------------------------------------------------------------------

// Une étape. prepareInsertion rend de VRAIS identifiants, tirés dans le document (withFreshIds), alors que
// PasteObjectsCommand n'accepte que des rangs provisoires 1..n (clipboardedits.h) : passés tels quels, un
// membre de groupe ou une extrémité de connecteur hors de 1..n deviendrait 0 et le groupe se viderait.
// preparePaste repasse donc les objets en rangs, références remappées, et les translate. prepareInsertion
// tourne dans redo, sur le document de la commande : les ressources manquantes y sont copiées à chaque redo
// (même clé, même copie), comme le collage du lot 5 ; les objets ne sont préparés qu'au premier.
class InsertFragmentCommand final : public core::UndoCommand
{
public:
    InsertFragmentCommand(std::size_t page, formats::Fragment fragment, double dx, double dy)
        : m_page(page), m_fragment(std::move(fragment)), m_dx(dx), m_dy(dy)
    {}
    void redo(core::Document& document, core::Changes& changes) override
    {
        std::vector<core::PageObject> fresh = formats::prepareInsertion(m_fragment, document);
        if (!m_inner)
            m_inner = std::make_unique<core::PasteObjectsCommand>(
                m_page, core::preparePaste(std::move(fresh), m_dx, m_dy));
        m_inner->redo(document, changes);
    }
    void undo(core::Document& document, core::Changes& changes) override
    {
        if (m_inner)
            m_inner->undo(document, changes);
    }

private:
    std::size_t m_page;
    formats::Fragment m_fragment;
    double m_dx;
    double m_dy;
    std::unique_ptr<core::PasteObjectsCommand> m_inner;
};

} // namespace

ResourceLibrary::ResourceLibrary(QObject* parent) : QAbstractListModel(parent)
{
    m_pool.setMaxThreadCount(1); // une vignette en vol au plus (pumpThumbnails)
}

ResourceLibrary::~ResourceLibrary()
{
    if (m_searchCancelled)
        m_searchCancelled->store(true);
    m_pool.clear(); // vignettes en file abandonnées ; m_pool attend les seules en cours
}

int ResourceLibrary::rowCount(const QModelIndex& parent) const
{
    return parent.isValid() ? 0 : static_cast<int>(m_items.size());
}

QVariant ResourceLibrary::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= rowCount())
        return {};
    const Item& item = m_items[static_cast<std::size_t>(index.row())];
    switch (role) {
    case Qt::DisplayRole:
    case NameRole:
        return item.name;
    case PathRole:
        return item.path;
    case KindRole:
        return item.kind;
    case ThumbnailRole:
        return item.thumbnail;
    default:
        return {};
    }
}

QHash<int, QByteArray> ResourceLibrary::roleNames() const
{
    return {{NameRole, "name"}, {PathRole, "path"}, {KindRole, "kind"}, {ThumbnailRole, "thumbnail"}};
}

DocumentController* ResourceLibrary::document() const noexcept
{
    return m_document;
}

void ResourceLibrary::setDocument(DocumentController* document)
{
    if (m_document == document)
        return;
    m_document = document;
    m_insertion.setDocument(document);
    emit documentChanged();
}

ProfileController* ResourceLibrary::profile() const noexcept
{
    return m_profile;
}

void ResourceLibrary::setProfile(ProfileController* profile)
{
    if (m_profile == profile)
        return;
    if (m_profile != nullptr)
        disconnect(m_profile, nullptr, this, nullptr);
    m_profile = profile;
    if (m_profile != nullptr) {
        connect(m_profile, &ProfileController::profileChanged, this, [this] {
            emit viewChanged();
            if (thumbnailWidth() != m_thumbnailWidth)
                requestThumbnails();
        });
    }
    emit profileChanged();
    emit viewChanged();
    if (thumbnailWidth() != m_thumbnailWidth)
        requestThumbnails();
}

Commands* ResourceLibrary::commands() const noexcept
{
    return m_commands;
}

void ResourceLibrary::setCommands(Commands* commands)
{
    if (m_commands == commands)
        return;
    m_commands = commands;
    emit commandsChanged();
}

ToolHost* ResourceLibrary::tools() const noexcept
{
    return m_tools;
}

void ResourceLibrary::setTools(ToolHost* tools)
{
    if (m_tools == tools)
        return;
    m_tools = tools;
    emit toolsChanged();
}

bool ResourceLibrary::contactActive() const noexcept
{
    return m_contactActive;
}

void ResourceLibrary::setContactActive(bool active)
{
    if (m_contactActive == active)
        return;
    m_contactActive = active;
    emit contactActiveChanged();
    pumpThumbnails(); // relâché : les demandes gardées partent
}

QString ResourceLibrary::root() const
{
    return m_root;
}

QString ResourceLibrary::folder() const
{
    return m_folder;
}

QVariantList ResourceLibrary::folders() const
{
    return m_folders;
}

int ResourceLibrary::itemCount() const noexcept
{
    return static_cast<int>(m_items.size());
}

bool ResourceLibrary::searching() const noexcept
{
    return m_searching;
}

bool ResourceLibrary::grid() const
{
    return m_profile != nullptr &&
           m_profile->setting(QString::fromLatin1(kSettingsSection), QStringLiteral("grille"), false)
               .toBool();
}

void ResourceLibrary::setGrid(bool grid)
{
    if (m_profile != nullptr)
        m_profile->setSetting(QString::fromLatin1(kSettingsSection), QStringLiteral("grille"), grid);
}

bool ResourceLibrary::large() const
{
    return m_profile != nullptr &&
           m_profile->setting(QString::fromLatin1(kSettingsSection), QStringLiteral("grande"), false)
               .toBool();
}

void ResourceLibrary::setLarge(bool large)
{
    if (m_profile != nullptr)
        m_profile->setSetting(QString::fromLatin1(kSettingsSection), QStringLiteral("grande"), large);
}

int ResourceLibrary::thumbnailWidth() const
{
    return large() ? kLargeThumbnail : kSmallThumbnail;
}

bool ResourceLibrary::usesTrash()
{
#if defined(Q_OS_ANDROID) || defined(Q_OS_IOS)
    return false;
#else
    return hooks().useTrash;
#endif
}

int ResourceLibrary::thumbnailsRendered() const noexcept
{
    return m_thumbnailsRendered;
}

QString ResourceLibrary::mineRoot()
{
    if (!hooks().mine.isEmpty())
        return hooks().mine;
#if defined(Q_OS_ANDROID)
    return QDir(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation))
        .filePath(QStringLiteral("Ressources"));
#else
    return QDir(QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation))
        .filePath(QStringLiteral("ActCraft/Ressources"));
#endif
}

QString ResourceLibrary::sharedRoot()
{
    if (!hooks().shared.isEmpty())
        return hooks().shared;
    return QDir(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation))
        .filePath(QStringLiteral("ressources-partagees"));
}

void ResourceLibrary::setRootPathsForTests(const QString& mine, const QString& shared)
{
    hooks().mine = mine;
    hooks().shared = shared;
}

void ResourceLibrary::setTrashForTests(bool useTrash, bool fails)
{
    hooks().useTrash = useTrash;
    hooks().trashFails = fails;
}

void ResourceLibrary::setRenderObserverForTests(std::function<void()> observer)
{
    QMutexLocker lock(&hooks().observerMutex);
    hooks().renderObserver = std::move(observer);
}

void ResourceLibrary::setSearchObserverForTests(std::function<void()> observer)
{
    QMutexLocker lock(&hooks().observerMutex);
    hooks().searchObserver = std::move(observer);
}

QString ResourceLibrary::rootPath(const QString& root) const
{
    if (root == QLatin1String("mine"))
        return QDir::cleanPath(mineRoot());
    if (root == QLatin1String("shared")) {
        const QString chosen = m_profile != nullptr ? m_profile
                                                          ->setting(QString::fromLatin1(kSettingsSection),
                                                                    QStringLiteral("partagees"), QString())
                                                          .toString()
                                                    : QString();
        return QDir::cleanPath(chosen.isEmpty() ? sharedRoot() : chosen);
    }
    if (root == QLatin1String("other"))
        return m_other;
    return {};
}

void ResourceLibrary::setRoot(const QString& root)
{
    const QString path = rootPath(root);
    if (path.isEmpty())
        return;
    if (m_root != root) {
        m_root = root;
        emit rootChanged();
    }
    m_rootPath = path;
    openFolder(path);
}

void ResourceLibrary::setOtherFolder(const QUrl& folder)
{
    const QString path = folder.isLocalFile() ? folder.toLocalFile() : folder.toString();
    if (path.isEmpty())
        return;
    m_other = QDir::cleanPath(path);
    setRoot(QStringLiteral("other"));
}

void ResourceLibrary::openFolder(const QString& path)
{
    if (path.isEmpty())
        return;
    const QString cleaned = QDir::cleanPath(path);
    if (m_folder != cleaned) {
        m_folder = cleaned;
        emit folderChanged();
    }
    refresh();
}

// Relit l'arbre de la racine et le dossier courant sur un fil secondaire ; un résultat périmé est jeté. Une
// recherche en cours est terminée : le dossier relu la remplace (sinon son résultat, jeté par la génération,
// laisserait `searching` vrai).
void ResourceLibrary::refresh()
{
    endSearch();
    if (m_folder.isEmpty())
        return;
    dropThumbnails();
    const std::uint64_t items = ++m_itemsGeneration;
    const std::uint64_t tree = ++m_treeGeneration;
    const QString root = m_rootPath;
    const bool createRoot = m_root == QLatin1String("mine") || m_root == QLatin1String("shared");
    const QString folder = m_folder;
    auto* watcher = new QFutureWatcher<ListingJob>(this);
    connect(watcher, &QFutureWatcher<ListingJob>::finished, this, [this, watcher] {
        ListingJob listing = watcher->result();
        watcher->deleteLater();
        if (listing.treeGeneration == m_treeGeneration && listing.folders != m_folders) {
            m_folders = std::move(listing.folders);
            emit foldersChanged();
        }
        if (listing.itemsGeneration == m_itemsGeneration)
            setItems(std::move(listing.items));
    });
    watcher->setFuture(QtConcurrent::run([root, createRoot, folder, items, tree] {
        ListingJob listing = listRoot(root, createRoot, folder);
        listing.itemsGeneration = items;
        listing.treeGeneration = tree;
        return listing;
    }));
}

void ResourceLibrary::setItems(std::vector<Item> items)
{
    const int before = itemCount();
    beginResetModel();
    m_items = std::move(items);
    endResetModel();
    if (itemCount() != before)
        emit itemCountChanged();
    requestThumbnails();
}

void ResourceLibrary::endSearch()
{
    if (m_searchCancelled)
        m_searchCancelled->store(true);
    setSearching(false);
}

void ResourceLibrary::setSearching(bool searching)
{
    if (m_searching == searching)
        return;
    m_searching = searching;
    emit searchingChanged();
}

void ResourceLibrary::search(const QString& text, bool byName, bool byKeywords)
{
    if (m_searchCancelled)
        m_searchCancelled->store(true);
    const QString needle = text.trimmed();
    if (needle.isEmpty() || m_rootPath.isEmpty()) {
        refresh();
        return;
    }
    dropThumbnails();
    const std::uint64_t generation = ++m_itemsGeneration;
    auto cancelled = std::make_shared<std::atomic<bool>>(false);
    m_searchCancelled = cancelled;
    setSearching(true);
    auto* watcher = new QFutureWatcher<std::vector<Item>>(this);
    connect(watcher, &QFutureWatcher<std::vector<Item>>::finished, this,
            [this, watcher, generation, cancelled] {
                std::vector<Item> found = watcher->result();
                watcher->deleteLater();
                if (cancelled->load() || generation != m_itemsGeneration)
                    return; // annulée ou remplacée : jamais appliquée
                setSearching(false);
                setItems(std::move(found));
            });
    const QString root = m_rootPath;
    watcher->setFuture(QtConcurrent::run([root, needle, byName, byKeywords, cancelled] {
        return searchRoot(root, needle, byName, byKeywords, cancelled);
    }));
}

// Recherche finie ou non : le dossier courant est relu (refresh termine la recherche en cours).
void ResourceLibrary::cancelSearch()
{
    refresh();
}

void ResourceLibrary::requestThumbnails()
{
    m_thumbnailWidth = thumbnailWidth();
    ++m_thumbnailGeneration;
    m_thumbnailQueue.assign(m_items.begin(), m_items.end());
    pumpThumbnails();
}

// Les éléments vont être remplacés (dossier quitté ou relu, recherche) : la file est vidée et la vignette en
// vol sera jetée à son retour.
void ResourceLibrary::dropThumbnails()
{
    ++m_thumbnailGeneration;
    m_thumbnailQueue.clear();
}

// Patron de PageThumbnails::pump (L4-T17) : une vignette en vol au plus, la suivante part à son retour, et
// seulement hors contact ; pendant un contact la file attend le relâché (setContactActive).
void ResourceLibrary::pumpThumbnails()
{
    if (m_thumbnailInFlight || m_contactActive || m_thumbnailQueue.empty())
        return;
    const Item item = std::move(m_thumbnailQueue.front());
    m_thumbnailQueue.pop_front();
    const std::uint64_t generation = m_thumbnailGeneration;
    const int width = m_thumbnailWidth;
    m_thumbnailInFlight = true;
    auto* watcher = new QFutureWatcher<ThumbnailJob>(this);
    connect(watcher, &QFutureWatcher<ThumbnailJob>::finished, this, [this, watcher] {
        const ThumbnailJob thumbnail = watcher->result();
        watcher->deleteLater();
        m_thumbnailInFlight = false;
        if (thumbnail.generation == m_thumbnailGeneration && !thumbnail.file.isEmpty()) {
            for (std::size_t row = 0; row < m_items.size(); ++row) {
                if (m_items[row].path != thumbnail.path)
                    continue;
                m_items[row].thumbnail = QUrl::fromLocalFile(thumbnail.file);
                const QModelIndex at = index(static_cast<int>(row));
                emit dataChanged(at, at, {ThumbnailRole});
            }
            if (thumbnail.rendered) {
                ++m_thumbnailsRendered;
                emit thumbnailsRenderedChanged();
            }
        }
        pumpThumbnails();
    });
    watcher->setFuture(QtConcurrent::run(
        &m_pool, [generation, item, width] { return makeThumbnail(generation, item, width); }));
}

// `path` renommé ou supprimé : si le dossier courant est `path` ou en dessous, il passe au parent de `path`.
void ResourceLibrary::leaveFolder(const QString& path)
{
    if (m_folder != path && !m_folder.startsWith(path + QLatin1Char('/')))
        return;
    m_folder = QFileInfo(path).absolutePath();
    emit folderChanged();
}

bool ResourceLibrary::mayEdit() const
{
    return m_commands != nullptr && m_document != nullptr && !m_commands->isLocked();
}

bool ResourceLibrary::rename(const QString& path, const QString& newName)
{
    const QString name = newName.trimmed();
    const QFileInfo info(path);
    const QString source = QDir::cleanPath(info.absoluteFilePath());
    if (m_commands == nullptr || m_commands->isLocked() || m_contactActive || !validName(name) ||
        source == m_rootPath || !info.exists())
        return false;
    QString target = name;
    if (info.isFile() && QFileInfo(name).suffix().compare(info.suffix(), Qt::CaseInsensitive) != 0)
        target += QLatin1Char('.') + info.suffix();
    const QString destination = info.dir().filePath(target);
    if (QFileInfo::exists(destination))
        return false;
    const bool done = info.isDir() ? QDir().rename(source, destination) : QFile::rename(source, destination);
    if (done) {
        leaveFolder(source);
        refresh();
    }
    return done;
}

bool ResourceLibrary::remove(const QString& path, bool permanently)
{
    const QFileInfo info(path);
    const QString target = QDir::cleanPath(info.absoluteFilePath());
    if (m_commands == nullptr || m_commands->isLocked() || m_contactActive || target == m_rootPath ||
        !info.exists())
        return false;
    bool done = false;
    if (usesTrash() && !permanently) {
        done = !hooks().trashFails && QFile::moveToTrash(target);
        if (!done) {
            emit trashFailed(target); // rien n'est supprimé ; le QML peut proposer la suppression définitive
            return false;
        }
    } else if (info.isDir()) {
        done = QDir(target).removeRecursively();
    } else {
        done = QFile::remove(target);
    }
    if (done) {
        leaveFolder(target);
        refresh();
    }
    return done;
}

bool ResourceLibrary::hasItem(const QString& path) const
{
    return std::any_of(m_items.begin(), m_items.end(),
                       [&path](const Item& item) { return item.path == path; });
}

bool ResourceLibrary::createFolder(const QString& name)
{
    const QString trimmed = name.trimmed();
    if (m_commands == nullptr || m_commands->isLocked() || m_contactActive || m_folder.isEmpty() ||
        !validName(trimmed))
        return false;
    const bool done = QDir(m_folder).mkdir(trimmed);
    if (done)
        refresh();
    return done;
}

QString ResourceLibrary::addSelection()
{
    if (!mayEdit() || m_contactActive || m_tools == nullptr || m_folder.isEmpty())
        return {};
    // Diffusion protégée (L8-T32, V58) : glisser vers les ressources copie le contenu, hors registre ; refusé
    // et signalé comme la commande resources.add.
    if (core::forbids(m_document->document().broadcast(), core::Restriction::Copy)) {
        m_commands->reportRestrictionRefusal(QStringLiteral("resources.add"));
        return {};
    }
    const std::vector<core::ObjectId>& selected = m_tools->user().selection();
    if (selected.empty())
        return {};
    const QDir dir(m_folder);
    if (!dir.exists() && !QDir().mkpath(m_folder))
        return {};
    QString path;
    for (int n = 1; n <= kMaxResourceNumber && path.isEmpty(); ++n) {
        const QString candidate = dir.filePath(QStringLiteral("Ressource%1.actcraft").arg(n));
        if (!QFileInfo::exists(candidate))
            path = candidate;
    }
    if (path.isEmpty())
        return {};
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly))
        return {};
    QString error;
    if (!formats::writeFragment(m_document->document(), m_document->currentIndex(), selected, file, error) ||
        !file.commit())
        return {};
    refresh();
    return path;
}

bool ResourceLibrary::insert(const QString& path, QPointF pagePoint)
{
    // Un fragment se lit et s'analyse ici, sur le fil principal : refusé pendant un contact, comme les autres
    // éditions disque (l'image aussi, pour une seule règle).
    if (!mayEdit() || m_contactActive)
        return false;
    const QFileInfo info(path);
    const QString kind = kindOf(info);
    if (kind == QLatin1String(kImageKind))
        return m_insertion.acceptDrop({QUrl::fromLocalFile(info.absoluteFilePath())}, {}, pagePoint);
    if (kind != QLatin1String(kFragmentKind))
        return false;
    std::optional<formats::Fragment> fragment = readFragmentFile(info.absoluteFilePath());
    if (!fragment || fragment->page.objects.empty())
        return false;
    // Centre de l'ensemble posé sur pagePoint.
    const std::optional<core::PointF> center = core::centerOfExtents(fragment->page.objects);
    const double dx = center ? pagePoint.x() - center->x : 0;
    const double dy = center ? pagePoint.y() - center->y : 0;
    m_document->execute(std::make_unique<InsertFragmentCommand>(
        static_cast<std::size_t>(m_document->currentIndex()), std::move(*fragment), dx, dy));
    return true;
}

} // namespace actcraft::ui
