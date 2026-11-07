#include "actcraft/media/player.h"

#include <actcraft/core/networkpath.h>

#include <QAudioOutput>
#include <QFileInfo>
#include <QMediaPlayer>
#include <QUrl>

namespace actcraft::media {

Player::Player(QObject* parent) : QObject(parent)
{
    for (int track = First; track <= Second; ++track) {
        Channel& channel = m_channels[static_cast<std::size_t>(track)];
        channel.output = new QAudioOutput(this);
        channel.player = new QMediaPlayer(this);
        channel.player->setAudioOutput(channel.output);
        // Signaux de Qt Multimedia reçus sur le fil du lecteur : rien n'est attendu, tout arrive ici.
        connect(channel.player, &QMediaPlayer::positionChanged, this, [this, track](qint64 position) {
            if (!m_channels[static_cast<std::size_t>(track)].path.isEmpty())
                emit positionChanged(track, position);
        });
        connect(channel.player, &QMediaPlayer::mediaStatusChanged, this,
                [this, track](QMediaPlayer::MediaStatus status) {
                    Channel& current = m_channels[static_cast<std::size_t>(track)];
                    switch (status) {
                    case QMediaPlayer::LoadedMedia:
                    case QMediaPlayer::BufferingMedia:
                    case QMediaPlayer::BufferedMedia:
                        // stop() du son remplacé émet LoadedMedia sur-le-champ, chemin déjà effacé : il ne
                        // marque pas « chargé » le fichier qui le suit.
                        if (!current.path.isEmpty())
                            current.loaded = true;
                        break;
                    case QMediaPlayer::InvalidMedia:
                        settleLater(track);
                        break;
                    case QMediaPlayer::EndOfMedia:
                        if (!current.path.isEmpty())
                            emit finished(track);
                        break;
                    default:
                        break;
                    }
                });
        connect(channel.player, &QMediaPlayer::errorOccurred, this,
                [this, track](QMediaPlayer::Error error, const QString&) {
                    if (error != QMediaPlayer::NoError)
                        settleLater(track);
                });
    }
}

Player::~Player() = default;

void Player::setFallback(std::function<void(const QString& path)> fallback)
{
    m_fallback = std::move(fallback);
}

void Player::play(const QString& path, Track track, bool loop)
{
    if (track != First && track != Second)
        return;
    Channel& channel = m_channels[static_cast<std::size_t>(track)];
    ++channel.generation;
    channel.path.clear(); // les signaux du son remplacé ne comptent plus
    channel.loaded = false;
    channel.player->stop();
    channel.player->setSource(
        QUrl()); // setSource sort tôt sur une adresse identique : le même fichier se recharge
    if (core::isNetworkPath(path)) {
        failLater(track, tr("Un chemin réseau ne se lit pas depuis un paperboard : %1").arg(path));
        return;
    }
    const QFileInfo info(path);
    if (!info.isFile()) {
        failLater(track, tr("Fichier introuvable : %1").arg(path));
        return;
    }
    if (!info.isReadable()) {
        failLater(track, tr("Lecture du fichier refusée : %1").arg(path));
        return;
    }
    channel.player->setLoops(loop ? QMediaPlayer::Infinite : QMediaPlayer::Once);
    channel.path = path; // avant setSource : un moteur peut déclarer le fichier illisible sur-le-champ
    channel.player->setSource(QUrl::fromLocalFile(path));
    channel.player->play(); // rend la main : le chargement se poursuit sur les fils de Qt Multimedia
}

void Player::pause(Track track)
{
    if (track != First && track != Second)
        return;
    m_channels[static_cast<std::size_t>(track)].player->pause();
}

void Player::stopAll()
{
    for (Channel& channel : m_channels) {
        ++channel.generation; // un repli ou une erreur encore en attente ne part plus
        channel.path.clear();
        channel.loaded = false;
        channel.player->stop();
        channel.player->setSource(QUrl());
    }
}

qint64 Player::position(Track track) const
{
    if (track != First && track != Second)
        return 0;
    const Channel& channel = m_channels[static_cast<std::size_t>(track)];
    return channel.path.isEmpty() ? 0 : channel.player->position();
}

bool Player::isPlaying(Track track) const
{
    if (track != First && track != Second)
        return false;
    const Channel& channel = m_channels[static_cast<std::size_t>(track)];
    return !channel.path.isEmpty() && channel.player->playbackState() == QMediaPlayer::PlayingState;
}

// Le moteur darwin émet InvalidMedia, puis ResourceError (« Failed to load media ») pour un fichier qu'il ne
// sait pas lire ; d'autres moteurs, FormatError. L'un et l'autre signal attendent donc le tour suivant de la
// boucle, où l'état et l'erreur sont connus tous deux.
void Player::settleLater(int track)
{
    const quint64 generation = m_channels[static_cast<std::size_t>(track)].generation;
    QMetaObject::invokeMethod(
        this,
        [this, track, generation] {
            if (m_channels[static_cast<std::size_t>(track)].generation == generation)
                settle(track);
        },
        Qt::QueuedConnection);
}

// Chargement en échec : seul un format illisible (InvalidMedia, FormatError) part au système ; accès refusé,
// réseau, ou ressource en échec sans « illisible » : erreur signalée sans repli. Une erreur après chargement
// ne bascule pas.
void Player::settle(int track)
{
    const Channel& channel = m_channels[static_cast<std::size_t>(track)];
    if (channel.path.isEmpty() || channel.loaded)
        return;
    const QMediaPlayer::Error error = channel.player->error();
    const bool refused = error == QMediaPlayer::AccessDeniedError || error == QMediaPlayer::NetworkError;
    if (!refused &&
        (channel.player->mediaStatus() == QMediaPlayer::InvalidMedia || error == QMediaPlayer::FormatError))
        fallBack(track);
    else
        fail(track, channel.player->errorString());
}

// Un fichier illisible est confié une seule fois au rappel posé par ui/, qui demande confirmation avant toute
// ouverture : media/ n'ouvre jamais rien lui-même.
void Player::fallBack(int track)
{
    Channel& channel = m_channels[static_cast<std::size_t>(track)];
    const QString path = channel.path;
    channel.path.clear(); // l'erreur et l'état « illisible » arrivent tous deux : un seul repli
    emit fellBack(path);
    if (m_fallback)
        m_fallback(path);
}

void Player::fail(int track, const QString& reason)
{
    m_channels[static_cast<std::size_t>(track)].path.clear();
    emit failed(track, reason);
}

// Erreur connue avant tout chargement : signalée au tour suivant de la boucle, jamais avant le retour de
// play(), et abandonnée si la piste a changé entre-temps.
void Player::failLater(int track, const QString& reason)
{
    const quint64 generation = m_channels[static_cast<std::size_t>(track)].generation;
    QMetaObject::invokeMethod(
        this,
        [this, track, generation, reason] {
            if (m_channels[static_cast<std::size_t>(track)].generation == generation)
                emit failed(track, reason);
        },
        Qt::QueuedConnection);
}

} // namespace actcraft::media
