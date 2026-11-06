#include "actcraft/media/player.h"

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
                        current.loaded = true;
                        break;
                    case QMediaPlayer::InvalidMedia:
                        fallBack(track);
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
                        fallBack(track);
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
    channel.path.clear(); // les signaux du son remplacé ne comptent plus
    channel.loaded = false;
    channel.player->stop();
    if (!QFileInfo(path).isFile()) {
        channel.player->setSource(QUrl());
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
        channel.path.clear(); // un repli encore en attente ne part plus
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

// Un fichier que Qt Multimedia ne sait pas lire est confié une seule fois au rappel posé par ui/, qui
// demande confirmation avant toute ouverture : media/ n'ouvre jamais rien lui-même.
void Player::fallBack(int track)
{
    Channel& channel = m_channels[static_cast<std::size_t>(track)];
    if (channel.path.isEmpty() || channel.loaded)
        return;
    const QString path = channel.path;
    channel.path.clear(); // l'erreur et l'état « illisible » arrivent tous deux : un seul repli
    emit fellBack(path);
    if (m_fallback)
        m_fallback(path);
}

} // namespace actcraft::media
