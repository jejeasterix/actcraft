#include <actcraft/bench/machineinfo.h>
#include <actcraft/bench/memory.h>

#include <QQuickWindow>
#include <QSGRendererInterface>
#include <QTest>

using namespace actcraft::bench;

class TstBenchMemory : public QObject
{
    Q_OBJECT

private slots:
    void processMemoryIsReported()
    {
#if defined(Q_OS_WIN) || defined(Q_OS_MACOS)
        const ProcessMemory m = readProcessMemory();
        QVERIFY(m.privateBytes > 0);
        QVERIFY(m.workingSetBytes > 0);
#else
        QSKIP("mémoire du processus non relevée sur cette plateforme");
#endif
    }

    void trackerKeepsStartEndAndPeak()
    {
        MemoryTracker tracker;
        tracker.sample({100, 50, 50}, 10);
        tracker.sample({300, 70, 70}, 30);
        tracker.sample({200, 60, 70}, 20);
        const MemorySummary s = tracker.summary();
        QCOMPARE(s.privateStart, std::uint64_t{100});
        QCOMPARE(s.privateEnd, std::uint64_t{200});
        QCOMPARE(s.privatePeak, std::uint64_t{300});
        QCOMPARE(s.workingSetPeak, std::uint64_t{70});
        QCOMPARE(s.gpuStart.value(), std::uint64_t{10});
        QCOMPARE(s.gpuEnd.value(), std::uint64_t{20});
        QCOMPARE(s.gpuPeak.value(), std::uint64_t{30});
        tracker.reset();
        tracker.sample({5, 5, 5}, std::nullopt);
        QCOMPARE(tracker.summary().privateStart, std::uint64_t{5});
        QVERIFY(!tracker.summary().gpuPeak.has_value());
    }

    void machineInfoHasBuildData()
    {
        const MachineInfo info = readMachineInfo();
        QVERIFY(!info.buildType.isEmpty());
        QVERIFY(!info.commit.isEmpty());
        QVERIFY(!info.compilerFlags.isEmpty());
        QVERIFY(info.qtVersion.startsWith(QStringLiteral("6.12")));
    }

    void gpuReaderIsNullInSoftware()
    {
        QQuickWindow window;
        window.resize(100, 100);
        window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        QCOMPARE(window.rendererInterface()->graphicsApi(), QSGRendererInterface::Software);
        QVERIFY(GpuMemoryReader::create(&window) == nullptr);
    }
};

QTEST_MAIN(TstBenchMemory)
#include "tst_bench_memory.moc"
