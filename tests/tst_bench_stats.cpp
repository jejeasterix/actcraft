#include <actcraft/bench/stats.h>

#include <QTest>

using namespace actcraft::bench;

namespace {
constexpr std::int64_t ms(double v)
{
    return static_cast<std::int64_t>(v * 1'000'000.0);
}

constexpr std::int64_t kPeriod60Hz = 16'666'667;

FrameSeries at60Hz()
{
    FrameSeries s;
    s.refreshPeriodNs = kPeriod60Hz;
    return s;
}

bool near(double a, double b)
{
    return qAbs(a - b) < 1e-6;
}
} // namespace

class TstBenchStats : public QObject
{
    Q_OBJECT

private slots:
    void percentileIsNearestRank()
    {
        std::vector<double> values;
        for (int i = 100; i >= 1; --i) {
            values.push_back(i);
        }
        const Percentiles p = summarize(values);
        QCOMPARE(p.p50, 50.0);
        QCOMPARE(p.p95, 95.0);
        QCOMPARE(p.p99, 99.0);
        QCOMPARE(p.max, 100.0);
        QCOMPARE(p.count, std::size_t{100});
        QCOMPARE(percentile({7.0}, 95), 7.0);
        QCOMPARE(percentile({}, 50), 0.0);
    }

    void percentileRankIsExactForEveryIntegerP()
    {
        // Rang ceil(p × n / 100) calculé sans arrondi : p / 100.0 × n donnerait 8 pour p = 7, n = 100.
        std::vector<double> values;
        for (int i = 1; i <= 100; ++i) {
            values.push_back(i);
        }
        for (int p = 1; p <= 100; ++p) {
            QCOMPARE(percentile(values, p), static_cast<double>(p));
        }
    }

    void invalidRefreshPeriodComputesNothing()
    {
        // Sans période mesurée, aucun seuil n'a de sens : indicateur explicite, pas de faux « 0 image/s ».
        FrameSeries s;
        s.swapNs = {0, ms(16), ms(33)};
        s.inputNs = {ms(5), ms(20)};
        const IntervalStats unset = classifyIntervals(s);
        QVERIFY(unset.invalidRefreshPeriod);
        QVERIFY(unset.classes.empty());
        QCOMPARE(unset.fps, 0.0);
        s.refreshPeriodNs = -1;
        QVERIFY(classifyIntervals(s).invalidRefreshPeriod);
        s.refreshPeriodNs = kPeriod60Hz;
        const IntervalStats valid = classifyIntervals(s);
        QVERIFY(!valid.invalidRefreshPeriod);
        QCOMPARE(valid.activeMs.size(), std::size_t{2});
    }

    void intervalNeedsAnInput()
    {
        FrameSeries s = at60Hz();
        s.swapNs = {0, ms(16), ms(33), ms(50)};
        s.inputNs = {ms(5), ms(20)};
        const IntervalStats r = classifyIntervals(s);
        QCOMPARE(r.classes.size(), std::size_t{3});
        QCOMPARE(r.classes[0], IntervalClass::Active);
        QCOMPARE(r.classes[1], IntervalClass::Active);
        QCOMPARE(r.classes[2], IntervalClass::Idle);
        QCOMPARE(r.activeMs.size(), std::size_t{2});
    }

    void firstIntervalAfterAPauseIsAResume()
    {
        FrameSeries s = at60Hz();
        s.swapNs = {0, ms(16), ms(316), ms(333)};
        s.inputNs = {ms(5), ms(310), ms(320)};
        const IntervalStats r = classifyIntervals(s);
        QCOMPARE(r.classes[0], IntervalClass::Active);
        QCOMPARE(r.classes[1], IntervalClass::Resume); // 294 ms sans entrée depuis l'image de départ
        QCOMPARE(r.classes[2], IntervalClass::Active);
        QVERIFY(r.active.max < 20.0);
        QCOMPARE(r.resumeMs.size(), std::size_t{1});
        QVERIFY(near(r.resumeMs[0], 6.0)); // 316 − 310 : la pause n'en fait pas partie
    }

    void slowResumeIsCountedApart()
    {
        // Accroc de 200 ms au début d'un trait : invisible dans les intervalles actifs, compté comme reprise.
        FrameSeries slow = at60Hz();
        slow.swapNs = {0, ms(16), ms(516), ms(533)};
        slow.inputNs = {ms(5), ms(310), ms(520)};
        const IntervalStats r = classifyIntervals(slow);
        QCOMPARE(r.classes[1], IntervalClass::Resume);
        QVERIFY(r.active.max < 20.0);
        QCOMPARE(r.resumeOverLimit, std::size_t{1}); // 206 ms ≥ 2,5 × 16,7 ms
        QVERIFY(near(r.resumeMaxMs, 206.0));

        FrameSeries fast = at60Hz();
        fast.swapNs = {0, ms(16), ms(320), ms(336)};
        fast.inputNs = {ms(5), ms(310), ms(325)};
        const IntervalStats f = classifyIntervals(fast);
        QCOMPARE(f.classes[1], IntervalClass::Resume);
        QCOMPARE(f.resumeOverLimit, std::size_t{0});
        QVERIFY(near(f.resumeMaxMs, 10.0));
    }

    void freezeIsCountedApart()
    {
        FrameSeries s = at60Hz();
        s.swapNs = {0, ms(16), ms(33), ms(50)};
        s.inputNs = {ms(5), ms(18), ms(40)};
        s.freezeNs = {ms(20)};
        const IntervalStats r = classifyIntervals(s);
        QCOMPARE(r.classes[1], IntervalClass::Freeze);
        QCOMPARE(r.freezeMs.size(), std::size_t{1});
        QCOMPARE(r.freezeOverLimit, std::size_t{0});
        QCOMPARE(r.activeMs.size(), std::size_t{2});

        FrameSeries slow = at60Hz();
        slow.swapNs = {0, ms(16), ms(66)};
        slow.inputNs = {ms(5), ms(18)};
        slow.freezeNs = {ms(20)};
        const IntervalStats l = classifyIntervals(slow);
        QCOMPARE(l.freezeOverLimit, std::size_t{1}); // 50 ms ≥ 2,5 × 16,7 ms
        QVERIFY(qAbs(l.freezeMaxMs - 50.0) < 1e-6);
    }

    void freezeNeverCountsAPause()
    {
        // Figement à 20 ms sans image avant le trait suivant (posé à 318 ms) : la pause n'est pas un
        // figement.
        FrameSeries spanning = at60Hz();
        spanning.swapNs = {0, ms(16), ms(336), ms(352)};
        spanning.inputNs = {ms(5), ms(15), ms(318), ms(340)};
        spanning.freezeNs = {ms(20)};
        const IntervalStats a = classifyIntervals(spanning);
        QCOMPARE(a.classes[1], IntervalClass::Resume);
        QVERIFY(a.freezeMs.empty());
        QCOMPARE(a.freezeOverLimit, std::size_t{0});
        QCOMPARE(a.freezeUnmeasured, std::size_t{1});
        QVERIFY(near(a.resumeMaxMs, 18.0)); // 336 − 318
        QCOMPARE(a.resumeOverLimit, std::size_t{0});

        // Figement tardif (trait figé à 300 ms, posé suivant à 318 ms) : c'est un vrai figement, hors limite.
        FrameSeries late = spanning;
        late.freezeNs = {ms(300)};
        const IntervalStats b = classifyIntervals(late);
        QCOMPARE(b.classes[1], IntervalClass::Freeze);
        QCOMPARE(b.freezeOverLimit, std::size_t{1});
        QCOMPARE(b.freezeUnmeasured, std::size_t{0});

        // Trait court après une pause (posé 310, levé 315, figé 316, image 330) : durée comptée depuis le
        // posé.
        FrameSeries tap = at60Hz();
        tap.swapNs = {0, ms(16), ms(330), ms(346)};
        tap.inputNs = {ms(5), ms(310), ms(315), ms(335)};
        tap.freezeNs = {ms(316)};
        const IntervalStats c = classifyIntervals(tap);
        QCOMPARE(c.classes[1], IntervalClass::Freeze);
        QVERIFY(near(c.freezeMaxMs, 20.0));
        QCOMPARE(c.freezeOverLimit, std::size_t{0});
    }

    void thresholdsFollowTheMeasuredRefreshPeriod()
    {
        // 60 Hz : un figement de 38 ms reste sous 2,5 × 16,7 = 41,7 ms ; un seuil fixe de 33,4 ms le
        // compterait.
        FrameSeries hz60 = at60Hz();
        hz60.swapNs = {0, ms(16), ms(54)};
        hz60.inputNs = {ms(5), ms(18)};
        hz60.freezeNs = {ms(20)};
        QCOMPARE(classifyIntervals(hz60).freezeOverLimit, std::size_t{0});

        // 144 Hz : la limite tombe à 2,5 × 6,94 = 17,4 ms ; un seuil fixe de 41,7 ms ignorerait ce figement.
        FrameSeries hz144;
        hz144.refreshPeriodNs = 6'944'444;
        hz144.swapNs = {0, ms(7), ms(27)};
        hz144.inputNs = {ms(2), ms(8)};
        hz144.freezeNs = {ms(9)};
        QCOMPARE(classifyIntervals(hz144).freezeOverLimit, std::size_t{1});

        // 144 Hz : 20 ms sans entrée depuis l'image de départ dépassent 2 périodes (13,9 ms) ; un seuil fixe
        // de 33,3 ms garderait cet intervalle actif.
        FrameSeries pause144;
        pause144.refreshPeriodNs = 6'944'444;
        pause144.swapNs = {0, ms(7), ms(30)};
        pause144.inputNs = {ms(2), ms(27)};
        const IntervalStats p = classifyIntervals(pause144);
        QCOMPARE(p.classes[0], IntervalClass::Active);
        QCOMPARE(p.classes[1], IntervalClass::Resume);
    }

    void thresholdBoundaries()
    {
        // Période de 10 ms : pause au-delà de 2 périodes (strictement), hors limite dès 2,5 périodes.
        FrameSeries s;
        s.refreshPeriodNs = 10'000'000;
        s.swapNs = {0, ms(30), ms(60), ms(85), ms(110) - 1};
        s.inputNs = {ms(20), ms(50) + 1};
        s.freezeNs = {ms(70), ms(90)};
        const IntervalStats r = classifyIntervals(s);
        QCOMPARE(r.classes[0], IntervalClass::Active); // première entrée à 2 périodes pile
        QCOMPARE(r.classes[1], IntervalClass::Resume); // 1 ns de plus
        QCOMPARE(r.classes[2], IntervalClass::Freeze); // 25 ms = 2,5 périodes : hors limite
        QCOMPARE(r.classes[3], IntervalClass::Freeze); // 25 ms − 1 ns : dans la limite
        QCOMPARE(r.freezeOverLimit, std::size_t{1});

        FrameSeries resume;
        resume.refreshPeriodNs = 10'000'000;
        resume.swapNs = {0, ms(60), ms(120) - 1};
        resume.inputNs = {ms(35), ms(95)};
        const IntervalStats q = classifyIntervals(resume);
        QCOMPARE(q.classes[0], IntervalClass::Resume); // 25 ms : hors limite
        QCOMPARE(q.classes[1], IntervalClass::Resume); // 25 ms − 1 ns : dans la limite
        QCOMPARE(q.resumeOverLimit, std::size_t{1});
    }

    void activeIntervalOfExactly25MsIsNotOver()
    {
        FrameSeries s = at60Hz();
        s.swapNs = {0, ms(25), ms(50) + 1};
        s.inputNs = {ms(1), ms(26)};
        const IntervalStats r = classifyIntervals(s);
        QCOMPARE(r.activeMs.size(), std::size_t{2});
        QCOMPARE(r.over25Ratio, 0.5); // 25 ms pile n'est pas « > 25 ms », 25 ms + 1 ns l'est
    }

    void instantsOnAFrameBelongToTheIntervalTheyEnd()
    {
        FrameSeries input = at60Hz();
        input.swapNs = {0, ms(16), ms(32)};
        input.inputNs = {ms(16)};
        const IntervalStats i = classifyIntervals(input);
        QCOMPARE(i.classes[0], IntervalClass::Active);
        QCOMPARE(i.classes[1], IntervalClass::Idle);

        FrameSeries freeze = at60Hz();
        freeze.swapNs = {0, ms(16), ms(32)};
        freeze.inputNs = {ms(5), ms(20)};
        freeze.freezeNs = {ms(16)};
        const IntervalStats f = classifyIntervals(freeze);
        QCOMPARE(f.classes[0], IntervalClass::Freeze);
        QCOMPARE(f.classes[1], IntervalClass::Active);
    }

    void plannedInstantsKeepABlockedIntervalActive()
    {
        // Fil principal bloqué 90 ms : entrées prévues toutes les 5 ms, postées en bloc à la fin.
        FrameSeries planned = at60Hz();
        planned.swapNs = {0, ms(90)};
        for (int i = 1; i <= 17; ++i) {
            planned.inputNs.push_back(ms(5.0 * i));
        }
        const IntervalStats r = classifyIntervals(planned);
        QCOMPARE(r.classes[0], IntervalClass::Active);
        QVERIFY(qAbs(r.active.max - 90.0) < 1e-6); // le blocage compte dans p95, p99 et max
        FrameSeries posted = planned;
        posted.inputNs.assign(17, ms(89));
        const IntervalStats p = classifyIntervals(posted);
        QCOMPARE(p.classes[0], IntervalClass::Resume); // d'où les instants prévus :
        QVERIFY(near(p.resumeMaxMs, 1.0));             // avec les instants postés, le blocage disparaît
    }

    void fpsAndShareOver25()
    {
        FrameSeries s = at60Hz();
        for (int i = 0; i <= 100; ++i) {
            s.swapNs.push_back(ms(16.6667 * i + (i > 98 ? 20 : 0)));
            s.inputNs.push_back(ms(16.6667 * i + 1));
        }
        const IntervalStats r = classifyIntervals(s);
        QVERIFY(qAbs(r.fps - 60.0) < 0.1);
        QVERIFY(r.over25Ratio > 0.0 && r.over25Ratio <= 0.011);
    }

    void latencyUsesFirstFrameContainingTheSample()
    {
        const std::vector<std::int64_t> planned{0, ms(5), ms(10)};
        const std::vector<SyncRecord> records{{0, ms(8), ms(20)}, {2, ms(24), ms(36)}};
        const LatencyStats l = computeLatency(planned, records);
        QCOMPARE(l.seq.size(), std::size_t{3});
        QVERIFY(qAbs(l.totalMs[0] - 20.0) < 1e-9);
        QVERIFY(qAbs(l.plannedToSyncMs[0] - 8.0) < 1e-9);
        QVERIFY(qAbs(l.syncToSwapMs[0] - 12.0) < 1e-9);
        QVERIFY(qAbs(l.totalMs[1] - 31.0) < 1e-9);
        QVERIFY(qAbs(l.totalMs[2] - 26.0) < 1e-9);
        QCOMPARE(l.unmatched, std::size_t{0});
    }

    void unmeasuredAndUnmatchedSamples()
    {
        const std::vector<std::int64_t> planned{0, -1, ms(10), ms(15)};
        const std::vector<SyncRecord> records{{2, ms(12), ms(20)}};
        const LatencyStats l = computeLatency(planned, records);
        QCOMPARE(l.seq.size(), std::size_t{2}); // 0 et 2 ; 1 non mesuré
        QCOMPARE(l.unmatched, std::size_t{1});  // 3 n'est dans aucune image
    }
};

QTEST_GUILESS_MAIN(TstBenchStats)
#include "tst_bench_stats.moc"
