#include <actcraft/tools/pentool.h>

#include <QTest>

#include <cstdint>
#include <string>
#include <utility>
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
    std::vector<core::Argb> colors; // couleur de l'encre vive à chaque trait
    void beginStroke(core::Argb color, double) override
    {
        m_log.calls.emplace_back("begin");
        colors.push_back(color);
    }
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

// FakeDocument ne connaît que commitStroke : il garde le chemin par défaut de commitAnnotation, celui des
// adaptateurs de ui/ qui ne le redéfinissent pas. Celui-ci le redéfinit et garde la translucidité reçue.
class AnnotationDocument final : public FakeDocument
{
public:
    using FakeDocument::FakeDocument;
    int lastTranslucency = -1; // -1 : commitAnnotation jamais appelé
    void commitAnnotation(core::Stroke stroke, std::uint8_t translucency) override
    {
        lastTranslucency = translucency;
        commitStroke(std::move(stroke));
    }
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

// Appui, deux mouvements, relâché.
void drawLine(tools::Tool& tool)
{
    tool.onSample(make(Phase::Press, 0, 0, 0));
    tool.onSample(make(Phase::Move, 4, 0, 1));
    tool.onSample(make(Phase::Move, 8, 0, 2));
    tool.onSample(make(Phase::Release, 8, 0, 3));
}

// Appui et un mouvement, sans relâché.
void pressAndMove(tools::Tool& tool)
{
    tool.onSample(make(Phase::Press, 0, 0, 0));
    tool.onSample(make(Phase::Move, 4, 0, 1));
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

    // Lot 3 (L3-T6) : le marqueur est le stylo de genre Highlighter (F-225), pas une classe à part.
    void highlighterStrokeIsTranslucentKind()
    {
        Log log;
        FakeInk ink(log);
        AnnotationDocument sink(log);
        tools::PenTool marker(ink, sink, core::StrokeKind::Highlighter);
        marker.setTranslucency(127); // D3 : opacité 128 sur 255
        QCOMPARE(marker.id(), QStringLiteral("highlighter"));
        QCOMPARE(marker.kind(), core::StrokeKind::Highlighter);
        QCOMPARE(marker.translucency(), std::uint8_t{127});
        drawLine(marker);
        QCOMPARE(sink.strokes.size(), std::size_t{1});
        QCOMPARE(sink.strokes[0].kind, core::StrokeKind::Highlighter);
        QCOMPARE(sink.lastTranslucency, 127);
    }

    // Chemin de référence : un trait lu d'un paperboard porte sa couleur opaque et sa translucidité dans
    // common.translucency (formats/src/flipchart.cpp), que le rendu applique en opacité 1 − t / 255
    // (render/src/pagepainter.cpp, render/src/objectnodes.cpp). Le trait versé doit donc garder sa couleur
    // opaque : une couleur déjà translucide doublerait l'effet. L'encre vive, elle, n'a pas d'opacité propre
    // : elle reçoit l'alpha 255 − t.
    void markerInkIsSeeThroughButItsStrokeStaysOpaque()
    {
        Log log;
        FakeInk ink(log);
        AnnotationDocument sink(log);
        tools::PenTool marker(ink, sink, core::StrokeKind::Highlighter);
        marker.setColor(0xFFFFFF00u);
        marker.setBaseWidth(20);
        marker.setTranslucency(127);
        drawLine(marker);
        QCOMPARE(ink.colors, (std::vector<core::Argb>{0x80FFFF00u}));
        QCOMPARE(sink.strokes.size(), std::size_t{1});
        QCOMPARE(sink.strokes[0].color, core::Argb{0xFFFFFF00u});
        QCOMPARE(sink.strokes[0].baseWidth, 20.0);
        QCOMPARE(sink.lastTranslucency, 127);

        // Le stylo, opaque : l'encre reçoit sa couleur telle quelle, et le trait passe aussi par
        // commitAnnotation.
        Log penLog;
        FakeInk penInk(penLog);
        AnnotationDocument penSink(penLog);
        tools::PenTool pen(penInk, penSink);
        pen.setColor(0xFF112233u);
        drawLine(pen);
        QCOMPARE(penInk.colors, (std::vector<core::Argb>{0xFF112233u}));
        QCOMPARE(penSink.strokes.size(), std::size_t{1});
        QCOMPARE(penSink.strokes[0].kind, core::StrokeKind::Pen);
        QCOMPARE(penSink.lastTranslucency, 0);
    }

    // D2 : les nouveaux traits ont des bouts ronds (le défaut de core::Stroke est carré). FakeDocument ne
    // redéfinit pas commitAnnotation : le chemin par défaut verse bien le trait par commitStroke.
    void newStrokesHaveRoundCaps()
    {
        Log log;
        FakeInk ink(log);
        FakeDocument document(log);
        tools::PenTool pen(ink, document);
        QCOMPARE(pen.id(), QStringLiteral("pen"));
        QCOMPARE(pen.kind(), core::StrokeKind::Pen);
        drawLine(pen);
        QCOMPARE(document.strokes.size(), std::size_t{1});
        QCOMPARE(document.strokes[0].cap, core::PenCap::Round);
        QCOMPARE(document.strokes[0].kind, core::StrokeKind::Pen);

        tools::PenTool marker(ink, document, core::StrokeKind::Highlighter);
        drawLine(marker);
        QCOMPARE(document.strokes.size(), std::size_t{2});
        QCOMPARE(document.strokes[1].cap, core::PenCap::Round);
        QCOMPARE(document.strokes[1].kind, core::StrokeKind::Highlighter);
    }

    void cancelCommitsTheStrokeInProgress()
    {
        Log log;
        FakeInk ink(log);
        FakeDocument document(log);
        tools::PenTool pen(ink, document);
        pressAndMove(pen);
        QVERIFY(pen.isBusy());
        pen.cancel();
        QVERIFY(!pen.isBusy());
        QCOMPARE(document.strokes.size(), std::size_t{1});

        // Le marqueur annulé verse par le même chemin : genre, bouts et translucidité gardés.
        Log markerLog;
        FakeInk markerInk(markerLog);
        AnnotationDocument sink(markerLog);
        tools::PenTool marker(markerInk, sink, core::StrokeKind::Highlighter);
        marker.setTranslucency(127);
        pressAndMove(marker);
        marker.cancel();
        QVERIFY(!marker.isBusy());
        QCOMPARE(sink.strokes.size(), std::size_t{1});
        QCOMPARE(sink.strokes[0].kind, core::StrokeKind::Highlighter);
        QCOMPARE(sink.strokes[0].cap, core::PenCap::Round);
        QCOMPARE(sink.lastTranslucency, 127);
    }

    // L3-T13 (H3-17) : abandon oublie le trait en cours (rien de versé, encre vidée) ; cancel verse toujours.
    // Par défaut, Tool::abandon fait cancel : la sélection et la gomme y abandonnent déjà sans étape.
    void abandonCommitsNothingCancelStillCommits()
    {
        Log log;
        FakeInk ink(log);
        FakeDocument document(log);
        tools::PenTool pen(ink, document);
        pressAndMove(pen);
        QVERIFY(pen.isBusy());
        pen.abandon();
        QVERIFY(!pen.isBusy());
        QVERIFY(document.strokes.empty());
        QCOMPARE(log.calls.back(), std::string("clear"));
        pen.abandon();                               // sans trait : rien
        pen.onSample(make(Phase::Move, 8, 0, 2));    // la suite du contact n'écrit plus
        pen.onSample(make(Phase::Release, 8, 0, 3)); // ni ne verse
        QVERIFY(document.strokes.empty());
        pressAndMove(pen);
        pen.cancel();
        QCOMPARE(document.strokes.size(), std::size_t{1});

        struct Counting final : tools::Tool
        {
            int cancels = 0;
            QString id() const override { return QStringLiteral("essai"); }
            bool isBusy() const noexcept override { return false; }
            void cancel() override { ++cancels; }
            void onSample(const input::Sample&) override {}
        };
        Counting other;
        other.abandon();
        QCOMPARE(other.cancels, 1);
    }

    // La translucidité d'un trait est celle de son appui, comme sa couleur : changée en plein trait, elle
    // vaut pour le suivant (l'encre vive et le trait versé restent d'accord).
    void translucencyIsTakenAtThePress()
    {
        Log log;
        FakeInk ink(log);
        AnnotationDocument sink(log);
        tools::PenTool marker(ink, sink, core::StrokeKind::Highlighter);
        marker.setTranslucency(127);
        pressAndMove(marker);
        marker.setTranslucency(200);
        marker.onSample(make(Phase::Release, 8, 0, 2));
        QCOMPARE(sink.lastTranslucency, 127);
        drawLine(marker);
        QCOMPARE(sink.lastTranslucency, 200);
        QCOMPARE(ink.colors, (std::vector<core::Argb>{0x80000000u, 0x37000000u}));
    }
};

QTEST_GUILESS_MAIN(TstPenTool)
#include "tst_pentool.moc"
