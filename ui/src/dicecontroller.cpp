#include "dicecontroller.h"

#include "presentationdocument.h"

#include <QRectF>
#include <QSettings>
#include <QString>

#include <numeric>

namespace actcraft::ui {

namespace {

constexpr int kTickMs = 100;
const char* const kCountKey = "tools/dice/count";
const char* const kSpeedKey = "tools/dice/speed";

int clampedSpeed(int seconds) noexcept
{
    return features::Dice::animationMs(seconds) / 1000;
}

void store(const char* key, int value)
{
    QSettings settings;
    settings.setValue(QString::fromLatin1(key), value);
}

} // namespace

DiceController::DiceController(QObject* parent)
    : QObject(parent), m_seed(QRandomGenerator::global()->generate()), m_dice(m_seed),
      m_noise(QRandomGenerator::global()->generate())
{
    QSettings settings;
    m_dice.setCount(settings.value(QString::fromLatin1(kCountKey), features::Dice::kDefaultCount).toInt());
    m_count = m_dice.count();
    m_speed =
        clampedSpeed(settings.value(QString::fromLatin1(kSpeedKey), features::Dice::kDefaultSeconds).toInt());
    m_ticker.setInterval(kTickMs);
    connect(&m_ticker, &QTimer::timeout, this, &DiceController::tick);
}

int DiceController::count() const noexcept
{
    return m_count;
}

void DiceController::setCount(int count)
{
    m_dice.setCount(count);
    if (m_dice.count() == m_count) {
        return;
    }
    m_count = m_dice.count();
    store(kCountKey, m_count);
    emit countChanged();
}

int DiceController::speed() const noexcept
{
    return m_speed;
}

void DiceController::setSpeed(int seconds)
{
    const int clamped = clampedSpeed(seconds);
    if (clamped == m_speed) {
        return;
    }
    m_speed = clamped;
    store(kSpeedKey, m_speed);
    emit speedChanged();
}

int DiceController::minCount() noexcept
{
    return features::Dice::kMinCount;
}

int DiceController::maxCount() noexcept
{
    return features::Dice::kMaxCount;
}

int DiceController::minSpeed() noexcept
{
    return clampedSpeed(0);
}

int DiceController::maxSpeed() noexcept
{
    return clampedSpeed(1000);
}

QVariantList DiceController::faces() const
{
    QVariantList list;
    for (int face : m_shown) {
        list.append(face);
    }
    return list;
}

int DiceController::total() const noexcept
{
    return m_rolling ? 0 : std::accumulate(m_final.begin(), m_final.end(), 0);
}

bool DiceController::rolling() const noexcept
{
    return m_rolling;
}

bool DiceController::canWrite() const noexcept
{
    return !m_rolling && !m_final.empty() && !m_document.isNull();
}

quint32 DiceController::seed() const noexcept
{
    return m_seed;
}

void DiceController::setSeed(quint32 seed)
{
    if (seed == m_seed) {
        return;
    }
    m_seed = seed;
    m_dice = features::Dice(seed);
    m_dice.setCount(m_count);
    emit seedChanged();
}

DocumentController* DiceController::documentController() const noexcept
{
    return m_document;
}

void DiceController::setDocumentController(DocumentController* controller)
{
    if (controller == m_document) {
        return;
    }
    m_document = controller;
    emit documentControllerChanged();
    refreshCanWrite();
}

void DiceController::roll()
{
    if (m_rolling) {
        return;
    }
    m_dice.setCount(m_count);
    m_final = m_dice.roll();
    m_remainingMs = features::Dice::animationMs(m_speed);
    m_rolling = true;
    shuffle();
    m_ticker.start();
    emit rollingChanged();
    emit facesChanged();
    refreshCanWrite();
}

void DiceController::skip()
{
    if (m_rolling) {
        finish();
    }
}

bool DiceController::writeTotal()
{
    if (!canWrite()) {
        return false;
    }
    DocumentInsertions insertions(*m_document);
    const core::Page* page = insertions.shownPage();
    if (!page) {
        return false;
    }
    const QRectF bounds(page->width / 2 - 40, page->height / 2 - 20, 80, 40);
    return insertions.insertText(QString::number(total()), bounds) != 0;
}

void DiceController::tick()
{
    m_remainingMs -= kTickMs;
    if (m_remainingMs <= 0) {
        finish();
        return;
    }
    shuffle();
    emit facesChanged();
}

void DiceController::finish()
{
    m_ticker.stop();
    m_shown = m_final;
    m_rolling = false;
    emit rollingChanged();
    emit facesChanged();
    refreshCanWrite();
}

void DiceController::shuffle()
{
    m_shown.assign(m_final.size(), 1);
    for (int& face : m_shown) {
        face = static_cast<int>(m_noise.bounded(1, 7));
    }
}

void DiceController::refreshCanWrite()
{
    if (canWrite() != m_canWrite) {
        m_canWrite = canWrite();
        emit canWriteChanged();
    }
}

} // namespace actcraft::ui
