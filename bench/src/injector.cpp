#include <actcraft/bench/injector.h>
#include <actcraft/core/clock.h>

#include <QCoreApplication>
#include <QTabletEvent>
#include <QWheelEvent>
#include <QWindow>

namespace actcraft::bench {

Injector::Injector(QWindow* target, PageToScene pageToScene, QObject* parent)
    : QObject(parent), m_target(target), m_pageToScene(std::move(pageToScene))
{
    m_stylus = std::make_unique<QPointingDevice>(
        QStringLiteral("actcraft-bench-stylus"), 0x41430001, QInputDevice::DeviceType::Stylus,
        QPointingDevice::PointerType::Pen,
        QInputDevice::Capability::Position | QInputDevice::Capability::Pressure, 1, 3);
    m_wheel = std::make_unique<QPointingDevice>(
        QStringLiteral("actcraft-bench-wheel"), 0x41430002, QInputDevice::DeviceType::Mouse,
        QPointingDevice::PointerType::Generic,
        QInputDevice::Capability::Position | QInputDevice::Capability::Scroll, 1, 3);
    // Coup unique réarmé sur l'échéance suivante : pas de cadence fixe qui prendrait du retard.
    m_timer.setSingleShot(true);
    m_timer.setTimerType(Qt::PreciseTimer);
    connect(&m_timer, &QTimer::timeout, this, [this] { tick(core::monotonicNowNs()); });
}

Injector::~Injector()
{
    m_timer.stop();
}

const QPointingDevice* Injector::stylus() const noexcept
{
    return m_stylus.get();
}

const QPointingDevice* Injector::wheel() const noexcept
{
    return m_wheel.get();
}

void Injector::start(std::vector<PlannedInput> inputs, std::int64_t t0Ns, bool autoTick)
{
    m_timer.stop();
    m_inputs = std::move(inputs);
    m_planned.resize(m_inputs.size());
    for (std::size_t i = 0; i < m_inputs.size(); ++i) {
        m_planned[i] = t0Ns + m_inputs[i].offsetNs;
    }
    m_posted.assign(m_inputs.size(), -1);
    m_next = 0;
    m_autoTick = autoTick;
    m_finishedSignalled = false;
    if (m_autoTick) {
        tick(core::monotonicNowNs());
    }
}

std::size_t Injector::tick(std::int64_t nowNs)
{
    std::size_t count = 0;
    // Rattrapage : tout l'arriéré part maintenant, chaque échantillon garde son instant prévu (m_planned).
    while (m_next < m_inputs.size() && m_planned[m_next] <= nowNs) {
        QCoreApplication::postEvent(m_target, makeEvent(m_inputs[m_next]));
        m_posted[m_next] = core::monotonicNowNs();
        ++m_next;
        ++count;
    }
    if (finished()) {
        m_timer.stop();
        if (!m_finishedSignalled) {
            m_finishedSignalled = true;
            emit planFinished();
        }
    } else if (m_autoTick) {
        scheduleNext();
    }
    return count;
}

bool Injector::finished() const noexcept
{
    return m_next >= m_inputs.size();
}

const std::vector<PlannedInput>& Injector::inputs() const noexcept
{
    return m_inputs;
}

const std::vector<std::int64_t>& Injector::plannedNs() const noexcept
{
    return m_planned;
}

const std::vector<std::int64_t>& Injector::postedNs() const noexcept
{
    return m_posted;
}

void Injector::scheduleNext()
{
    const std::int64_t waitNs = m_planned[m_next] - core::monotonicNowNs();
    // Arrondi au-dessus : jamais de réveil en avance suivi d'une attente active. Un réveil un peu tardif
    // reste visible dans le retard d'injection (posté − prévu) et l'arriéré part au réveil suivant.
    const int waitMs = waitNs <= 0 ? 0 : static_cast<int>((waitNs + 999'999) / 1'000'000);
    m_timer.start(waitMs);
}

QEvent* Injector::makeEvent(const PlannedInput& input) const
{
    if (input.kind == InputKind::Wheel) {
        const QPointF center(m_target->width() / 2.0, m_target->height() / 2.0);
        return new QWheelEvent(center, m_target->mapToGlobal(center), QPoint(), input.angleDelta,
                               Qt::NoButton, input.modifiers, Qt::NoScrollPhase, false,
                               Qt::MouseEventNotSynthesized, m_wheel.get());
    }
    const QPointF scene = m_pageToScene(input.pagePos);
    QEvent::Type type = QEvent::TabletMove;
    Qt::MouseButton button = Qt::NoButton;
    Qt::MouseButtons buttons = Qt::LeftButton;
    if (input.kind == InputKind::TabletPress) {
        type = QEvent::TabletPress;
        button = Qt::LeftButton;
    } else if (input.kind == InputKind::TabletRelease) {
        type = QEvent::TabletRelease;
        button = Qt::LeftButton;
        buttons = Qt::NoButton;
    }
    return new QTabletEvent(type, m_stylus.get(), scene, m_target->mapToGlobal(scene), input.pressure, 0, 0,
                            0, 0, 0, input.modifiers, button, buttons);
}

} // namespace actcraft::bench
