#include <actcraft/tools/pentool.h>

#include <QTest>

#include <string>
#include <vector>

using namespace actcraft;
using input::Phase;

namespace {

struct Log
{
    std::vector<std::string> calls;
};

class FakeInk : public tools::InkSurface
{
public:
    explicit FakeInk(Log& log) : m_log(log) {}
    std::vector<std::uint64_t> seqs;
    void beginStroke(core::Argb, double) override { m_log.calls.emplace_back("begin"); }
    void addPoint(const core::StrokePoint&, std::uint64_t seq) override
    {
        m_log.calls.emplace_back("add");
        seqs.push_back(seq);
    }
    void clear() override { m_log.calls.emplace_back("clear"); }

private:
    Log& m_log;
};

class FakeDocument : public tools::DocumentSink
{
public:
    explicit FakeDocument(Log& log) : m_log(log) {}
    std::vector<core::Stroke> strokes;
    void commitStroke(core::Stroke stroke) override
    {
        m_log.calls.emplace_back("commit");
        strokes.push_back(std::move(stroke));
    }

private:
    Log& m_log;
};

input::Sample make(Phase phase, double x, double y, std::uint64_t seq, float pressure = 0.5f)
{
    input::Sample s;
    s.phase = phase;
    s.x = x;
    s.y = y;
    s.seq = seq;
    s.pressure = pressure;
    return s;
}

} // namespace

class TstPenTool : public QObject
{
    Q_OBJECT

private slots:
    void pressMoveReleaseCommitsThenClears()
    {
        Log log;
        FakeInk ink(log);
        FakeDocument document(log);
        tools::PenTool pen(ink, document);
        pen.setColor(0xFF112233u);
        pen.setBaseWidth(4);
        pen.onSample(make(Phase::Press, 0, 0, 0, 0.3f));
        QVERIFY(pen.isDrawing());
        pen.onSample(make(Phase::Move, 1, 0, 1));
        pen.onSample(make(Phase::Release, 2, 0, 2, 0.0f));
        QVERIFY(!pen.isDrawing());
        const std::vector<std::string> expected{"begin", "add", "add", "add", "commit", "clear"};
        QCOMPARE(log.calls, expected);
        QCOMPARE(document.strokes.size(), std::size_t{1});
        QCOMPARE(document.strokes[0].points.size(), std::size_t{3});
        QCOMPARE(document.strokes[0].color, core::Argb{0xFF112233u});
        QCOMPARE(document.strokes[0].baseWidth, 4.0);
        QCOMPARE(document.strokes[0].points[0].pressure, 0.3f);
    }

    void sequenceNumbersReachTheInk()
    {
        Log log;
        FakeInk ink(log);
        FakeDocument document(log);
        tools::PenTool pen(ink, document);
        pen.onSample(make(Phase::Press, 0, 0, 7));
        pen.onSample(make(Phase::Move, 1, 1, 8));
        const std::vector<std::uint64_t> expected{7, 8};
        QCOMPARE(ink.seqs, expected);
    }

    void moveWithoutPressIsIgnored()
    {
        Log log;
        FakeInk ink(log);
        FakeDocument document(log);
        tools::PenTool pen(ink, document);
        pen.onSample(make(Phase::Move, 1, 1, 0));
        pen.onSample(make(Phase::Release, 1, 1, 1));
        QVERIFY(log.calls.empty());
    }

    void singleTapDoesNotCommit()
    {
        Log log;
        FakeInk ink(log);
        FakeDocument document(log);
        tools::PenTool pen(ink, document);
        pen.onSample(make(Phase::Press, 5, 5, 0));
        pen.onSample(make(Phase::Release, 5, 5, 1));
        QVERIFY(document.strokes.empty());
        QCOMPARE(log.calls.back(), std::string("clear"));
    }

    void pressWhileDrawingFinishesPrevious()
    {
        Log log;
        FakeInk ink(log);
        FakeDocument document(log);
        tools::PenTool pen(ink, document);
        pen.onSample(make(Phase::Press, 0, 0, 0));
        pen.onSample(make(Phase::Move, 3, 0, 1));
        pen.onSample(make(Phase::Press, 10, 10, 2)); // relâché perdu
        QCOMPARE(document.strokes.size(), std::size_t{1});
        QVERIFY(pen.isDrawing());
    }

    // Socle (L0-S7, D-S9) : le stylo vu comme un outil. La vue garde le versement différé sur isBusy() et
    // termine un geste par cancel().
    void penIsATool()
    {
        Log log;
        FakeInk ink(log);
        FakeDocument document(log);
        tools::PenTool pen(ink, document);
        tools::Tool& tool = pen;
        QCOMPARE(tool.id(), QStringLiteral("pen"));
        QVERIFY(!tool.isBusy());
        tool.cancel(); // sans trait : rien
        QVERIFY(log.calls.empty());
        tool.onSample(make(Phase::Press, 0, 0, 0));
        QVERIFY(tool.isBusy());
        tool.onSample(make(Phase::Move, 4, 0, 1));
        QVERIFY(tool.isBusy());
        tool.cancel(); // verse le trait en cours, tel quel
        QVERIFY(!tool.isBusy());
        QCOMPARE(document.strokes.size(), std::size_t{1});
        QCOMPARE(document.strokes[0].points.size(), std::size_t{2});
        QCOMPARE(log.calls.back(), std::string("clear"));
        tool.onSample(make(Phase::Move, 8, 0, 2)); // la suite du contact n'écrit plus
        QCOMPARE(document.strokes.size(), std::size_t{1});
        tool.onSample(make(Phase::Press, 0, 5, 3));
        tool.onSample(make(Phase::Release, 6, 5, 4));
        QVERIFY(!tool.isBusy()); // le relâché termine aussi
        QCOMPARE(document.strokes.size(), std::size_t{2});
    }
};

QTEST_GUILESS_MAIN(TstPenTool)
#include "tst_pentool.moc"
