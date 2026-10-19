#include <actcraft/bench/injector.h>
#include <actcraft/core/clock.h>

#include <QSignalSpy>
#include <QTabletEvent>
#include <QTest>
#include <QTimer>
#include <QWheelEvent>
#include <QWindow>

#include <algorithm>
#include <chrono>
#include <thread>

using namespace actcraft;
using bench::InputKind;

namespace {

// Jamais affichée : les événements postés n'ont besoin ni d'écran ni de premier plan.
class RecordingWindow : public QWindow
{
public:
    struct Received
    {
        QEvent::Type type;
        qreal pressure = 0;
        QPointF position;
        const QInputDevice* device = nullptr;
        QPoint angleDelta;
        Qt::KeyboardModifiers modifiers;
        Qt::MouseButton button = Qt::NoButton;
        Qt::MouseButtons buttons;
    };
    std::vector<Received> events;

protected:
    bool event(QEvent* e) override
    {
        switch (e->type()) {
        case QEvent::TabletPress:
        case QEvent::TabletMove:
        case QEvent::TabletRelease: {
            auto* t = static_cast<QTabletEvent*>(e);
            events.push_back({e->type(),
                              t->pressure(),
                              t->position(),
                              t->pointingDevice(),
                              {},
                              t->modifiers(),
                              t->button(),
                              t->buttons()});
            return true;
        }
        case QEvent::Wheel: {
            auto* w = static_cast<QWheelEvent*>(e);
            events.push_back(
                {e->type(), 0, w->position(), w->pointingDevice(), w->angleDelta(), w->modifiers(), {}, {}});
            return true;
        }
        default:
            return QWindow::event(e);
        }
    }
};

std::vector<bench::PlannedInput> strokePlan(int count)
{
    std::vector<bench::PlannedInput> plan;
    for (int i = 0; i < count; ++i) {
        bench::PlannedInput in;
        in.offsetNs = i * bench::kStylusPeriodNs;
        in.kind = i == 0           ? InputKind::TabletPress
                  : i == count - 1 ? InputKind::TabletRelease
                                   : InputKind::TabletMove;
        in.pagePos = QPointF(10 + i, 20);
        in.pressure = 0.5f;
        plan.push_back(in);
    }
    return plan;
}

} // namespace

class TstBenchInjector : public QObject
{
    Q_OBJECT

private slots:
    void catchUpPostsEveryDueEvent()
    {
        RecordingWindow window;
        bench::Injector injector(&window, [](QPointF p) { return p; });
        QSignalSpy finished(&injector, &bench::Injector::planFinished);
        const std::int64_t t0 = 1'000'000'000;
        injector.start(strokePlan(21), t0, false);
        QCOMPARE(injector.tick(t0), std::size_t{1});
        QCOMPARE(injector.tick(t0 + 50'000'000), std::size_t{10}); // 5 … 50 ms : tout l'arriéré d'un coup
        QCoreApplication::processEvents();
        QCOMPARE(window.events.size(), std::size_t{11});
        QCOMPARE(window.events[0].type, QEvent::TabletPress);
        QCOMPARE(injector.tick(t0 + 1'000'000'000), std::size_t{10});
        QVERIFY(injector.finished());
        QCOMPARE(finished.count(), 1);
        QCoreApplication::processEvents();
        QCOMPARE(window.events.back().type, QEvent::TabletRelease);
    }

    void nothingIsPostedBeforeItsPlannedInstant()
    {
        RecordingWindow window;
        bench::Injector injector(&window, [](QPointF p) { return p; });
        const std::int64_t t0 = 1'000'000'000;
        injector.start(strokePlan(3), t0, false);
        QCOMPARE(injector.tick(t0 - 1), std::size_t{0});
        QCOMPARE(injector.tick(t0), std::size_t{1});
        QCOMPARE(injector.tick(t0 + bench::kStylusPeriodNs - 1), std::size_t{0});
        QCOMPARE(injector.postedNs()[1], std::int64_t{-1});
        QVERIFY(!injector.finished());
        QCoreApplication::processEvents();
        QCOMPARE(window.events.size(), std::size_t{1});
    }

    // Le test qui distingue le rattrapage du minuteur de 5 ms à un événement par déclenchement : après 50 ms
    // de fil principal bloqué, ce dernier n'enverrait qu'un événement par réveil (rafale de 1) et garderait
    // son retard ; l'injecteur poste l'arriéré d'un coup, chaque échantillon gardant son instant prévu.
    void blockedMainThreadBacklogIsPostedAtOnce()
    {
        RecordingWindow window;
        bench::Injector injector(&window, [](QPointF p) { return p; });
        QSignalSpy finished(&injector, &bench::Injector::planFinished);
        const std::int64_t t0 = core::monotonicNowNs() + 10'000'000;
        injector.start(strokePlan(40), t0);
        QTimer::singleShot(20, [] { std::this_thread::sleep_for(std::chrono::milliseconds(50)); });
        QVERIFY(finished.wait(3000));
        const auto& planned = injector.plannedNs();
        const auto& posted = injector.postedNs();
        QCOMPARE(posted.size(), std::size_t{40});
        std::size_t longestBurst = 1;
        std::size_t burst = 1;
        for (std::size_t i = 0; i < posted.size(); ++i) {
            QCOMPARE(planned[i], t0 + static_cast<std::int64_t>(i) * bench::kStylusPeriodNs);
            QVERIFY(posted[i] >= planned[i]);
            if (i > 0) {
                burst = posted[i] - posted[i - 1] < 1'000'000 ? burst + 1 : 1;
                longestBurst = std::max(longestBurst, burst);
            }
        }
        QVERIFY2(longestBurst >= 9, qPrintable(QString::number(longestBurst)));
        QCoreApplication::processEvents();
        QCOMPARE(window.events.size(), std::size_t{40});
    }

    // Déterministe, sans boucle d'événements : un minuteur grossier ou armé à une durée fixe passerait les
    // tests de rattrapage (l'arriéré part quand même), mais décalerait chaque envoi de son instant prévu.
    void wakeTimerIsPreciseAndArmedOnTheNextDeadline()
    {
        RecordingWindow window;
        bench::Injector injector(&window, [](QPointF p) { return p; });
        injector.start(strokePlan(3), core::monotonicNowNs() + 10'000'000, true);
        QCOMPARE(injector.postedNs()[0], std::int64_t{-1});
        QVERIFY(injector.wakeTimer().isActive());
        QCOMPARE(injector.wakeTimer().timerType(), Qt::PreciseTimer);
        const int remaining = injector.wakeTimer().remainingTime();
        QVERIFY2(remaining >= 9 && remaining <= 11, qPrintable(QString::number(remaining)));
        injector.stop();
    }

    // Abandon (« injection incomplete », T16) : le minuteur ne doit plus poster d'événements stylet, et le
    // plan n'est pas déclaré fini.
    void stopLeavesThePlanUnfinishedAndSilent()
    {
        RecordingWindow window;
        bench::Injector injector(&window, [](QPointF p) { return p; });
        QSignalSpy finished(&injector, &bench::Injector::planFinished);
        injector.start(strokePlan(20), core::monotonicNowNs(), true); // le premier échantillon part aussitôt
        const auto sent = [&injector] {
            return static_cast<std::size_t>(std::count_if(injector.postedNs().begin(),
                                                          injector.postedNs().end(),
                                                          [](std::int64_t t) { return t >= 0; }));
        };
        const std::size_t beforeStop = sent();
        QVERIFY(beforeStop >= 1 && beforeStop < 20);
        injector.stop();
        QVERIFY(!injector.wakeTimer().isActive());
        QTest::qWait(150); // le plan entier tient en 95 ms
        QCOMPARE(injector.tick(core::monotonicNowNs()), std::size_t{0});
        QCOMPARE(sent(), beforeStop);
        QCOMPARE(window.events.size(), beforeStop);
        QVERIFY(!injector.finished());
        QCOMPARE(finished.count(), 0);
    }

    void wheelEventsCarryDeltaAndModifiers()
    {
        RecordingWindow window;
        window.resize(200, 100);
        bench::Injector injector(&window, [](QPointF p) { return p; });
        bench::PlannedInput wheel;
        wheel.kind = InputKind::Wheel;
        wheel.angleDelta = QPoint(0, 8);
        wheel.modifiers = Qt::ControlModifier;
        injector.start({wheel}, 0, false);
        injector.tick(0);
        QCoreApplication::processEvents();
        QCOMPARE(window.events.size(), std::size_t{1});
        QCOMPARE(window.events[0].angleDelta, QPoint(0, 8));
        QVERIFY(window.events[0].modifiers & Qt::ControlModifier);
        QCOMPARE(window.events[0].device, injector.wheel());
        QCOMPARE(window.events[0].position, QPointF(100, 50)); // centre de la fenêtre
    }

    void tabletEventsUseTheSyntheticStylusAndPageToScene()
    {
        RecordingWindow window;
        bench::Injector injector(&window, [](QPointF p) { return p * 2; });
        bench::PlannedInput press;
        press.kind = InputKind::TabletPress;
        press.pagePos = QPointF(10, 20);
        press.pressure = 0.7f;
        injector.start({press}, 0, false);
        injector.tick(0);
        QCoreApplication::processEvents();
        QCOMPARE(window.events[0].device, injector.stylus());
        QCOMPARE(window.events[0].position, QPointF(20, 40));
        QVERIFY(qAbs(window.events[0].pressure - 0.7) < 0.001);
    }

    // Boutons du stylet posé, comme le chemin vérifié de T2 (tst_qt_verifications) : un déplacement sans
    // Qt::LeftButton dans buttons() est un survol que input::sampleFromEvent écarte.
    void tabletButtonsFollowThePenContact()
    {
        RecordingWindow window;
        bench::Injector injector(&window, [](QPointF p) { return p; });
        injector.start(strokePlan(3), 0, false);
        for (std::int64_t i = 0; i < 3; ++i) {
            QCOMPARE(injector.tick(i * bench::kStylusPeriodNs), std::size_t{1});
        }
        QCoreApplication::processEvents();
        QCOMPARE(window.events.size(), std::size_t{3});
        QCOMPARE(window.events[0].button, Qt::LeftButton);
        QCOMPARE(window.events[0].buttons, Qt::MouseButtons(Qt::LeftButton));
        QCOMPARE(window.events[1].type, QEvent::TabletMove);
        QCOMPARE(window.events[1].button, Qt::NoButton);
        QCOMPARE(window.events[1].buttons, Qt::MouseButtons(Qt::LeftButton));
        QCOMPARE(window.events[2].button, Qt::LeftButton);
        QCOMPARE(window.events[2].buttons, Qt::MouseButtons(Qt::NoButton));
    }

    // Pincement (L0-S2) : l'injecteur remet au récepteur deux doigts posés à l'écart `from`, `steps` mises à
    // jour, relâchés à `to`, milieu fixe ; un doigt seul suit les mêmes règles (page → scène).
    void pinchDeliversTwoTouchPoints()
    {
        QWindow window;
        bench::Injector injector(&window, [](QPointF p) { return p; });
        struct Frame
        {
            bench::GesturePhase phase = bench::GesturePhase::Press;
            std::vector<QPointF> fingers;
        };
        std::vector<Frame> frames;
        const bench::Injector::FingerSink sink = [&frames](bench::GesturePhase phase,
                                                           const std::vector<QPointF>& fingers) {
            Frame frame;
            frame.phase = phase;
            frame.fingers = fingers;
            frames.push_back(frame);
        };
        QVERIFY(injector.pinch(QPointF(200, 150), 100, 200, 10, sink, bench::PointSpace::Window));
        QCOMPARE(frames.size(), std::size_t{12}); // appui, 10 mises à jour, relâché
        QVERIFY(frames.front().phase == bench::GesturePhase::Press);
        QVERIFY(frames.back().phase == bench::GesturePhase::Release);
        QVERIFY(frames[5].phase == bench::GesturePhase::Move);
        double previous = 0;
        for (const Frame& frame : frames) {
            QCOMPARE(frame.fingers.size(), std::size_t{2});
            const double spread = frame.fingers[1].x() - frame.fingers[0].x();
            QVERIFY2(spread >= previous, qPrintable(QString::number(spread))); // l'écart ne se referme jamais
            previous = spread;
            QVERIFY2(qAbs((frame.fingers[0].x() + frame.fingers[1].x()) / 2 - 200) <= 1, "milieu en x");
            QVERIFY2(qAbs(frame.fingers[0].y() - 150) <= 1 && qAbs(frame.fingers[1].y() - 150) <= 1,
                     "milieu en y");
        }
        const auto spreadOf = [](const Frame& frame) { return frame.fingers[1].x() - frame.fingers[0].x(); };
        QVERIFY2(qAbs(spreadOf(frames.front()) - 100) <= 1,
                 qPrintable(QString::number(spreadOf(frames.front()))));
        QVERIFY2(qAbs(spreadOf(frames.back()) - 200) <= 1,
                 qPrintable(QString::number(spreadOf(frames.back()))));

        // Arguments invalides : rien n'est émis.
        QVERIFY(!injector.pinch(QPointF(200, 150), 0, 200, 10, sink, bench::PointSpace::Window));
        QVERIFY(!injector.pinch(QPointF(200, 150), 100, -5, 10, sink, bench::PointSpace::Window));
        QVERIFY(!injector.pinch(QPointF(200, 150), 100, 200, 0, sink, bench::PointSpace::Window));
        QCOMPARE(frames.size(), std::size_t{12});

        // Un doigt, repère de page : la conversion double les coordonnées ; appui, 16 déplacements, relâché.
        bench::Injector doubling(&window, [](QPointF p) { return p * 2; });
        frames.clear();
        QVERIFY(doubling.touchPath({QPointF(10, 10), QPointF(50, 10)}, sink));
        QCOMPARE(frames.size(), std::size_t{18});
        QCOMPARE(frames.front().fingers.size(), std::size_t{1});
        QCOMPARE(frames.front().fingers[0], QPointF(20, 20));
        QCOMPARE(frames.back().fingers[0], QPointF(100, 20));
        QVERIFY(frames.back().phase == bench::GesturePhase::Release);
        QVERIFY(!doubling.touchPath({}, sink));
        QCOMPARE(doubling.scenePoint(QPointF(3, 4), bench::PointSpace::Page), QPointF(6, 8));
        QCOMPARE(doubling.scenePoint(QPointF(3, 4), bench::PointSpace::Window), QPointF(3, 4));
    }

    // Pointeur (L0-S2) : le maintien sépare l'appui du relâché (instants relevés à l'arrivée au récepteur).
    void pointerHoldsBeforeRelease()
    {
        QWindow window;
        bench::Injector injector(&window, [](QPointF p) { return p; });
        std::vector<bench::GesturePhase> phases;
        std::vector<QPointF> positions;
        std::vector<std::int64_t> atNs;
        const bench::Injector::PointerSink sink = [&](bench::GesturePhase phase, QPointF position) {
            phases.push_back(phase);
            positions.push_back(position);
            atNs.push_back(core::monotonicNowNs());
        };
        QVERIFY(
            injector.pointerPath({QPointF(10, 20), QPointF(60, 20)}, sink, 120, bench::PointSpace::Window));
        QCOMPARE(phases.size(), std::size_t{18}); // appui, 16 déplacements, relâché
        QVERIFY(phases.front() == bench::GesturePhase::Press);
        QVERIFY(phases[1] == bench::GesturePhase::Move);
        QVERIFY(phases.back() == bench::GesturePhase::Release);
        QCOMPARE(positions.front(), QPointF(10, 20));
        QCOMPARE(positions.back(), QPointF(60, 20));
        const std::int64_t heldNs = atNs.back() - atNs.front();
        QVERIFY2(heldNs >= 120'000'000, qPrintable(QString::number(heldNs)));
        // Le maintien suit le dernier déplacement : c'est entre lui et le relâché que le temps passe.
        QVERIFY2(atNs.back() - atNs[phases.size() - 2] >= 120'000'000, "maintien avant le relâché");

        // Sans maintien : appui et relâché, rien entre eux ; un point suffit.
        phases.clear();
        positions.clear();
        atNs.clear();
        QVERIFY(injector.pointerPath({QPointF(5, 5)}, sink, 0, bench::PointSpace::Window));
        QCOMPARE(phases.size(), std::size_t{2});

        // Arguments invalides : rien n'est émis.
        phases.clear();
        QVERIFY(!injector.pointerPath({QPointF(5, 5)}, sink, -1, bench::PointSpace::Window));
        QVERIFY(!injector.pointerPath({}, sink, 0, bench::PointSpace::Window));
        QVERIFY(phases.empty());
    }
};

QTEST_MAIN(TstBenchInjector)
#include "tst_bench_injector.moc"
