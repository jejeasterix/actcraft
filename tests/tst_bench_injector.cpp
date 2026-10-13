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
};

QTEST_MAIN(TstBenchInjector)
#include "tst_bench_injector.moc"
