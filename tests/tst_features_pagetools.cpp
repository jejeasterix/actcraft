#include <actcraft/features/pagetools.h>

#include <QtTest>

using namespace actcraft::features;

class TstFeaturesPageTools : public QObject
{
    Q_OBJECT

private slots:
    void keepCarriesCurtainAcrossPages()
    {
        const PresentationState now{true, Spotlight::Shape::Off};     // rideau ouvert sur la page quittée
        const auto next = resolvePageTools(now, PageToolsSettings{}); // page « comme avant »
        QVERIFY(next.curtain);
        QCOMPARE(next.resetCurtain, false);
    }
    void offStopsTools()
    {
        PageToolsSettings p;
        p.tools = PageToolChoice::Off;
        const auto next = resolvePageTools({true, Spotlight::Shape::Circle}, p);
        QVERIFY(!next.curtain);
        QCOMPARE(next.spotlight, Spotlight::Shape::Off);
    }
    void pageModeOverridesKeep()
    {
        PageToolsSettings p;
        p.tools = PageToolChoice::Curtain;
        p.curtainStart = Curtain::Start::Left;
        p.curtainDistance = 120;
        const auto next = resolvePageTools({false, Spotlight::Shape::Off}, p);
        QVERIFY(next.curtain);
        QVERIFY(next.resetCurtain);
        QCOMPARE(next.curtainStart, Curtain::Start::Left);
        QCOMPARE(next.curtainDistance, 120.0);
        QCOMPARE(next.spotlight, Spotlight::Shape::Off); // un seul outil de page à la fois (F-591)
    }
    void spotlightPageUsesItsFrame()
    {
        PageToolsSettings p;
        p.tools = PageToolChoice::Spotlight;
        p.spotlight = Spotlight::Shape::Square;
        p.spotlightFrame = QRectF(10, 20, 200, 100);
        const auto next = resolvePageTools({true, Spotlight::Shape::Off}, p);
        QVERIFY(!next.curtain);
        QCOMPARE(next.spotlight, Spotlight::Shape::Square);
        QCOMPARE(next.spotlightFrame, QRectF(10, 20, 200, 100));
    }
    void spotlightPageWithoutShapeOpensACircle()
    {
        PageToolsSettings p;
        p.tools = PageToolChoice::Spotlight; // forme absente de la page
        const auto next = resolvePageTools({false, Spotlight::Shape::Off}, p);
        QCOMPARE(next.spotlight, Spotlight::Shape::Circle);
    }
};

QTEST_GUILESS_MAIN(TstFeaturesPageTools)
#include "tst_features_pagetools.moc"
