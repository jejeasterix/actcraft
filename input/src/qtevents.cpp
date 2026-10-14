#include <actcraft/input/qtevents.h>

#include <QMouseEvent>
#include <QPointingDevice>
#include <QTabletEvent>
#include <QTouchEvent>

namespace actcraft::input {

std::optional<Sample> sampleFromEvent(const QEvent& event, QPointF pagePos, std::int64_t nowNs)
{
    Sample sample;
    sample.x = pagePos.x();
    sample.y = pagePos.y();
    sample.timestampNs = nowNs;
    switch (event.type()) {
    case QEvent::TabletPress:
    case QEvent::TabletMove:
    case QEvent::TabletRelease: {
        const auto& tablet = static_cast<const QTabletEvent&>(event);
        if (event.type() == QEvent::TabletMove && !(tablet.buttons() & Qt::LeftButton)) {
            return std::nullopt; // survol
        }
        sample.device = DeviceKind::Stylus;
        sample.phase = event.type() == QEvent::TabletPress  ? Phase::Press
                       : event.type() == QEvent::TabletMove ? Phase::Move
                                                            : Phase::Release;
        sample.pressure = static_cast<float>(tablet.pressure());
        return sample;
    }
    case QEvent::MouseButtonPress:
    case QEvent::MouseMove:
    case QEvent::MouseButtonRelease: {
        const auto& mouse = static_cast<const QMouseEvent&>(event);
        if (const QPointingDevice* device = mouse.pointingDevice();
            device && device->type() != QInputDevice::DeviceType::Mouse &&
            device->type() != QInputDevice::DeviceType::TouchPad) {
            return std::nullopt; // souris synthétisée depuis un stylet ou un écran tactile
        }
        if (event.type() == QEvent::MouseMove) {
            if (!(mouse.buttons() & Qt::LeftButton)) {
                return std::nullopt;
            }
            sample.phase = Phase::Move;
        } else {
            if (mouse.button() != Qt::LeftButton) {
                return std::nullopt;
            }
            sample.phase = event.type() == QEvent::MouseButtonPress ? Phase::Press : Phase::Release;
        }
        sample.device = DeviceKind::Mouse;
        sample.pressure = 0.5f;
        return sample;
    }
    case QEvent::TouchBegin:
    case QEvent::TouchUpdate:
    case QEvent::TouchEnd: {
        const auto& touch = static_cast<const QTouchEvent&>(event);
        if (touch.pointCount() != 1) {
            return std::nullopt; // deux doigts et plus : gestes (lot 3), jamais un trait
        }
        sample.device = DeviceKind::Touch;
        sample.phase = event.type() == QEvent::TouchBegin    ? Phase::Press
                       : event.type() == QEvent::TouchUpdate ? Phase::Move
                                                             : Phase::Release;
        sample.pressure = 0.5f; // comme la souris : épaisseur nominale
        return sample;
    }
    default:
        return std::nullopt;
    }
}

} // namespace actcraft::input
