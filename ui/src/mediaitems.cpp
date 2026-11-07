#include <actcraft/media/player.h>
#include <actcraft/ui/mediaitems.h>
#include <actcraft/ui/toolhost.h>

#include <QDataStream>
#include <QFile>
#include <QFileInfo>
#include <QFutureWatcher>
#include <QRegularExpression>
#include <QTemporaryDir>
#include <QUrl>
#include <QtConcurrent>

#include <algorithm>
#include <cmath>
#include <numbers>
#include <variant>

namespace actcraft::ui {

namespace {

bool isVideoExtension(const QString& extension)
{
    static const QStringList videos{"mp4", "m4v", "mov", "avi", "webm", "mkv", "mpg", "mpeg", "wmv"};
    return videos.contains(extension.toLower());
}

// Règle de media::Player (et de LinkLauncher) : un chemin réseau est refusé avant tout accès au disque (sous
// Windows, tester son existence envoie l'authentification du compte à l'hôte).
bool isNetworkPath(QString path)
{
    path.replace(u'\\', u'/');
    return path.startsWith(QStringLiteral("//"));
}

// Nom du fichier extrait d'une ressource : clé assainie (jamais de séparateur) et extension du document.
QString extractedName(const std::string& key, const std::string& extension)
{
    QString name = QString::fromStdString(key);
    name.replace(QRegularExpression(QStringLiteral("[^A-Za-z0-9_-]")), QStringLiteral("_"));
    return name + QLatin1Char('.') + QString::fromStdString(extension);
}

// Nom d'origine d'une ressource pour la confirmation : le nom du média de la page qui la porte, sinon la clé.
QString originalName(const core::Page& page, const std::string& key, const std::string& extension)
{
    for (const core::PageObject& object : page.objects)
        if (const auto* media = std::get_if<core::Media>(&object))
            if (media->media == key && !media->fileName.empty())
                return QFileInfo(QString::fromStdString(media->fileName)).fileName();
    return QString::fromStdString(key + "." + extension);
}

// Son de confirmation par défaut, écrit par nous : 0,25 s à 880 Hz, mono 16 bits, attaque et chute douces.
QByteArray chimeWave()
{
    constexpr int rate = 22050;
    constexpr int samples = rate / 4;
    QByteArray pcm;
    pcm.reserve(samples * 2);
    for (int i = 0; i < samples; ++i) {
        const double t = static_cast<double>(i) / rate;
        const double envelope = std::min(1.0, i / 200.0) * (1.0 - static_cast<double>(i) / samples);
        const auto value = static_cast<qint16>(
            std::lround(12000.0 * envelope * std::sin(2.0 * std::numbers::pi * 880.0 * t)));
        pcm.append(static_cast<char>(value & 0xFF));
        pcm.append(static_cast<char>((value >> 8) & 0xFF));
    }
    QByteArray wav;
    QDataStream out(&wav, QIODevice::WriteOnly);
    out.setByteOrder(QDataStream::LittleEndian);
    out.writeRawData("RIFF", 4);
    out << static_cast<quint32>(36 + pcm.size());
    out.writeRawData("WAVEfmt ", 8);
    out << quint32(16) << quint16(1) << quint16(1) << quint32(rate) << quint32(rate * 2) << quint16(2)
        << quint16(16);
    out.writeRawData("data", 4);
    out << static_cast<quint32>(pcm.size());
    out.writeRawData(pcm.constData(), static_cast<int>(pcm.size()));
    return wav;
}

} // namespace

MediaItems::MediaItems(QObject* parent)
    : QAbstractListModel(parent), m_player(std::make_unique<media::Player>())
{
    m_refresh.setSingleShot(true);
    m_refresh.setInterval(0); // regroupe les changements d'un même tour de boucle
    connect(&m_refresh, &QTimer::timeout, this, &MediaItems::refresh);
    m_player->setFallback([this](const QString& path) { openWithSystem(path); }); // confirmation (N2-9)
    connect(m_player.get(), &media::Player::positionChanged, this, [this](int track, qint64) {
        if (track == media::Player::First)
            updateSound();
    });
    connect(m_player.get(), &media::Player::finished, this, [this](int) { updateSound(); });
    connect(m_player.get(), &media::Player::failed, this, [this](int track, const QString& reason) {
        updateSound();
        if (track == media::Player::First)
            emit soundFailed(reason); // l'exécuteur d'actions le refuse (actionRefused)
    });
    connect(m_player.get(), &media::Player::fellBack, this, [this](const QString&) { updateSound(); });
}

MediaItems::~MediaItems()
{
    m_player->stopAll();
}

DocumentController* MediaItems::document() const
{
    return m_document;
}

void MediaItems::setDocument(DocumentController* document)
{
    if (m_document == document)
        return;
    if (m_document)
        disconnect(m_document, nullptr, this, nullptr);
    m_document = document;
    resetForDocument(); // un autre contrôleur : rien de l'ancien ne joue ni ne reste sur le disque
    if (m_document) {
        connect(m_document, &DocumentController::pagesChanged, this, [this] { scheduleRefresh(true); });
        connect(m_document, &DocumentController::pageContentChanged, this, [this](int page) {
            if (m_document && page == m_document->currentIndex())
                scheduleRefresh(false);
        });
        connect(m_document, &DocumentController::documentChanged, this, [this] { scheduleRefresh(false); });
    }
    scheduleRefresh(true);
    emit documentChanged();
}

bool MediaItems::contactActive() const
{
    return m_contactActive;
}

void MediaItems::setContactActive(bool active)
{
    if (m_contactActive == active)
        return;
    m_contactActive = active;
    emit contactActiveChanged();
    if (!m_contactActive && m_pending)
        scheduleRefresh(false); // la relecture reportée part à la retombée du contact
}

LinkLauncher* MediaItems::links() const
{
    return m_links;
}

void MediaItems::setLinks(LinkLauncher* links)
{
    if (m_links == links)
        return;
    m_links = links;
    emit linksChanged();
}

ToolHost* MediaItems::tools() const
{
    return m_tools;
}

void MediaItems::setTools(ToolHost* tools)
{
    if (m_tools == tools)
        return;
    if (m_tools)
        disconnect(m_tools, nullptr, this, nullptr);
    m_tools = tools;
    if (m_tools)
        connect(m_tools, &ToolHost::containerAccepted, this, &MediaItems::onContainerAccepted);
    emit toolsChanged();
}

bool MediaItems::soundPlaying() const
{
    return m_soundPlaying;
}

qint64 MediaItems::soundPosition() const
{
    return m_soundPosition;
}

QString MediaItems::soundName() const
{
    return m_soundName;
}

media::Player* MediaItems::player() const
{
    return m_player.get();
}

int MediaItems::rowCount(const QModelIndex& parent) const
{
    return parent.isValid() ? 0 : static_cast<int>(m_entries.size());
}

QVariant MediaItems::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= rowCount())
        return {};
    const Entry& e = m_entries[static_cast<std::size_t>(index.row())];
    switch (role) {
    case IdRole:
        return QVariant::fromValue(e.id);
    case KindRole:
        return e.kind;
    case SourceRole:
        return QUrl::fromLocalFile(e.source).toString();
    case BoundsRole:
        return e.bounds;
    case ControlsHiddenRole:
        return e.controlsHidden;
    case LoopRole:
        return e.loop;
    case AspectLockedRole:
        return e.aspectLocked;
    case ClearBackgroundRole:
        return e.clearBackground;
    case WrapTextRole:
        return e.wrapText;
    case AutoPlayRole:
        return e.autoPlay;
    default:
        return {};
    }
}

QHash<int, QByteArray> MediaItems::roleNames() const
{
    return {{IdRole, "mediaId"},
            {KindRole, "kind"},
            {SourceRole, "mediaSource"}, // pas « source » : masquerait Loader::source dans le délégué
            {BoundsRole, "bounds"},
            {ControlsHiddenRole, "controlsHidden"},
            {LoopRole, "loop"},
            {AspectLockedRole, "aspectLocked"},
            {ClearBackgroundRole, "clearBackground"},
            {WrapTextRole, "wrapText"},
            {AutoPlayRole, "autoPlay"}};
}

MediaItems::Entry* MediaItems::entryById(quint64 id)
{
    const auto it =
        std::find_if(m_entries.begin(), m_entries.end(), [id](const Entry& e) { return e.id == id; });
    return it == m_entries.end() ? nullptr : &*it;
}

bool MediaItems::play(quint64 id)
{
    const Entry* entry = entryById(id);
    if (!entry)
        return false;
    if (entry->kind == QLatin1String("video")) {
        if (!entry->source.isEmpty()) {
            emit videoPlayRequested(id);
            return true;
        }
        // Extraction à la demande : la première lecture écrit le fichier, hors du fil principal.
        return extract(entry->resource, id, [this, id](const QString& path) {
            if (path.isEmpty()) {
                emit soundFailed(QStringLiteral("fichier non extrait du document"));
                return;
            }
            Entry* current = entryById(id); // la page a pu changer pendant l'écriture
            if (!current)
                return;
            if (current->source.isEmpty()) {
                current->source = path;
                const QModelIndex at = index(static_cast<int>(current - m_entries.data()));
                emit dataChanged(at, at, {SourceRole}); // avant la demande : le lecteur lit la source
            }
            emit videoPlayRequested(id);
        });
    }
    if (entry->kind != QLatin1String("sound"))
        return false;
    const std::uint64_t request = ++m_soundRequest;
    return extract(entry->resource, id, [this, request, loop = entry->loop](const QString& path) {
        if (request != m_soundRequest)
            return; // une demande plus récente, un arrêt ou un changement de page est passé par là
        if (path.isEmpty())
            emit soundFailed(QStringLiteral("fichier non extrait du document"));
        else
            startSound(path, loop);
    });
}

void MediaItems::videoFailed(quint64 id)
{
    const auto it =
        std::find_if(m_entries.begin(), m_entries.end(), [id](const Entry& e) { return e.id == id; });
    if (it != m_entries.end() && !it->source.isEmpty())
        openWithSystem(it->source);
}

bool MediaItems::playResource(const std::string& key)
{
    const std::uint64_t request = ++m_soundRequest;
    return extract(key, 0, [this, request](const QString& path) {
        if (request != m_soundRequest)
            return;
        if (path.isEmpty())
            emit soundFailed(QStringLiteral("fichier non extrait du document"));
        else
            startSound(path, false);
    });
}

bool MediaItems::isSoundResource(const std::string& key) const
{
    const core::Resource* stored =
        key.empty() || !m_document ? nullptr : m_document->document().resource(key);
    if (!stored)
        return false;
    const QString extension = QString::fromStdString(stored->extension);
    return !isVideoExtension(extension) && extension.compare(QLatin1String("swf"), Qt::CaseInsensitive) != 0;
}

// Refus d'emblée (le lecteur ne le dirait que plus tard, par signal) : chemin réseau, avant tout accès au
// disque, puis fichier absent. L'action est alors refusée et le lecteur de sons ne s'ouvre pas.
bool MediaItems::playFile(const QString& path)
{
    if (path.isEmpty() || isNetworkPath(path) || !QFileInfo(path).isFile())
        return false;
    return startSound(path, false);
}

bool MediaItems::startSound(const QString& path, bool loop)
{
    m_soundName = QFileInfo(path).fileName();
    m_player->play(path, media::Player::First, loop);
    updateSound();
    emit soundStarted();
    return true;
}

void MediaItems::stopSound()
{
    ++m_soundRequest; // un son encore en cours d'écriture ne partira pas
    m_player->pause(media::Player::First);
    updateSound();
}

void MediaItems::onContainerAccepted(quint64 container, bool chime)
{
    if (!chime || !m_document)
        return;
    const core::Page& page =
        m_document->document().page(static_cast<std::size_t>(m_document->currentIndex()));
    std::string key;
    for (const core::PageObject& object : page.objects) {
        if (object.common.id == container) {
            key = object.common.container.chimeSound;
            break;
        }
    }
    // Piste 2 : le son de la piste 1 continue. Écriture hors du fil principal, comme play() : le son du
    // conteneur passe par extract(), le son par défaut par playDefaultChime().
    const auto onTrack2 = [this, epoch = m_epoch](const QString& path) {
        if (epoch == m_epoch && !path.isEmpty())
            m_player->play(path, media::Player::Second, false);
    };
    if (!key.empty() && extract(key, 0, onTrack2))
        return;
    playDefaultChime();
}

void MediaItems::scheduleRefresh(bool pageShown)
{
    m_pendingPageShown = m_pendingPageShown || pageShown;
    m_pending = true;
    if (!m_contactActive)
        m_refresh.start();
}

void MediaItems::refresh()
{
    if (m_contactActive)
        return; // reporté : setContactActive(false) relance
    bool pageShown = m_pendingPageShown;
    m_pending = false;
    m_pendingPageShown = false;
    if (m_document && m_document->generation() != m_generation) {
        resetForDocument(); // document remplacé : l'ancien ne joue plus et ses fichiers extraits sont
                            // supprimés
        pageShown = true;   // les médias du nouveau sont montrés comme à l'ouverture
    }
    std::vector<Entry> next = collect(pageShown);
    if (!pageShown) {
        for (Entry& e : next) { // une édition ne relance jamais une lecture auto
            const auto old = std::find_if(m_entries.begin(), m_entries.end(),
                                          [&e](const Entry& o) { return o.id == e.id; });
            if (old == m_entries.end())
                continue;
            if (old->resource == e.resource) {
                e.autoPlay = old->autoPlay;
                if (e.source.isEmpty())
                    e.source = old->source; // déjà extrait à la demande : la vidéo qui joue n'est pas touchée
                e.needsFile = e.source.isEmpty() && (e.needsFile || old->needsFile); // écriture en cours
            }
        }
        if (next == m_entries)
            return;                 // trait, édition d'un autre objet : la vidéo qui joue n'est pas touchée
        applyRows(std::move(next)); // par identifiant : la vidéo qui joue garde sa ligne et son délégué
        fetchFiles();
        return;
    }
    beginResetModel();
    m_entries = std::move(next);
    endResetModel();
    emit itemsChanged();
    fetchFiles();
    if (pageShown) {
        for (const Entry& e : m_entries) {
            if (e.kind == QLatin1String("sound") && e.playOnOpen) {
                play(e.id); // lecteur intégré seulement, jamais LinkLauncher
                break;
            }
        }
    }
}

// Mise à jour du modèle par identifiant (jamais de remise à zéro hors changement de page ou de document) :
// le Repeater de MediaLayer garde le délégué d'une vidéo qui joue quand un autre média arrive, part, est
// masqué ou change de rang. Un média dont l'identifiant reste mais dont le type ou la ressource change est
// un autre média : retiré puis inséré.
void MediaItems::applyRows(std::vector<Entry> next)
{
    const auto sameMedia = [](const Entry& a, const Entry& b) {
        return a.id == b.id && a.kind == b.kind && a.resource == b.resource;
    };
    const auto wanted = [&](const Entry& e) {
        return std::any_of(next.begin(), next.end(), [&](const Entry& n) { return sameMedia(n, e); });
    };
    bool structural = false;
    // Retraits, de la fin vers le début, par plages contiguës.
    int row = static_cast<int>(m_entries.size()) - 1;
    while (row >= 0) {
        if (wanted(m_entries[static_cast<std::size_t>(row)])) {
            --row;
            continue;
        }
        const int last = row;
        while (row > 0 && !wanted(m_entries[static_cast<std::size_t>(row - 1)]))
            --row;
        beginRemoveRows({}, row, last);
        m_entries.erase(m_entries.begin() + row, m_entries.begin() + last + 1);
        endRemoveRows();
        structural = true;
        --row;
    }
    // Ne restent que des médias voulus, dans l'ancien ordre : on les place un à un, on insère les nouveaux.
    for (std::size_t i = 0; i < next.size(); ++i) {
        const int at = static_cast<int>(i);
        if (i >= m_entries.size() || !sameMedia(m_entries[i], next[i])) {
            const auto found =
                std::find_if(m_entries.begin() + static_cast<std::ptrdiff_t>(std::min(i, m_entries.size())),
                             m_entries.end(), [&](const Entry& e) { return sameMedia(e, next[i]); });
            if (found != m_entries.end()) {
                const int from = static_cast<int>(found - m_entries.begin());
                beginMoveRows({}, from, from, {}, at);
                std::rotate(m_entries.begin() + at, found, found + 1);
                endMoveRows();
            } else {
                beginInsertRows({}, at, at);
                m_entries.insert(m_entries.begin() + at, next[i]);
                endInsertRows();
            }
            structural = true;
        }
        if (!(m_entries[i] == next[i])) {
            m_entries[i] = next[i];
            emit dataChanged(index(at), index(at));
        }
    }
    if (m_entries.size() > next.size()) { // identifiants en double dans un document abîmé : jamais de reste
        beginRemoveRows({}, static_cast<int>(next.size()), static_cast<int>(m_entries.size()) - 1);
        m_entries.resize(next.size());
        endRemoveRows();
        structural = true;
    }
    if (structural)
        emit itemsChanged();
}

std::vector<MediaItems::Entry> MediaItems::collect(bool pageShown)
{
    std::vector<Entry> out;
    if (!m_document)
        return out;
    const int current = m_document->currentIndex();
    if (current < 0 || static_cast<std::size_t>(current) >= m_document->document().pageCount())
        return out;
    if (pageShown) {
        if (m_shownPage >= 0 && m_shownPage != current)
            ++m_soundRequest; // un son demandé pour la page précédente ne part pas sur celle-ci
        m_shownPage = current;
    }
    const core::Document& document = m_document->document();
    const core::Page& page = document.page(static_cast<std::size_t>(current));
    for (const core::PageObject& object : page.objects) {
        if (!object.common.visible)
            continue; // objet masqué : ni montré ni joué
        Entry e;
        e.id = object.common.id;
        if (const auto* media = std::get_if<core::Media>(&object)) {
            const core::Resource* stored = media->media.empty() ? nullptr : document.resource(media->media);
            if (!stored)
                continue;
            const QString extension = QString::fromStdString(stored->extension);
            if (extension.compare(QLatin1String("swf"), Qt::CaseInsensitive) == 0)
                continue; // objet Flash : jamais joué ici
            e.kind = isVideoExtension(extension) ? QStringLiteral("video") : QStringLiteral("sound");
            e.resource = media->media; // extraite à la première lecture, pas à l'affichage de la page
            e.bounds = QRectF(media->bounds.x, media->bounds.y, media->bounds.width, media->bounds.height);
            e.controlsHidden = media->controlsHidden;
            e.loop = media->loop;
            e.aspectLocked = media->aspectLocked;
            e.clearBackground = media->clearBackground;
            e.wrapText = media->wrapText;
            e.playOnOpen = media->playOnOpen;
            // Lecture demandée par le document lui-même : le fichier est nécessaire. Il est écrit hors du fil
            // principal (fetchFiles) ; autoPlay ne passe à vrai qu'une fois la source arrivée.
            e.needsFile = pageShown && media->playOnOpen && e.kind == QLatin1String("video");
        } else if (const auto* image = std::get_if<core::ImageObject>(&object)) {
            const core::Resource* stored =
                image->resource.empty() ? nullptr : document.resource(image->resource);
            if (!stored || QString::fromStdString(stored->extension)
                                   .compare(QLatin1String("gif"), Qt::CaseInsensitive) != 0)
                continue;
            e.kind = QStringLiteral("gif"); // V57 : animé par la surcouche, première image dans le cache
            e.resource = image->resource;
            e.needsFile = true; // le GIF s'anime dès l'affichage : extrait aussitôt (hors du fil principal)
            e.bounds = QRectF(image->bounds.x, image->bounds.y, image->bounds.width, image->bounds.height);
        } else {
            continue;
        }
        out.push_back(std::move(e));
    }
    return out;
}

void MediaItems::resetForDocument()
{
    m_player->stopAll();
    m_files.clear();
    m_chime.clear();
    m_chimeWriting = false;
    ++m_epoch; // les écritures en cours sont périmées : leur résultat sera jeté
    m_writing.clear();
    retireDirectory(); // les fichiers extraits de l'ancien document partent, mais pas avant les délégués
    m_soundName.clear();
    m_generation = m_document ? m_document->generation() : 0;
    updateSound();
}

void MediaItems::openWithSystem(const QString& path)
{
    if (!m_links)
        return;
    if (m_document) {
        for (const auto& [key, file] : m_files) {
            if (file != path)
                continue;
            if (const core::Resource* stored = m_document->document().resource(key)) {
                const int current = m_document->currentIndex();
                const bool valid =
                    current >= 0 && static_cast<std::size_t>(current) < m_document->document().pageCount();
                const QString name =
                    valid ? originalName(m_document->document().page(static_cast<std::size_t>(current)), key,
                                         stored->extension)
                          : QString::fromStdString(key + "." + stored->extension);
                m_links->requestStored(stored->data, name, m_document->fileUrl().toString());
                return;
            }
        }
    }
    m_links->request(path, LinkLauncher::SystemMedia); // fichier du disque : pas extrait du document
}

QString MediaItems::knownFile(const std::string& key) const
{
    const auto known = m_files.find(key);
    return known != m_files.end() && QFileInfo::exists(known->second) ? known->second : QString();
}

bool MediaItems::extract(const std::string& key, quint64 requester, Ready done)
{
    if (key.empty() || !m_document)
        return false;
    const core::Resource* stored = m_document->document().resource(key);
    if (!stored)
        return false;
    if (const QString known = knownFile(key); !known.isEmpty()) {
        if (done)
            done(known);
        return true;
    }
    if (const auto writing = m_writing.find(key); writing != m_writing.end()) {
        std::vector<Waiter>& waiters = writing->second; // écriture déjà lancée : on s'y joint, une fois
        if (done && std::none_of(waiters.begin(), waiters.end(),
                                 [requester](const Waiter& w) { return w.requester == requester; }))
            waiters.push_back({requester, std::move(done)});
        return true;
    }
    if (!m_dir)
        m_dir = std::make_shared<QTemporaryDir>();
    if (!m_dir->isValid())
        return false;
    const QString path = m_dir->filePath(extractedName(key, stored->extension));
    std::vector<Waiter>& waiters = m_writing[key];
    if (done)
        waiters.push_back({requester, std::move(done)});
    // Écriture sur le pool global (BRIEF § 4, règle 1) : la ressource est copiée ici (QByteArray partagé),
    // le fil secondaire ne lit jamais le document. Le dossier suit l'écriture (un document remplacé ne le
    // supprime qu'une fois l'écriture finie) ; le résultat revient par la boucle, avec la génération du
    // moment (document remplacé : jeté).
    auto* watcher = new QFutureWatcher<bool>(this);
    connect(watcher, &QFutureWatcherBase::finished, this, [this, watcher, key, path, epoch = m_epoch] {
        const bool ok = watcher->result();
        watcher->deleteLater();
        onWritten(key, path, ok, epoch);
    });
    watcher->setFuture(QtConcurrent::run([dir = m_dir, path, data = stored->data]() -> bool {
        static_cast<void>(dir); // retient le dossier jusqu'à la fin de l'écriture
        QFile file(path);
        if (!file.open(QIODevice::WriteOnly))
            return false;
        const bool ok = file.write(data) == data.size();
        file.close();
        return ok;
    }));
    return true;
}

void MediaItems::onWritten(const std::string& key, const QString& path, bool ok, std::uint64_t epoch)
{
    if (epoch != m_epoch)
        return; // document remplacé pendant l'écriture : résultat jeté
    std::vector<Waiter> waiters;
    if (const auto writing = m_writing.find(key); writing != m_writing.end()) {
        waiters = std::move(writing->second);
        m_writing.erase(writing);
    }
    if (!m_document || m_document->generation() != m_generation)
        return; // remplacé, relecture pas encore passée : rien ne se lance pour un document disparu
    if (ok) {
        m_files[key] = path;
        fillEntries(key, path);
    }
    for (Waiter& waiter : waiters)
        waiter.done(ok ? path : QString());
}

void MediaItems::fillEntries(const std::string& key, const QString& path)
{
    for (std::size_t i = 0; i < m_entries.size(); ++i) {
        if (m_entries[i].resource != key || !m_entries[i].needsFile)
            continue;
        const bool video = m_entries[i].kind == QLatin1String("video");
        m_entries[i].source = path;
        m_entries[i].needsFile = false;
        m_entries[i].autoPlay = video; // seule une vidéo en lecture auto attend son fichier pour jouer
        const QModelIndex at = index(static_cast<int>(i));
        emit dataChanged(at, at, {SourceRole}); // la source d'abord : le lecteur armé par autoPlay la lit
        if (video)
            emit dataChanged(at, at, {AutoPlayRole});
    }
}

void MediaItems::fetchFiles()
{
    std::vector<std::string> keys;
    for (const Entry& e : m_entries)
        if (e.needsFile && e.source.isEmpty())
            keys.push_back(e.resource);
    for (const std::string& key : keys) {
        if (const QString known = knownFile(key); !known.isEmpty())
            fillEntries(key, known);
        else
            extract(key, 0, {});
    }
}

// Les délégués QML (MediaPlayer, AnimatedImage) tiennent leur fichier ouvert jusqu'à leur destruction, qui
// suit la remise à zéro du modèle ; Windows refuse de supprimer un fichier ouvert. Le dossier de l'ancien
// document est donc gardé deux tours de boucle, puis lâché (à la destruction du modèle au plus tard).
void MediaItems::retireDirectory()
{
    if (!m_dir)
        return;
    QTimer::singleShot(0, this, [this, dir = std::move(m_dir)] {
        QTimer::singleShot(0, this, [dir] { static_cast<void>(dir); });
    });
    m_dir.reset();
}

// Son de confirmation par défaut : écrit une fois, hors du fil principal, puis joué sur la piste 2.
void MediaItems::playDefaultChime()
{
    if (!m_chime.isEmpty() && QFileInfo::exists(m_chime)) {
        m_player->play(m_chime, media::Player::Second, false);
        return;
    }
    if (m_chimeWriting)
        return; // écriture en cours : la confirmation en cours de préparation suffit
    if (!m_dir)
        m_dir = std::make_shared<QTemporaryDir>();
    if (!m_dir->isValid())
        return;
    const QString path = m_dir->filePath(QStringLiteral("chime.wav"));
    m_chimeWriting = true;
    auto* watcher = new QFutureWatcher<bool>(this);
    connect(watcher, &QFutureWatcherBase::finished, this, [this, watcher, path, epoch = m_epoch] {
        const bool ok = watcher->result();
        watcher->deleteLater();
        if (epoch != m_epoch)
            return; // document remplacé pendant l'écriture : résultat jeté
        m_chimeWriting = false;
        if (!ok)
            return;
        m_chime = path;
        m_player->play(path, media::Player::Second, false);
    });
    watcher->setFuture(QtConcurrent::run([dir = m_dir, path]() -> bool {
        static_cast<void>(dir); // retient le dossier jusqu'à la fin de l'écriture
        QFile file(path);
        if (!file.open(QIODevice::WriteOnly))
            return false;
        const QByteArray wave = chimeWave();
        const bool ok = file.write(wave) == wave.size();
        file.close();
        return ok;
    }));
}

void MediaItems::updateSound()
{
    const bool playing = m_player->isPlaying(media::Player::First);
    const qint64 position = m_player->position(media::Player::First);
    if (playing == m_soundPlaying && position == m_soundPosition)
        return;
    m_soundPlaying = playing;
    m_soundPosition = position;
    emit soundStateChanged();
}

} // namespace actcraft::ui
