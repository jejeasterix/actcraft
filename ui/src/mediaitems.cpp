#include <actcraft/media/player.h>
#include <actcraft/ui/mediaitems.h>
#include <actcraft/ui/toolhost.h>

#include <QDataStream>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <QTemporaryDir>
#include <QUrl>

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
    m_player->setFallback([this](const QString& path) {
        if (m_links)
            m_links->request(path, LinkLauncher::SystemMedia); // confirmation demandée (N2-9)
    });
    connect(m_player.get(), &media::Player::positionChanged, this, [this](int track, qint64) {
        if (track == media::Player::First)
            updateSound();
    });
    connect(m_player.get(), &media::Player::finished, this, [this](int) { updateSound(); });
    connect(m_player.get(), &media::Player::failed, this, [this](int, const QString&) { updateSound(); });
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
            {SourceRole, "source"},
            {BoundsRole, "bounds"},
            {ControlsHiddenRole, "controlsHidden"},
            {LoopRole, "loop"},
            {AspectLockedRole, "aspectLocked"},
            {ClearBackgroundRole, "clearBackground"},
            {WrapTextRole, "wrapText"},
            {AutoPlayRole, "autoPlay"}};
}

bool MediaItems::play(quint64 id)
{
    const auto it =
        std::find_if(m_entries.begin(), m_entries.end(), [id](const Entry& e) { return e.id == id; });
    if (it == m_entries.end())
        return false;
    if (it->kind == QLatin1String("video")) {
        emit videoPlayRequested(id);
        return true;
    }
    if (it->kind != QLatin1String("sound"))
        return false;
    m_soundName = QFileInfo(it->source).fileName();
    m_player->play(it->source, media::Player::First, it->loop);
    updateSound();
    emit soundStarted();
    return true;
}

void MediaItems::stopSound()
{
    m_player->pause(media::Player::First);
    updateSound();
}

void MediaItems::onContainerAccepted(quint64 container, bool chime)
{
    if (!chime || !m_document)
        return;
    const core::Page& page =
        m_document->document().page(static_cast<std::size_t>(m_document->currentIndex()));
    QString path;
    for (const core::PageObject& object : page.objects) {
        if (object.common.id == container) {
            path = fileFor(object.common.container.chimeSound);
            break;
        }
    }
    if (path.isEmpty())
        path = defaultChime();
    if (!path.isEmpty())
        m_player->play(path, media::Player::Second, false); // piste 2 : le son de la piste 1 continue
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
    const bool pageShown = m_pendingPageShown;
    m_pending = false;
    m_pendingPageShown = false;
    std::vector<Entry> next = collect(pageShown);
    if (!pageShown) {
        for (Entry& e : next) { // une édition ne relance jamais une lecture auto
            const auto old = std::find_if(m_entries.begin(), m_entries.end(),
                                          [&e](const Entry& o) { return o.id == e.id; });
            e.autoPlay = old != m_entries.end() && old->autoPlay;
        }
        if (next == m_entries)
            return; // trait, édition d'un autre objet : la vidéo qui joue n'est pas touchée
        const bool sameRows =
            next.size() == m_entries.size() &&
            std::equal(next.begin(), next.end(), m_entries.begin(), [](const Entry& a, const Entry& b) {
                return a.id == b.id && a.kind == b.kind && a.source == b.source;
            });
        if (sameRows) {
            for (std::size_t i = 0; i < next.size(); ++i) {
                if (!(next[i] == m_entries[i])) {
                    m_entries[i] = next[i];
                    const QModelIndex at = index(static_cast<int>(i));
                    emit dataChanged(at, at);
                }
            }
            return;
        }
    }
    beginResetModel();
    m_entries = std::move(next);
    endResetModel();
    emit itemsChanged();
    if (pageShown) {
        for (const Entry& e : m_entries) {
            if (e.kind == QLatin1String("sound") && e.playOnOpen) {
                play(e.id); // lecteur intégré seulement, jamais LinkLauncher
                break;
            }
        }
    }
}

std::vector<MediaItems::Entry> MediaItems::collect(bool pageShown)
{
    std::vector<Entry> out;
    if (!m_document)
        return out;
    const int current = m_document->currentIndex();
    if (current < 0 || static_cast<std::size_t>(current) >= m_document->document().pageCount())
        return out;
    if (pageShown)
        m_shownPage = current;
    const core::Document& document = m_document->document();
    const core::Page& page = document.page(static_cast<std::size_t>(current));
    for (const core::PageObject& object : page.objects) {
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
            e.source = fileFor(media->media);
            e.bounds = QRectF(media->bounds.x, media->bounds.y, media->bounds.width, media->bounds.height);
            e.controlsHidden = media->controlsHidden;
            e.loop = media->loop;
            e.aspectLocked = media->aspectLocked;
            e.clearBackground = media->clearBackground;
            e.wrapText = media->wrapText;
            e.playOnOpen = media->playOnOpen;
            e.autoPlay = pageShown && media->playOnOpen && e.kind == QLatin1String("video");
        } else if (const auto* image = std::get_if<core::ImageObject>(&object)) {
            const core::Resource* stored =
                image->resource.empty() ? nullptr : document.resource(image->resource);
            if (!stored || QString::fromStdString(stored->extension)
                                   .compare(QLatin1String("gif"), Qt::CaseInsensitive) != 0)
                continue;
            e.kind = QStringLiteral("gif"); // V57 : animé par la surcouche, première image dans le cache
            e.source = fileFor(image->resource);
            e.bounds = QRectF(image->bounds.x, image->bounds.y, image->bounds.width, image->bounds.height);
        } else {
            continue;
        }
        if (!e.source.isEmpty())
            out.push_back(std::move(e));
    }
    return out;
}

QString MediaItems::fileFor(const std::string& key)
{
    if (key.empty() || !m_document)
        return {};
    const core::Resource* stored = m_document->document().resource(key);
    if (!stored)
        return {};
    if (const auto known = m_files.find(key); known != m_files.end() && QFileInfo::exists(known->second))
        return known->second;
    if (!m_dir)
        m_dir = std::make_unique<QTemporaryDir>();
    if (!m_dir->isValid())
        return {};
    QString name = QString::fromStdString(key);
    name.replace(QRegularExpression(QStringLiteral("[^A-Za-z0-9_-]")), QStringLiteral("_"));
    const QString path = m_dir->filePath(name + QLatin1Char('.') + QString::fromStdString(stored->extension));
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly) || file.write(stored->data) != stored->data.size())
        return {};
    file.close();
    m_files[key] = path;
    return path;
}

QString MediaItems::defaultChime()
{
    if (!m_chime.isEmpty() && QFileInfo::exists(m_chime))
        return m_chime;
    if (!m_dir)
        m_dir = std::make_unique<QTemporaryDir>();
    if (!m_dir->isValid())
        return {};
    const QString path = m_dir->filePath(QStringLiteral("chime.wav"));
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly))
        return {};
    file.write(chimeWave());
    file.close();
    m_chime = path;
    return m_chime;
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
