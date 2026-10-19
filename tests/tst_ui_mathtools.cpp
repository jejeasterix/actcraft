#include "mathtoolscontroller.h"
#include "support/lot7registry.h"
#include "support/presentationharness.h"

#include <actcraft/core/document.h>
#include <actcraft/core/model.h>
#include <actcraft/features/mathtools.h>
#include <actcraft/render/fixedpageitem.h>
#include <actcraft/render/inkitem.h>
#include <actcraft/tools/userstate.h>
#include <actcraft/ui/documentcontroller.h>
#include <actcraft/ui/pageview.h>
#include <actcraft/ui/toolhost.h>

#include <QCursor>
#include <QtTest>

#include <cstddef>
#include <initializer_list>
#include <variant>
#include <vector>

namespace {

// Tout le fichier dans cet espace de noms anonyme (exécutable unique : aucun nom global hors du registre).
using actcraft::ui::MathToolsController;
using presentationharness::MainWindow;
using presentationharness::Pointer;

MathToolsController* mathTools(const MainWindow& w)
{
    return w.window() ? w.window()->findChild<MathToolsController*>(QStringLiteral("mathTools")) : nullptr;
}

actcraft::ui::DocumentController* documentController(const MainWindow& w)
{
    return w.window() ? w.window()->findChild<actcraft::ui::DocumentController*>(QStringLiteral("documentController"))
                      : nullptr;
}

// Cadre d'un outil ouvert, lu du contrôleur (coordonnées de page) ; bord haut à centre - largeur / 2.
struct Frame
{
    double cx = 0;
    double cy = 0;
    double length = 0;
    double thickness = 0;
    double top() const { return cy - thickness / 2; }
    double left() const { return cx - length / 2; }
};

Frame frameOf(const MathToolsController& tools, int index)
{
    const QVariantMap m = tools.toolAt(index);
    return {m.value(QStringLiteral("centerX")).toDouble(), m.value(QStringLiteral("centerY")).toDouble(),
            m.value(QStringLiteral("length")).toDouble(), m.value(QStringLiteral("thickness")).toDouble()};
}

QPointF scene(const MainWindow& w, QPointF page)
{
    return w.pageView()->pageToScene(page);
}

// Trait au stylet le long de `pagePoints` (repère de page), quatre déplacements par segment.
void stylusPath(const MainWindow& w, std::initializer_list<QPointF> pagePoints, bool release = true)
{
    const std::vector<QPointF> points(pagePoints);
    presentationharness::sendTablet(w.window(), QEvent::TabletPress, scene(w, points.front()));
    for (std::size_t i = 1; i < points.size(); ++i) {
        for (int k = 1; k <= 4; ++k) {
            const QPointF at = points[i - 1] + (points[i] - points[i - 1]) * (k / 4.0);
            presentationharness::sendTablet(w.window(), QEvent::TabletMove, scene(w, at));
        }
    }
    if (release) {
        presentationharness::sendTablet(w.window(), QEvent::TabletRelease, scene(w, points.back()));
    }
    QCoreApplication::processEvents();
}

const actcraft::core::Stroke* lastStroke(const actcraft::core::Page& page)
{
    return page.objects.empty() ? nullptr : std::get_if<actcraft::core::Stroke>(&page.objects.back());
}

const actcraft::core::Stroke* lastStroke(const MainWindow& w)
{
    return lastStroke(w.fixedPage()->page());
}

bool allOnLine(const actcraft::core::Stroke& stroke, double y)
{
    for (const auto& point : stroke.points) {
        if (qAbs(point.y - y) > 0.5) {
            return false;
        }
    }
    return !stroke.points.empty();
}

class TstUiMathTools : public QObject
{
    Q_OBJECT

private slots:
    // Le stylo reçoit des échantillons déjà posés sur le bord (filtre de la vue, avant le Pipeline) ; la règle
    // fermée, le même trait garde ses y d'origine : le stylo lui-même n'a pas changé.
    void rulerGuidesThePenWithoutChangingIt()
    {
        MainWindow w;
        QVERIFY(w.window());
        QVERIFY(w.run(QStringLiteral("math.ruler")));
        MathToolsController* tools = mathTools(w);
        QVERIFY(tools);
        QCOMPARE(tools->count(), 1);
        const double top = frameOf(*tools, 0).top();
        const int before = w.pageObjectCount();
        stylusPath(w, {{300, top - 4}, {400, top - 1}, {500, top + 2}, {600, top + 3}, {700, top + 3}});
        QTRY_COMPARE(w.pageObjectCount(), before + 1);
        const actcraft::core::Stroke* guided = lastStroke(w);
        QVERIFY(guided);
        QVERIFY(guided->points.size() >= 2);
        QVERIFY2(allOnLine(*guided, top), "tous les points du trait guidé sur le bord haut");
        QVERIFY(qAbs(guided->points.front().x - 300) <= 0.5);
        QVERIFY(qAbs(guided->points.back().x - 700) <= 0.5);

        tools->close(0);
        QCOMPARE(tools->count(), 0);
        stylusPath(w, {{300, top - 4}, {400, top - 1}, {500, top + 2}, {600, top + 3}, {700, top + 3}});
        QTRY_COMPARE(w.pageObjectCount(), before + 2);
        const actcraft::core::Stroke* unguided = lastStroke(w);
        QVERIFY(unguided);
        QVERIFY2(qAbs(unguided->points.front().y - (top - 4)) <= 0.5, "règle fermée : y d'origine au départ");
        QVERIFY2(qAbs(unguided->points.back().y - (top + 3)) <= 0.5, "règle fermée : y d'origine à la fin");
    }

    // Trait guidé coupé par page.next avant le relâché : la vue synthétise le relâché au dernier point brut
    // (hors du bord) et le passe au même filtre ; le dernier point versé est sur le bord.
    void syntheticReleaseStaysOnTheGuide()
    {
        MainWindow w;
        QVERIFY(w.window());
        QVERIFY(w.run(QStringLiteral("math.ruler")));
        MathToolsController* tools = mathTools(w);
        QVERIFY(tools);
        actcraft::ui::DocumentController* doc = documentController(w);
        QVERIFY(doc);
        const double top = frameOf(*tools, 0).top();
        const std::size_t before = doc->document().page(0).objects.size();
        QCOMPARE(doc->currentIndex(), 0);
        stylusPath(w, {{300, top - 4}, {450, top - 1}, {600, top + 5}}, false);
        QVERIFY(w.run(QStringLiteral("page.next")));
        QTRY_COMPARE(doc->document().page(0).objects.size(), before + 1);
        const actcraft::core::Stroke* stroke = lastStroke(doc->document().page(0));
        QVERIFY(stroke);
        QVERIFY2(qAbs(stroke->points.back().y - top) <= 0.5, "dernier point (relâché synthétisé) sur le bord");
        QVERIFY(qAbs(stroke->points.back().x - 600) <= 0.5);
        QVERIFY(allOnLine(*stroke, top));
        QCOMPARE(tools->count(), 1); // par fenêtre : la règle reste en changeant de page
        presentationharness::sendTablet(w.window(), QEvent::TabletRelease, scene(w, QPointF(600, top + 5)));
        QTest::qWait(20);
        QCOMPARE(doc->document().page(0).objects.size(), before + 1);
    }

    // Ouvrir, déplacer (au stylet, par la règle elle-même) et fermer : empreinte du document inchangée.
    void mathToolIsNotSaved()
    {
        MainWindow w;
        QVERIFY(w.window());
        actcraft::ui::DocumentController* doc = documentController(w);
        QVERIFY(doc);
        const QString fingerprint = doc->property("fingerprint").toString();
        QVERIFY(!fingerprint.isEmpty());
        const int before = w.pageObjectCount();
        QVERIFY(w.run(QStringLiteral("math.ruler")));
        MathToolsController* tools = mathTools(w);
        QVERIFY(tools);
        const Frame f = frameOf(*tools, 0);
        presentationharness::strokeAt(w.window(), Pointer::Stylus, scene(w, QPointF(f.cx - 62, f.cy)),
                                      scene(w, QPointF(f.cx - 62, f.cy - 80)));
        QTRY_VERIFY(qAbs(frameOf(*tools, 0).cy - (f.cy - 80)) <= 2);
        QCOMPARE(doc->property("fingerprint").toString(), fingerprint);
        tools->close(0);
        QCOMPARE(doc->property("fingerprint").toString(), fingerprint);
        QCOMPARE(w.pageObjectCount(), before);
    }

    // P6 : fermer la règle laisse le trait guidé au document, sur son bord.
    void closingTheRulerKeepsTheStroke()
    {
        MainWindow w;
        QVERIFY(w.window());
        QVERIFY(w.run(QStringLiteral("math.ruler")));
        MathToolsController* tools = mathTools(w);
        QVERIFY(tools);
        const double top = frameOf(*tools, 0).top();
        const int before = w.pageObjectCount();
        stylusPath(w, {{300, top - 3}, {700, top + 2}});
        QTRY_COMPARE(w.pageObjectCount(), before + 1);
        tools->close(0);
        QTest::qWait(50);
        QCOMPARE(tools->count(), 0);
        QCOMPARE(w.pageObjectCount(), before + 1);
        const actcraft::core::Stroke* stroke = lastStroke(w);
        QVERIFY(stroke);
        QVERIFY(allOnLine(*stroke, top));
    }

    // Outils ouverts rangés dans l'état de l'utilisateur, par l'identifiant de leur commande.
    void openToolsKeptInTheUserState()
    {
        MainWindow w;
        QVERIFY(w.window());
        auto* host = w.window()->findChild<actcraft::ui::ToolHost*>(QStringLiteral("appTools"));
        QVERIFY(host);
        QVERIFY(host->user().openMathTools().empty());
        QVERIFY(w.run(QStringLiteral("math.ruler")));
        QCOMPARE(host->user().openMathTools().size(), std::size_t{1});
        QCOMPARE(host->user().openMathTools().front(), QStringLiteral("math.ruler"));
        QVERIFY(w.run(QStringLiteral("math.ruler")));
        QCOMPARE(host->user().openMathTools().size(), std::size_t{2});
        MathToolsController* tools = mathTools(w);
        QVERIFY(tools);
        tools->close(0);
        QCOMPARE(host->user().openMathTools().size(), std::size_t{1});
        tools->close(0);
        QVERIFY(host->user().openMathTools().empty());
    }

    // Translucidité du corps : kDefaultToolOpacity (100 sur 255, L7-T0), jamais opaque.
    void mathToolOpacityIsTheDefault()
    {
        MainWindow w;
        QVERIFY(w.window());
        QVERIFY(w.run(QStringLiteral("math.ruler")));
        QQuickItem* body = w.item(QStringLiteral("rulerBody"));
        QVERIFY(body);
        QCOMPARE(MathToolsController::defaultOpacity(), actcraft::features::kDefaultToolOpacity);
        QVERIFY(qAbs(body->opacity() - actcraft::features::kDefaultToolOpacity / 255.0) < 1e-6);
        QVERIFY(body->opacity() < 1.0);
    }

    // P21 : déplacer au centre, tourner à l'extrémité, allonger au bout ; ni dans la bande du bord guide, ni
    // hors de la règle.
    void mathToolCursorFollowsTheZone()
    {
        MainWindow w;
        QVERIFY(w.window());
        QVERIFY(w.run(QStringLiteral("math.ruler")));
        MathToolsController* tools = mathTools(w);
        QVERIFY(tools);
        const Frame f = frameOf(*tools, 0);
        QQuickWindow* window = w.window();
        // Deux déplacements : le second relit le curseur une fois le survol du premier livré aux gestionnaires.
        const auto hoverAt = [&](QPointF page) {
            QTest::mouseMove(window, scene(w, page).toPoint());
            QTest::mouseMove(window, scene(w, page).toPoint() + QPoint(1, 0));
        };
        hoverAt(QPointF(f.cx - 62, f.cy));
        QTRY_COMPARE(window->cursor().shape(), Qt::SizeAllCursor);
        hoverAt(QPointF(f.left() + 24 + 30, f.cy));
        QTRY_COMPARE(window->cursor().shape(), Qt::CrossCursor);
        hoverAt(QPointF(f.left() + 10, f.cy));
        QTRY_COMPARE(window->cursor().shape(), Qt::SizeHorCursor);
        hoverAt(QPointF(f.cx - 62, f.top() + 4)); // bande du bord guide : rien à saisir
        QTRY_VERIFY(window->cursor().shape() != Qt::SizeHorCursor);
        QVERIFY(window->cursor().shape() != Qt::SizeAllCursor);
        QVERIFY(window->cursor().shape() != Qt::CrossCursor);
        hoverAt(QPointF(f.cx - 62, f.top() - 60)); // hors de la règle
        QTRY_VERIFY(window->cursor().shape() != Qt::SizeAllCursor);
    }

    void rulerBlocksTheStylus_data() { presentationharness::addPointerRows(); }

    // Appui sur le corps, loin des bords guides : la règle bouge, rien n'est versé. Pendant : le même pointeur
    // dans la bande d'un bord, dehors puis dedans, verse un trait guidé.
    void rulerBlocksTheStylus()
    {
        QFETCH(Pointer, pointer);
        MainWindow w;
        QVERIFY(w.window());
        QVERIFY(w.run(QStringLiteral("math.ruler")));
        MathToolsController* tools = mathTools(w);
        QVERIFY(tools);
        const Frame f = frameOf(*tools, 0);
        const int before = w.pageObjectCount();
        presentationharness::strokeAt(w.window(), pointer, scene(w, QPointF(f.cx - 62, f.cy)),
                                      scene(w, QPointF(f.cx - 62, f.cy - 60)));
        QTRY_VERIFY2(qAbs(frameOf(*tools, 0).cy - (f.cy - 60)) <= 2, "règle déplacée par son gestionnaire");
        QTest::qWait(50);
        QCOMPARE(w.pageObjectCount(), before);
        QCOMPARE(w.pageView()->ink()->pointCount(), std::size_t{0});

        const Frame moved = frameOf(*tools, 0);
        presentationharness::strokeAt(w.window(), pointer, scene(w, QPointF(moved.cx - 200, moved.top() - 6)),
                                      scene(w, QPointF(moved.cx + 200, moved.top() - 6)));
        QTRY_COMPARE(w.pageObjectCount(), before + 1);
        const actcraft::core::Stroke* outside = lastStroke(w);
        QVERIFY(outside);
        QVERIFY2(allOnLine(*outside, moved.top()), "bande extérieure : trait versé et guidé");
        presentationharness::strokeAt(w.window(), pointer, scene(w, QPointF(moved.cx - 200, moved.top() + 6)),
                                      scene(w, QPointF(moved.cx + 200, moved.top() + 6)));
        QTRY_COMPARE(w.pageObjectCount(), before + 2);
        const actcraft::core::Stroke* inside = lastStroke(w);
        QVERIFY(inside);
        QVERIFY2(allOnLine(*inside, moved.top()), "bande intérieure (hors couverture) : trait versé et guidé");
    }
};

} // namespace

ACTCRAFT_LOT7_TEST("mathtools", TstUiMathTools)
#include "tst_ui_mathtools.moc"
