#include <actcraft/features/pagetools.h>

#include <actcraft/core/model.h>

#include <QtTest>

#include <utility>

using namespace actcraft::features;
namespace core = actcraft::core;

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

    // F-592 : un rideau de page sans mode (Off) ou « complet » (Full) s'ouvre fermé en entier. CurtainMode::Full
    // vaut 5 et Curtain::Start::Full 0 : un transtypage par valeur rendrait un côté hors de l'énumération.
    void curtainModeOffMeansFull()
    {
        core::PagePresentation p;
        p.tools = core::PageToolsMode::Curtain;
        for (const core::CurtainMode mode : {core::CurtainMode::Off, core::CurtainMode::Full}) {
            p.curtain = mode;
            const PageToolsSettings settings = toPageToolsSettings(p);
            QCOMPARE(settings.curtainStart, Curtain::Start::Full);
            const auto next = resolvePageTools({false, Spotlight::Shape::Off}, settings);
            QVERIFY(next.curtain);
            QVERIFY(next.resetCurtain);
            QCOMPARE(next.curtainStart, Curtain::Start::Full);
        }
    }

    // Chaque valeur des cinq champs lus par les outils passe la conversion.
    void conversionKeepsEveryValue()
    {
        core::PagePresentation p;
        const std::pair<core::PageToolsMode, PageToolChoice> tools[] = {
            {core::PageToolsMode::Keep, PageToolChoice::Keep},
            {core::PageToolsMode::Curtain, PageToolChoice::Curtain},
            {core::PageToolsMode::Spotlight, PageToolChoice::Spotlight},
            {core::PageToolsMode::Off, PageToolChoice::Off}};
        for (const auto& [mode, choice] : tools) {
            p.tools = mode;
            QCOMPARE(toPageToolsSettings(p).tools, choice);
        }
        const std::pair<core::CurtainMode, Curtain::Start> curtains[] = {
            {core::CurtainMode::Off, Curtain::Start::Full},       {core::CurtainMode::Top, Curtain::Start::Top},
            {core::CurtainMode::Bottom, Curtain::Start::Bottom}, {core::CurtainMode::Left, Curtain::Start::Left},
            {core::CurtainMode::Right, Curtain::Start::Right},   {core::CurtainMode::Full, Curtain::Start::Full}};
        for (const auto& [mode, start] : curtains) {
            p.curtain = mode;
            QCOMPARE(toPageToolsSettings(p).curtainStart, start);
        }
        for (const double distance : {0.0, 120.5, 700.0}) {
            p.curtainDistance = distance;
            QCOMPARE(toPageToolsSettings(p).curtainDistance, distance);
        }
        const std::pair<core::SpotlightShape, Spotlight::Shape> shapes[] = {
            {core::SpotlightShape::Off, Spotlight::Shape::Off},
            {core::SpotlightShape::Circle, Spotlight::Shape::Circle},
            {core::SpotlightShape::Square, Spotlight::Shape::Square},
            {core::SpotlightShape::SolidCircle, Spotlight::Shape::SolidCircle},
            {core::SpotlightShape::SolidSquare, Spotlight::Shape::SolidSquare}};
        for (const auto& [shape, own] : shapes) {
            p.spotlight = shape;
            QCOMPARE(toPageToolsSettings(p).spotlight, own);
        }
        QCOMPARE(toPageToolsSettings(p).spotlightFrame, QRectF()); // vide : cadre par défaut
        p.spotlightFrame = core::RectF{10, 20, 200, 100};
        QCOMPARE(toPageToolsSettings(p).spotlightFrame, QRectF(10, 20, 200, 100));
    }
};

QTEST_GUILESS_MAIN(TstFeaturesPageTools)
#include "tst_features_pagetools.moc"
