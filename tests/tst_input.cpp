#include <actcraft/core/inkprobe.h>
#include <actcraft/input/pipeline.h>
#include <actcraft/input/qtevents.h>

#include <QMouseEvent>
#include <QPointingDevice>
#include <QTabletEvent>
#include <QTest>
#include <QTouchEvent>

#include <utility>
#include <vector>

using namespace actcraft;
using input::Phase;

namespace {

class RecordingSink : public input::SampleSink
{
public:
    std::vector<input::Sample> samples;
    void onSample(const input::Sample& sample) override { samples.push_back(sample); }
};

class RecordingProbe : public core::InkProbe
{
public:
    std::vector<std::pair<std::uint64_t, std::int64_t>> received;
    void sampleReceived(std::uint64_t seq, std::int64_t nowNs) override { received.emplace_back(seq, nowNs); }
    void samplesSynced(std::uint64_t, std::int64_t) override {}
};

input::Sample make(Phase phase, double x, double y, float pressure = 0.5f, std::int64_t t = 0)
{
    input::Sample s;
    s.phase = phase;
    s.x = x;
    s.y = y;
    s.pressure = pressure;
    s.timestampNs = t;
    s.device = input::DeviceKind::Stylus;
    return s;
}

} // namespace

class TstInput : public QObject
{
    Q_OBJECT

private slots:
    void sequenceNumbersIncreaseAndReset()
    {
        RecordingSink sink;
        input::Pipeline pipeline(sink);
        QCOMPARE(pipeline.push(make(Phase::Press, 1, 1)), std::uint64_t{0});
        QCOMPARE(pipeline.push(make(Phase::Move, 2, 2)), std::uint64_t{1});
        QCOMPARE(pipeline.nextSequence(), std::uint64_t{2});
        pipeline.resetSequence(10);
        QCOMPARE(pipeline.push(make(Phase::Move, 3, 3)), std::uint64_t{10});
        QCOMPARE(sink.samples.back().seq, std::uint64_t{10});
    }

    void duplicateMoveFilteredButSequenceConsumed()
    {
        RecordingSink sink;
        input::Pipeline pipeline(sink);
        pipeline.push(make(Phase::Press, 1, 1));
        pipeline.push(make(Phase::Move, 1, 1));
        pipeline.push(make(Phase::Move, 2, 2));
        QCOMPARE(sink.samples.size(), std::size_t{2});
        QCOMPARE(sink.samples[0].seq, std::uint64_t{0});
        QCOMPARE(sink.samples[1].seq, std::uint64_t{2});
    }

    void releaseIsNeverFiltered()
    {
        RecordingSink sink;
        input::Pipeline pipeline(sink);
        pipeline.push(make(Phase::Press, 5, 5));
        pipeline.push(make(Phase::Move, 6, 6));
        pipeline.push(make(Phase::Release, 6, 6));
        QCOMPARE(sink.samples.size(), std::size_t{3});
        QCOMPARE(sink.samples.back().phase, Phase::Release);
    }

    void probeSeesEveryReceivedSample()
    {
        RecordingSink sink;
        RecordingProbe probe;
        input::Pipeline pipeline(sink);
        pipeline.setProbe(&probe);
        pipeline.push(make(Phase::Press, 1, 1, 0.5f, 100));
        pipeline.push(make(Phase::Move, 1, 1, 0.5f, 200)); // filtré, mais reçu
        pipeline.push(make(Phase::Move, 2, 2, 0.5f, 300));
        QCOMPARE(probe.received.size(), std::size_t{3});
        QCOMPARE(probe.received[1].first, std::uint64_t{1});
        QCOMPARE(probe.received[1].second, std::int64_t{200});
    }

    void predictorIsIdentity()
    {
        const input::IdentityPredictor predictor;
        const input::Sample s = make(Phase::Move, 3.5, 4.25, 0.7f, 42);
        const input::Sample p = predictor.predict(s);
        QCOMPARE(p.x, s.x);
        QCOMPARE(p.y, s.y);
        QCOMPARE(p.pressure, s.pressure);
        QCOMPARE(p.timestampNs, s.timestampNs);
    }

    void tabletEventsConvert()
    {
        QPointingDevice stylus(QStringLiteral("test-stylus"), 77, QInputDevice::DeviceType::Stylus,
                               QPointingDevice::PointerType::Pen,
                               QInputDevice::Capability::Position | QInputDevice::Capability::Pressure, 1, 3);
        const QTabletEvent press(QEvent::TabletPress, &stylus, QPointF(5, 5), QPointF(5, 5), 0.7, 0, 0, 0, 0,
                                 0, Qt::NoModifier, Qt::LeftButton, Qt::LeftButton);
        const auto s = input::sampleFromEvent(press, QPointF(100, 200), 123);
        QVERIFY(s.has_value());
        QCOMPARE(s->phase, Phase::Press);
        QCOMPARE(s->device, input::DeviceKind::Stylus);
        QVERIFY(qAbs(s->pressure - 0.7f) < 0.001f);
        QCOMPARE(s->x, 100.0);
        QCOMPARE(s->y, 200.0);
        QCOMPARE(s->timestampNs, std::int64_t{123});

        const QTabletEvent move(QEvent::TabletMove, &stylus, QPointF(6, 6), QPointF(6, 6), 0.4, 0, 0, 0, 0, 0,
                                Qt::NoModifier, Qt::NoButton, Qt::LeftButton);
        QCOMPARE(input::sampleFromEvent(move, QPointF(1, 1), 0)->phase, Phase::Move);

        const QTabletEvent hover(QEvent::TabletMove, &stylus, QPointF(6, 6), QPointF(6, 6), 0.0, 0, 0, 0, 0,
                                 0, Qt::NoModifier, Qt::NoButton, Qt::NoButton);
        QVERIFY(!input::sampleFromEvent(hover, QPointF(1, 1), 0).has_value());

        const QTabletEvent release(QEvent::TabletRelease, &stylus, QPointF(6, 6), QPointF(6, 6), 0.0, 0, 0, 0,
                                   0, 0, Qt::NoModifier, Qt::LeftButton, Qt::NoButton);
        QCOMPARE(input::sampleFromEvent(release, QPointF(1, 1), 0)->phase, Phase::Release);
    }

    void mouseLeftButtonConverts()
    {
        const QMouseEvent press(QEvent::MouseButtonPress, QPointF(5, 5), QPointF(5, 5), QPointF(5, 5),
                                Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
        const auto s = input::sampleFromEvent(press, QPointF(10, 20), 0);
        QVERIFY(s.has_value());
        QCOMPARE(s->device, input::DeviceKind::Mouse);
        QCOMPARE(s->pressure, 0.5f);
        const QMouseEvent right(QEvent::MouseButtonPress, QPointF(5, 5), QPointF(5, 5), QPointF(5, 5),
                                Qt::RightButton, Qt::RightButton, Qt::NoModifier);
        QVERIFY(!input::sampleFromEvent(right, QPointF(10, 20), 0).has_value());
        const QMouseEvent hover(QEvent::MouseMove, QPointF(5, 5), QPointF(5, 5), QPointF(5, 5), Qt::NoButton,
                                Qt::NoButton, Qt::NoModifier);
        QVERIFY(!input::sampleFromEvent(hover, QPointF(10, 20), 0).has_value());
    }

    void singleTouchPointBecomesTouchSamples()
    {
        QPointingDevice screen(QStringLiteral("tst-touch"), 7701, QInputDevice::DeviceType::TouchScreen,
                               QPointingDevice::PointerType::Finger, QInputDevice::Capability::Position, 10,
                               0);
        QTouchEvent begin(QEvent::TouchBegin, &screen, Qt::NoModifier,
                          {QEventPoint(1, QEventPoint::State::Pressed, QPointF(10, 20), QPointF(10, 20))});
        const auto press = input::sampleFromEvent(begin, QPointF(5, 6), 42);
        QVERIFY(press.has_value());
        QVERIFY(press->device == input::DeviceKind::Touch);
        QVERIFY(press->phase == Phase::Press);
        QCOMPARE(press->x, 5.0);
        QCOMPARE(press->y, 6.0);
        QCOMPARE(press->pressure, 0.5f);
        QCOMPARE(press->timestampNs, std::int64_t{42});

        QTouchEvent update(QEvent::TouchUpdate, &screen, Qt::NoModifier,
                           {QEventPoint(1, QEventPoint::State::Updated, QPointF(30, 20), QPointF(30, 20))});
        const auto move = input::sampleFromEvent(update, QPointF(7, 6), 43);
        QVERIFY(move.has_value() && move->phase == Phase::Move);

        QTouchEvent end(QEvent::TouchEnd, &screen, Qt::NoModifier,
                        {QEventPoint(1, QEventPoint::State::Released, QPointF(30, 20), QPointF(30, 20))});
        const auto release = input::sampleFromEvent(end, QPointF(7, 6), 44);
        QVERIFY(release.has_value() && release->phase == Phase::Release);
    }

    void twoFingersAndCancelGiveNothing()
    {
        QPointingDevice screen(QStringLiteral("tst-touch-2"), 7702, QInputDevice::DeviceType::TouchScreen,
                               QPointingDevice::PointerType::Finger, QInputDevice::Capability::Position, 10,
                               0);
        QTouchEvent two(QEvent::TouchUpdate, &screen, Qt::NoModifier,
                        {QEventPoint(1, QEventPoint::State::Updated, QPointF(10, 20), QPointF(10, 20)),
                         QEventPoint(2, QEventPoint::State::Pressed, QPointF(90, 20), QPointF(90, 20))});
        QVERIFY(!input::sampleFromEvent(two, QPointF(5, 6), 1).has_value());
        QTouchEvent cancel(QEvent::TouchCancel, &screen, Qt::NoModifier, QList<QEventPoint>{});
        QVERIFY(!input::sampleFromEvent(cancel, QPointF(5, 6), 2).has_value());
    }

    void otherEventsIgnored()
    {
        const QEvent focus(QEvent::FocusIn);
        QVERIFY(!input::sampleFromEvent(focus, QPointF(0, 0), 0).has_value());
    }
};

QTEST_MAIN(TstInput)
#include "tst_input.moc"
