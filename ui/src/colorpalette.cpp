#include "actcraft/ui/colorpalette.h"

#include "actcraft/ui/profilecontroller.h"

#include <QQuickItem>
#include <QQuickWindow>
#include <QtMath>

#include <algorithm>

namespace actcraft::ui {

namespace {

constexpr int kHues = 6;
constexpr int kLightnesses = 6;

bool containsColor(const QStringList& list, const QString& color)
{
    return list.contains(color, Qt::CaseInsensitive);
}

} // namespace

ColorPalette::ColorPalette(QObject* parent) : QObject(parent) {}

QStringList ColorPalette::extendedGrid()
{
    QStringList out;
    for (int row = 0; row < kLightnesses; ++row) {
        for (int column = 0; column < kHues; ++column) {
            // Teintes 0, 60 … 300 ; clartés 0,15 à 0,90 par pas de 0,15.
            const float hue = static_cast<float>(column) / 6.0f;
            const float lightness = 0.15f * static_cast<float>(row + 1);
            out << QColor::fromHslF(hue, 1.0f, lightness).name();
        }
    }
    return out;
}

void ColorPalette::setProfile(ProfileController* profile)
{
    if (profile == m_profile) {
        return;
    }
    if (m_profile) {
        disconnect(m_profile, nullptr, this, nullptr);
    }
    m_profile = profile;
    if (m_profile) {
        connect(m_profile, &ProfileController::profileChanged, this, &ColorPalette::changed);
        connect(m_profile, &QObject::destroyed, this, [this] {
            m_profile = nullptr;
            emit changed();
        });
    }
    emit changed();
}

QStringList ColorPalette::swatches() const
{
    const int count = m_profile ? m_profile->swatches() : kDefaultSwatches;
    QStringList extras;
    if (m_profile) {
        for (const QString& color : m_profile->extraColors()) {
            if (!containsColor(m_base, color) && !containsColor(extras, color)) {
                extras << color;
            }
        }
    }
    const int keptExtras = std::min<int>(count, static_cast<int>(extras.size()));
    QStringList out = m_base.mid(0, count - keptExtras);
    out << extras.mid(extras.size() - keptExtras);
    return out;
}

QStringList ColorPalette::extended() const
{
    return extendedGrid();
}

int ColorPalette::extraCount() const
{
    return m_profile ? static_cast<int>(m_profile->extraColors().size()) : 0;
}

void ColorPalette::add(const QString& color)
{
    const QColor parsed(color);
    if (!m_profile || !parsed.isValid() || containsColor(m_base, parsed.name())) {
        return;
    }
    m_profile->addColor(parsed.name()); // le profil ignore les doublons (casse comprise)
}

void ColorPalette::setSnapshot(QImage image)
{
    m_snapshot = std::move(image);
    ++m_snapshots;
}

void ColorPalette::beginPick()
{
    emit snapshotRequested();
}

void ColorPalette::grabFrom(QQuickItem* item)
{
    if (item && item->window()) {
        setSnapshot(item->window()->grabWindow());
    }
}

QString ColorPalette::colorAt(int x, int y) const
{
    if (m_snapshot.isNull()) {
        return {};
    }
    const qreal ratio = m_snapshot.devicePixelRatio();
    const int px = qFloor(x * ratio);
    const int py = qFloor(y * ratio);
    if (x < 0 || y < 0 || px >= m_snapshot.width() || py >= m_snapshot.height()) {
        return {};
    }
    return m_snapshot.pixelColor(px, py).name();
}

} // namespace actcraft::ui
