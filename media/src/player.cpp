#include "actcraft/media/player.h"

namespace actcraft::media {

Player::Player(QObject* parent) : QObject(parent) {}

Player::~Player() = default;

void Player::setFallback(std::function<void(const QString& path)> fallback)
{
    m_fallback = std::move(fallback);
}

void Player::play(const QString&, Track, bool) {}

void Player::pause(Track) {}

void Player::stopAll() {}

qint64 Player::position(Track) const
{
    return 0;
}

bool Player::isPlaying(Track) const
{
    return false;
}

void Player::fallBack(int) {}

} // namespace actcraft::media
