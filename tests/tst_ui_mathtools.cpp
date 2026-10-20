#include "mathtoolscontroller.h"
#include "support/lot7registry.h"
#include "support/presentationharness.h"

#include <actcraft/core/document.h>
#include <actcraft/core/edits.h>
#include <actcraft/core/model.h>
#include <actcraft/core/undo.h>
#include <actcraft/features/mathtools.h>
#include <actcraft/render/fixedpageitem.h>
#include <actcraft/render/inkitem.h>
#include <actcraft/tools/userstate.h>
#include <actcraft/ui/documentcontroller.h>
#include <actcraft/ui/pageview.h>
#include <actcraft/ui/toolhost.h>

#include <QColor>
#include <QCursor>
#include <QSignalSpy>
#include <QtMath>
#include <QtTest>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <initializer_list>
#include <memory>
#include <utility>
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
    return w.window() ? w.window()->findChild<actcraft::ui::DocumentController*>(
                            QStringLiteral("documentController"))
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

// Trait au stylet le long de `points` (repère de page), quatre déplacements par segment.
void stylusPath(const MainWindow& w, const std::vector<QPointF>& points, bool release = true)
{
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

void stylusPath(const MainWindow& w, std::initializer_list<QPointF> pagePoints, bool release = true)
{
    stylusPath(w, std::vector<QPointF>(pagePoints), release);
}

// Un temps d'un pointeur (appui, déplacement, relâché), injecté comme presentationharness::strokeAt.
void pointerStep(QQuickWindow* window, Pointer pointer, QEvent::Type phase, QPointF at)
{
    switch (pointer) {
    case Pointer::Stylus:
        presentationharness::sendTablet(window, phase, at);
        break;
    case Pointer::Finger:
        // Séquence temporaire : livrée à la fin de l'instruction, comme dans presentationharness::strokeAt.
        if (phase == QEvent::TabletPress) {
            QTest::touchEvent(window, presentationharness::touchDevice()).press(0, at.toPoint());
        } else if (phase == QEvent::TabletMove) {
            QTest::touchEvent(window, presentationharness::touchDevice()).move(0, at.toPoint());
        } else {
            QTest::touchEvent(window, presentationharness::touchDevice()).release(0, at.toPoint());
        }
        presentationharness::settleTouch();
        break;
    case Pointer::Mouse:
        if (phase == QEvent::TabletPress) {
            QTest::mousePress(window, Qt::LeftButton, Qt::NoModifier, at.toPoint());
        } else if (phase == QEvent::TabletMove) {
            QTest::mouseMove(window, at.toPoint());
        } else {
            QTest::mouseRelease(window, Qt::LeftButton, Qt::NoModifier, at.toPoint());
        }
        break;
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

// Équerre ouverte, lue du contrôleur : sommet de l'angle droit, côté (angle 0 : les côtés vont vers +x et
// +y).
struct Square
{
    double vx = 0;
    double vy = 0;
    double side = 0;
    double hypotenuse() const { return vx + vy + side; } // x + y le long de l'hypoténuse
};

Square squareOf(const MathToolsController& tools, int index)
{
    const QVariantMap m = tools.toolAt(index);
    return {m.value(QStringLiteral("centerX")).toDouble(), m.value(QStringLiteral("centerY")).toDouble(),
            m.value(QStringLiteral("length")).toDouble()};
}

bool allOnHypotenuse(const actcraft::core::Stroke& stroke, double sum)
{
    for (const auto& point : stroke.points) {
        if (qAbs(point.x + point.y - sum) > 0.5) {
            return false;
        }
    }
    return !stroke.points.empty();
}

bool allOnColumn(const actcraft::core::Stroke& stroke, double x)
{
    for (const auto& point : stroke.points) {
        if (qAbs(point.x - x) > 0.5) {
            return false;
        }
    }
    return !stroke.points.empty();
}

// Rapporteur ouvert, lu du contrôleur : centre, rayon, angle.
struct Dial
{
    double cx = 0;
    double cy = 0;
    double radius = 0;
    double angle = 0;
    QPointF centre() const { return {cx, cy}; }
};

Dial dialOf(const MathToolsController& tools, int index)
{
    const QVariantMap m = tools.toolAt(index);
    return {m.value(QStringLiteral("centerX")).toDouble(), m.value(QStringLiteral("centerY")).toDouble(),
            m.value(QStringLiteral("length")).toDouble(), m.value(QStringLiteral("angle")).toDouble()};
}

// Point de page à `degrees` (repère de page, y vers le bas) et à `fromRim` pixels du pourtour (positif :
// dehors).
QPointF onDial(const Dial& d, double degrees, double fromRim)
{
    const double rad = qDegreesToRadians(degrees);
    return {d.cx + (d.radius + fromRim) * std::cos(rad), d.cy + (d.radius + fromRim) * std::sin(rad)};
}

// Points de `from` à `to` degrés, un tous les 10°, à `fromRim` du pourtour.
std::vector<QPointF> rimPath(const Dial& d, double from, double to, double fromRim)
{
    std::vector<QPointF> points;
    const int steps = std::max(1, qRound(std::abs(to - from) / 10));
    for (int i = 0; i <= steps; ++i) {
        points.push_back(onDial(d, from + (to - from) * i / steps, fromRim));
    }
    return points;
}

bool onRim(double x, double y, const Dial& d)
{
    return qAbs(std::hypot(x - d.cx, y - d.cy) - d.radius) <= 0.5;
}

bool allOnRim(const actcraft::core::Stroke& stroke, const Dial& d)
{
    for (const auto& point : stroke.points) {
        if (!onRim(point.x, point.y, d)) {
            return false;
        }
    }
    return !stroke.points.empty();
}

bool samePoint(double x, double y, double ex, double ey)
{
    return qAbs(x - ex) <= 0.5 && qAbs(y - ey) <= 0.5;
}

const actcraft::core::Page& shownPage(const actcraft::ui::DocumentController& doc)
{
    return doc.document().page(static_cast<std::size_t>(doc.currentIndex()));
}

// Dernier objet du type T de la page (le plus haut), sinon nul.
template <typename T> const T* lastOf(const actcraft::core::Page& page)
{
    for (auto it = page.objects.rbegin(); it != page.objects.rend(); ++it) {
        if (const T* found = std::get_if<T>(&actcraft::core::body(*it))) {
            return found;
        }
    }
    return nullptr;
}

// Dernier objet du type T de la page, avec ses données communes (couche), sinon nul.
template <typename T> const actcraft::core::PageObject* lastObjectOf(const actcraft::core::Page& page)
{
    for (auto it = page.objects.rbegin(); it != page.objects.rend(); ++it) {
        if (std::get_if<T>(&actcraft::core::body(*it))) {
            return &*it;
        }
    }
    return nullptr;
}

// Passe un objet de la page sur une autre couche, au sommet de celle-ci (commande du banc : aucune commande
// du socle ne le fait, et commitStroke pose toujours le trait sur Top). Annoncé en ObjectChanged, comme un
// objet réordonné (fixedpageitem.cpp).
class MoveToLayer final : public actcraft::core::UndoCommand
{
public:
    MoveToLayer(std::size_t page, actcraft::core::ObjectId id, actcraft::core::Layer layer)
        : m_page(page), m_id(id), m_layer(layer)
    {}
    void redo(actcraft::core::Document& document, actcraft::core::Changes& changes) override
    {
        swapLayer(document, changes);
    }
    void undo(actcraft::core::Document& document, actcraft::core::Changes& changes) override
    {
        swapLayer(document, changes);
    }

private:
    void swapLayer(actcraft::core::Document& document, actcraft::core::Changes& changes)
    {
        actcraft::core::Page& page = document.page(m_page);
        const auto it =
            std::find_if(page.objects.begin(), page.objects.end(),
                         [this](const actcraft::core::PageObject& o) { return o.common.id == m_id; });
        if (it == page.objects.end()) {
            return;
        }
        actcraft::core::PageObject object = std::move(*it);
        page.objects.erase(it);
        std::swap(object.common.layer, m_layer);
        actcraft::core::insertAtTopOfLayer(page, std::move(object));
        changes.push_back({actcraft::core::Change::Kind::ObjectChanged, m_page, m_id, {}});
    }

    std::size_t m_page;
    actcraft::core::ObjectId m_id;
    actcraft::core::Layer m_layer;
};

template <typename T> int countOf(const actcraft::core::Page& page)
{
    int n = 0;
    for (const auto& object : page.objects) {
        if (std::get_if<T>(&actcraft::core::body(object))) {
            ++n;
        }
    }
    return n;
}

class TstUiMathTools : public QObject
{
    Q_OBJECT

private slots:
    // Le stylo reçoit des échantillons déjà posés sur le bord (filtre de la vue, avant le Pipeline) ; la
    // règle fermée, le même trait garde ses y d'origine : le stylo lui-même n'a pas changé.
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

        // Tolérance de 16 px d'écran ÷ zoom : à 12 px de page du bord, libre au zoom 2 (24 px d'écran), guidé
        // au zoom 0,5 (6 px d'écran).
        for (const qreal zoom : {2.0, 0.5}) {
            const QPointF anchor = w.pageView()->mapFromScene(scene(w, QPointF(500, top)));
            w.pageView()->zoomAround(anchor, zoom);
            QTRY_VERIFY(qAbs(w.pageView()->zoom() - zoom) < 1e-6);
            const int count = w.pageObjectCount();
            stylusPath(w, {{450, top - 12}, {550, top - 12}});
            QTRY_COMPARE(w.pageObjectCount(), count + 1);
            const actcraft::core::Stroke* atZoom = lastStroke(w);
            QVERIFY(atZoom);
            if (zoom > 1) {
                QVERIFY2(qAbs(atZoom->points.front().y - (top - 12)) <= 0.5,
                         "zoom 2 : 24 px d'écran, trait libre");
            } else {
                QVERIFY2(allOnLine(*atZoom, top), "zoom 0,5 : 6 px d'écran, trait guidé");
            }
        }

        tools->close(0);
        QCOMPARE(tools->count(), 0);
        stylusPath(w, {{300, top - 4}, {400, top - 1}, {500, top + 2}, {600, top + 3}, {700, top + 3}});
        QTRY_COMPARE(w.pageObjectCount(), before + 4);
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
        QVERIFY2(qAbs(stroke->points.back().y - top) <= 0.5,
                 "dernier point (relâché synthétisé) sur le bord");
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
        // Deux déplacements : le second relit le curseur une fois le survol du premier livré aux
        // gestionnaires.
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
        hoverAt(QPointF(f.cx - 62, f.top() - 4)); // bande extérieure du bord guide : rien à saisir
        QTRY_VERIFY(window->cursor().shape() != Qt::SizeHorCursor);
        QVERIFY(window->cursor().shape() != Qt::SizeAllCursor);
        QVERIFY(window->cursor().shape() != Qt::CrossCursor);
        // Le survol de la couche (cercle d'accroche) n'est pris que dans la bande d'un bord guide.
        QObject* snapHover = window->findChild<QObject*>(QStringLiteral("mathToolSnapHover"));
        QVERIFY(snapHover);
        QTRY_VERIFY2(snapHover->property("hovered").toBool(), "bande du bord guide : survol de la couche");
        hoverAt(QPointF(f.cx - 62, f.top() - 60)); // hors de la règle
        QTRY_VERIFY(window->cursor().shape() != Qt::SizeAllCursor);
        QTRY_VERIFY2(!snapHover->property("hovered").toBool(),
                     "hors de la règle : la couche ne prend pas le survol (surcouches de dessous, spot)");
    }

    void rulerBlocksTheStylus_data() { presentationharness::addPointerRows(); }

    // Appui sur le corps, loin des bords guides : la règle bouge, rien n'est versé. Pendant : le même
    // pointeur dans la bande d'un bord, dehors puis dedans, verse un trait guidé.
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
        // Un seul geste : encre lue d'abord sous le seuil de glisser (sans couverture, la vue prendrait
        // l'appui et le stylo dessinerait ; le gestionnaire de la règle ne la lui reprend qu'au-delà du
        // seuil), puis glisser jusqu'au bout.
        const QPointF from = scene(w, QPointF(f.cx - 62, f.cy));
        const QPointF to = scene(w, QPointF(f.cx - 62, f.cy - 60));
        pointerStep(w.window(), pointer, QEvent::TabletPress, from);
        pointerStep(w.window(), pointer, QEvent::TabletMove, from + QPointF(0, -2));
        QCOMPARE(w.pageView()->ink()->pointCount(), std::size_t{0});
        for (int k = 1; k <= 8; ++k) {
            pointerStep(w.window(), pointer, QEvent::TabletMove, from + (to - from) * (k / 8.0));
        }
        pointerStep(w.window(), pointer, QEvent::TabletRelease, to);
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
        // Bande intérieure : sur le corps, donc couverte (seule la bande extérieure est au stylo).
        presentationharness::strokeAt(w.window(), pointer, scene(w, QPointF(moved.cx - 200, moved.top() + 6)),
                                      scene(w, QPointF(moved.cx - 100, moved.top() + 6)));
        QTRY_VERIFY2(qAbs(frameOf(*tools, 0).cx - (moved.cx + 100)) <= 2,
                     "bande intérieure : la règle bouge");
        QTest::qWait(50);
        QCOMPARE(w.pageObjectCount(), before + 1);
        if (pointer == Pointer::Mouse) {
            // L'appui gauche de la souris sur la règle n'atteint pas la vue, couverture ou non (constaté sous
            // mutation) : le clic droit, que la vue teste par sa couverture (menu contextuel), discrimine.
            QSignalSpy menu(w.pageView(), &actcraft::ui::PageView::contextMenuRequested);
            const Frame now = frameOf(*tools, 0);
            QTest::mouseClick(w.window(), Qt::RightButton, Qt::NoModifier,
                              scene(w, QPointF(now.cx - 62, now.cy)).toPoint());
            QTest::qWait(50);
            QCOMPARE(menu.count(), 0);
            QTest::mouseClick(w.window(), Qt::RightButton, Qt::NoModifier,
                              scene(w, QPointF(now.cx - 62, now.top() - 40)).toPoint());
            QTRY_COMPARE(menu.count(), 1);
            QTest::keyClick(w.window(), Qt::Key_Escape);
        }
    }

    // Les trois bords de l'équerre guident le stylo (filtre de la vue) : l'hypoténuse et les deux côtés de
    // l'angle droit, du côté extérieur ; l'équerre fermée, le trait garde ses coordonnées d'origine.
    void setSquareGuidesThePen()
    {
        MainWindow w;
        QVERIFY(w.window());
        QVERIFY(w.run(QStringLiteral("math.set-square")));
        MathToolsController* tools = mathTools(w);
        QVERIFY(tools);
        QCOMPARE(tools->count(), 1);
        QCOMPARE(tools->toolAt(0).value(QStringLiteral("kind")).toString(), QStringLiteral("setSquare"));
        const Square q = squareOf(*tools, 0);
        QVERIFY(q.side > 100);
        const double c = q.hypotenuse();
        const int before = w.pageObjectCount();

        // Hypoténuse, côté extérieur (x + y > c) : 4 à 8 pixels de page de la droite.
        const auto beside = [&](double x, double outside) { return QPointF(x, c + outside - x); };
        const double x0 = q.vx + 0.3 * q.side;
        stylusPath(w, {beside(x0, 8), beside(x0 + 40, 5), beside(x0 + 80, 7), beside(x0 + 120, 4)});
        QTRY_COMPARE(w.pageObjectCount(), before + 1);
        const actcraft::core::Stroke* hypotenuse = lastStroke(w);
        QVERIFY(hypotenuse);
        QVERIFY2(allOnHypotenuse(*hypotenuse, c), "tous les points du trait sur l'hypoténuse");

        // Côté +x (au-dessus) puis côté +y (à gauche).
        stylusPath(w, {{q.vx + 40, q.vy - 5}, {q.vx + 140, q.vy - 2}, {q.vx + 240, q.vy - 6}});
        QTRY_COMPARE(w.pageObjectCount(), before + 2);
        QVERIFY2(allOnLine(*lastStroke(w), q.vy), "côté +x de l'angle droit");
        stylusPath(w, {{q.vx - 5, q.vy + 40}, {q.vx - 2, q.vy + 140}, {q.vx - 6, q.vy + 240}});
        QTRY_COMPARE(w.pageObjectCount(), before + 3);
        QVERIFY2(allOnColumn(*lastStroke(w), q.vx), "côté +y de l'angle droit");

        tools->close(0);
        QCOMPARE(tools->count(), 0);
        stylusPath(w, {beside(x0, 8), beside(x0 + 40, 5), beside(x0 + 80, 7), beside(x0 + 120, 4)});
        QTRY_COMPARE(w.pageObjectCount(), before + 4);
        const actcraft::core::Stroke* free = lastStroke(w);
        QVERIFY(free);
        QVERIFY2(qAbs(free->points.front().x + free->points.front().y - (c + 8)) <= 0.5,
                 "équerre fermée : coordonnées d'origine");
    }

    // Translucidité du corps : kDefaultToolOpacity (100 sur 255), jamais opaque.
    void setSquareOpacityIsTheDefault()
    {
        MainWindow w;
        QVERIFY(w.window());
        QVERIFY(w.run(QStringLiteral("math.set-square")));
        QQuickItem* body = w.item(QStringLiteral("setSquareBody"));
        QVERIFY(body);
        QVERIFY(qAbs(body->opacity() - actcraft::features::kDefaultToolOpacity / 255.0) < 1e-6);
        QVERIFY(body->opacity() < 1.0);
    }

    // Déplacer au centre, tourner aux pointes, redimensionner le long de l'hypoténuse ; ni dans la bande du
    // côté extérieur, ni dans l'angle de la boîte hors du triangle.
    void setSquareCursorFollowsTheZone()
    {
        MainWindow w;
        QVERIFY(w.window());
        QVERIFY(w.run(QStringLiteral("math.set-square")));
        MathToolsController* tools = mathTools(w);
        QVERIFY(tools);
        const Square q = squareOf(*tools, 0);
        QQuickWindow* window = w.window();
        const auto hoverAt = [&](double lx, double ly) {
            const QPointF page(q.vx + lx, q.vy + ly);
            QTest::mouseMove(window, scene(w, page).toPoint());
            QTest::mouseMove(window, scene(w, page).toPoint() + QPoint(1, 0));
        };
        hoverAt(60, 150);
        QTRY_COMPARE(window->cursor().shape(), Qt::SizeAllCursor);
        hoverAt(q.side - 40, 12);
        QTRY_COMPARE(window->cursor().shape(), Qt::CrossCursor);
        hoverAt(12, q.side - 40);
        QTRY_COMPARE(window->cursor().shape(), Qt::CrossCursor);
        hoverAt(q.side / 2 - 12, q.side / 2 - 12);
        QTRY_COMPARE(window->cursor().shape(), Qt::SizeFDiagCursor);
        hoverAt(q.side / 2, -4); // bande extérieure d'un côté guide : rien à saisir
        QTRY_VERIFY(window->cursor().shape() != Qt::SizeAllCursor);
        QVERIFY(window->cursor().shape() != Qt::CrossCursor);
        QVERIFY(window->cursor().shape() != Qt::SizeFDiagCursor);
        hoverAt(0.8 * q.side, 0.8 * q.side); // dans la boîte, hors du triangle
        QTRY_VERIFY(window->cursor().shape() != Qt::SizeAllCursor);
        QVERIFY(window->cursor().shape() != Qt::SizeFDiagCursor);
    }

    void setSquareBlocksTheStylus_data() { presentationharness::addPointerRows(); }

    // Appui sur le corps (le triangle) : l'équerre bouge, rien n'est versé. Pendant : le même pointeur sur la
    // bande extérieure d'un côté guide verse un trait guidé ; dans l'angle de la boîte hors du triangle, un
    // trait libre sans que l'équerre bouge.
    void setSquareBlocksTheStylus()
    {
        QFETCH(Pointer, pointer);
        MainWindow w;
        QVERIFY(w.window());
        QVERIFY(w.run(QStringLiteral("math.set-square")));
        MathToolsController* tools = mathTools(w);
        QVERIFY(tools);
        const Square q = squareOf(*tools, 0);
        const int before = w.pageObjectCount();
        // Un seul geste : encre lue d'abord sous le seuil de glisser, puis glisser jusqu'au bout.
        const QPointF from = scene(w, QPointF(q.vx + 60, q.vy + 150));
        const QPointF to = scene(w, QPointF(q.vx + 60, q.vy + 90));
        pointerStep(w.window(), pointer, QEvent::TabletPress, from);
        pointerStep(w.window(), pointer, QEvent::TabletMove, from + QPointF(0, -2));
        QCOMPARE(w.pageView()->ink()->pointCount(), std::size_t{0});
        for (int k = 1; k <= 8; ++k) {
            pointerStep(w.window(), pointer, QEvent::TabletMove, from + (to - from) * (k / 8.0));
        }
        pointerStep(w.window(), pointer, QEvent::TabletRelease, to);
        QTRY_VERIFY2(qAbs(squareOf(*tools, 0).vy - (q.vy - 60)) <= 2,
                     "équerre déplacée par son gestionnaire");
        QTest::qWait(50);
        QCOMPARE(w.pageObjectCount(), before);
        QCOMPARE(w.pageView()->ink()->pointCount(), std::size_t{0});

        const Square moved = squareOf(*tools, 0);
        presentationharness::strokeAt(w.window(), pointer, scene(w, QPointF(moved.vx + 60, moved.vy - 6)),
                                      scene(w, QPointF(moved.vx + 240, moved.vy - 6)));
        QTRY_COMPARE(w.pageObjectCount(), before + 1);
        const actcraft::core::Stroke* outside = lastStroke(w);
        QVERIFY(outside);
        QVERIFY2(allOnLine(*outside, moved.vy), "bande extérieure : trait versé et guidé");
        // Hors du triangle, dans la boîte : libre, l'équerre ne bouge pas.
        const double far = 0.8 * moved.side;
        presentationharness::strokeAt(w.window(), pointer,
                                      scene(w, QPointF(moved.vx + far - 30, moved.vy + far)),
                                      scene(w, QPointF(moved.vx + far + 30, moved.vy + far)));
        QTRY_COMPARE(w.pageObjectCount(), before + 2);
        const actcraft::core::Stroke* corner = lastStroke(w);
        QVERIFY(corner);
        QVERIFY2(allOnLine(*corner, moved.vy + far), "hors du triangle : trait libre, à ses coordonnées");
        QVERIFY(qAbs(squareOf(*tools, 0).vx - moved.vx) <= 0.5);
        QVERIFY(qAbs(squareOf(*tools, 0).vy - moved.vy) <= 0.5);
        // Bande intérieure d'un côté : sur le corps, donc couverte.
        presentationharness::strokeAt(w.window(), pointer, scene(w, QPointF(moved.vx + 60, moved.vy + 6)),
                                      scene(w, QPointF(moved.vx + 160, moved.vy + 6)));
        QTRY_VERIFY2(qAbs(squareOf(*tools, 0).vx - (moved.vx + 100)) <= 2,
                     "bande intérieure : l'équerre bouge");
        QTest::qWait(50);
        QCOMPARE(w.pageObjectCount(), before + 2);
        if (pointer == Pointer::Mouse) {
            // Comme pour la règle : le clic droit, que la vue teste par sa couverture, discrimine.
            QSignalSpy menu(w.pageView(), &actcraft::ui::PageView::contextMenuRequested);
            const Square now = squareOf(*tools, 0);
            QTest::mouseClick(w.window(), Qt::RightButton, Qt::NoModifier,
                              scene(w, QPointF(now.vx + 60, now.vy + 150)).toPoint());
            QTest::qWait(50);
            QCOMPARE(menu.count(), 0);
            QTest::mouseClick(w.window(), Qt::RightButton, Qt::NoModifier,
                              scene(w, QPointF(now.vx + far, now.vy + far)).toPoint());
            QTRY_COMPARE(menu.count(), 1);
            QTest::keyClick(w.window(), Qt::Key_Escape);
        }
    }

    // F-281, arc plein : le stylo verse son trait guidé tel quel (l'arc seul, sur le pourtour, à sa
    // couleur) ; le contrôleur ajoute ensuite une seule forme, le secteur rempli de la couleur du trait,
    // sans contour, sur la couche du trait, par DocumentInsertions. Le stylo n'est jamais modifié : aucun
    // point du trait au centre. Couche : le trait passe sur Bottom (ni Top, que commitStroke pose, ni Middle,
    // la couche par défaut d'un objet) entre son versement et l'insertion différée ; le secteur doit l'y
    // suivre.
    void filledArcBecomesOneShape()
    {
        MainWindow w;
        QVERIFY(w.window());
        w.pageView()->setInkColor(QColor(0x22, 0x66, 0xAA)); // pas le noir par défaut
        QVERIFY(w.run(QStringLiteral("math.protractor")));
        MathToolsController* tools = mathTools(w);
        QVERIFY(tools);
        actcraft::ui::DocumentController* doc = documentController(w);
        QVERIFY(doc);
        QCOMPARE(tools->count(), 1);
        QCOMPARE(tools->toolAt(0).value(QStringLiteral("kind")).toString(), QStringLiteral("protractor"));
        tools->setArcStyle(0, QStringLiteral("filled"));
        QCOMPARE(tools->toolAt(0).value(QStringLiteral("arcStyle")).toString(), QStringLiteral("filled"));
        const Dial d = dialOf(*tools, 0);
        const int before = w.pageObjectCount();
        const int shapesBefore = countOf<actcraft::core::Shape>(shownPage(*doc));
        const std::vector<QPointF> quarter = rimPath(d, -90, 0, 6); // haut → droite, bande extérieure
        stylusPath(w, quarter, false);
        presentationharness::sendTablet(w.window(), QEvent::TabletRelease, scene(w, quarter.back()));
        {
            // Le trait seul est versé : l'insertion du secteur est différée (contrat de addChangeObserver).
            const actcraft::core::Page& page = shownPage(*doc);
            QCOMPARE(countOf<actcraft::core::Shape>(page), shapesBefore);
            QVERIFY(!page.objects.empty());
            QVERIFY(std::get_if<actcraft::core::Stroke>(&actcraft::core::body(page.objects.back())));
            const actcraft::core::ObjectId drawn = page.objects.back().common.id;
            doc->execute(std::make_unique<MoveToLayer>(static_cast<std::size_t>(doc->currentIndex()), drawn,
                                                       actcraft::core::Layer::Bottom));
        }
        QTRY_COMPARE(w.pageObjectCount(), before + 2);
        QTest::qWait(50);
        QCOMPARE(w.pageObjectCount(), before + 2);
        const actcraft::core::Page& page = shownPage(*doc);
        QCOMPARE(countOf<actcraft::core::Shape>(page), shapesBefore + 1);
        const auto* stroke = lastOf<actcraft::core::Stroke>(page);
        QVERIFY(stroke);
        QVERIFY2(allOnRim(*stroke, d), "le trait du stylo est l'arc seul, sur le pourtour");
        QVERIFY2(stroke->color != 0xFF000000u, "le trait a la couleur de l'encre");
        const auto* sector = lastOf<actcraft::core::Shape>(page);
        QVERIFY(sector);
        QVERIFY(sector->kind == actcraft::core::ShapeKind::Path);
        QCOMPARE(sector->paths.size(), std::size_t{1});
        QVERIFY(!sector->open);
        QVERIFY(sector->brush.kind == actcraft::core::FillKind::Solid);
        QCOMPARE(sector->brush.color, stroke->color);
        QVERIFY2(sector->pen.style == actcraft::core::PenStyle::None, "secteur : aucun contour");
        QCOMPARE(sector->pen.width, 0.0);
        const actcraft::core::PageObject* strokeObject = lastObjectOf<actcraft::core::Stroke>(page);
        const actcraft::core::PageObject* sectorObject = lastObjectOf<actcraft::core::Shape>(page);
        QVERIFY(strokeObject && sectorObject);
        QVERIFY(strokeObject->common.layer == actcraft::core::Layer::Bottom);
        QVERIFY2(sectorObject->common.layer == strokeObject->common.layer,
                 "secteur : sur la couche du trait");
        const auto& outline = sector->paths.front();
        QVERIFY(outline.size() >= 4);
        QVERIFY2(samePoint(outline.front().x, outline.front().y, d.cx, d.cy) &&
                     samePoint(outline.back().x, outline.back().y, d.cx, d.cy),
                 "secteur : du centre au centre");
        for (std::size_t i = 1; i + 1 < outline.size(); ++i) {
            QVERIFY2(onRim(outline[i].x, outline[i].y, d), "secteur : son arc sur le pourtour");
        }
        QVERIFY2(samePoint(outline[1].x, outline[1].y, stroke->points.front().x, stroke->points.front().y),
                 "secteur : l'arc part du début du trait");
        const auto& end = outline[outline.size() - 2];
        QVERIFY2(samePoint(end.x, end.y, stroke->points.back().x, stroke->points.back().y),
                 "secteur : l'arc finit à la fin du trait");
        QCOMPARE(tools->toolAt(0).value(QStringLiteral("swept")).toInt(), 90);
    }

    // Arc fermé : le trait guidé, puis les deux rayons d'un seul objet, ajoutés en une étape après lui.
    // Une annulation retire les deux rayons, et seulement eux ; la suivante, le trait du stylo.
    void closedArcIsUndoneAtOnce()
    {
        MainWindow w;
        QVERIFY(w.window());
        QVERIFY(w.run(QStringLiteral("math.protractor")));
        MathToolsController* tools = mathTools(w);
        QVERIFY(tools);
        actcraft::ui::DocumentController* doc = documentController(w);
        QVERIFY(doc);
        w.pageView()->setInkWidth(
            7); // ni l'épaisseur par défaut de la vue, ni celle du stylo, ni celle d'un Line
        tools->setArcStyle(0, QStringLiteral("closed"));
        const Dial d = dialOf(*tools, 0);
        const int before = w.pageObjectCount();
        const int linesBefore = countOf<actcraft::core::Line>(shownPage(*doc));
        stylusPath(w, rimPath(d, -90, 0, 6));
        QTRY_COMPARE(w.pageObjectCount(), before + 2);
        QTest::qWait(50);
        QCOMPARE(w.pageObjectCount(), before + 2);
        {
            const actcraft::core::Page& page = shownPage(*doc);
            QCOMPARE(countOf<actcraft::core::Line>(page), linesBefore + 1);
            const auto* stroke = lastOf<actcraft::core::Stroke>(page);
            QVERIFY(stroke);
            QVERIFY2(allOnRim(*stroke, d), "le trait du stylo est l'arc seul");
            const auto* radii = lastOf<actcraft::core::Line>(page);
            QVERIFY(radii);
            QCOMPARE(radii->points.size(), std::size_t{3});
            QVERIFY2(samePoint(radii->points[0].x, radii->points[0].y, stroke->points.front().x,
                               stroke->points.front().y),
                     "premier rayon : au début de l'arc");
            QVERIFY2(samePoint(radii->points[1].x, radii->points[1].y, d.cx, d.cy),
                     "les rayons passent au centre");
            QVERIFY2(samePoint(radii->points[2].x, radii->points[2].y, stroke->points.back().x,
                               stroke->points.back().y),
                     "second rayon : à la fin de l'arc");
            QCOMPARE(radii->pen.color, stroke->color);
            QCOMPARE(stroke->baseWidth, 7.0);
            QCOMPARE(radii->pen.width, stroke->baseWidth);
        }
        doc->undo();
        QTRY_COMPARE(w.pageObjectCount(), before + 1);
        {
            const actcraft::core::Page& page = shownPage(*doc);
            QCOMPARE(countOf<actcraft::core::Line>(page), linesBefore);
            const auto* stroke = lastOf<actcraft::core::Stroke>(page);
            QVERIFY(stroke);
            QVERIFY2(allOnRim(*stroke, d), "une annulation : les deux rayons partent, l'arc reste");
        }
        doc->undo();
        QTRY_COMPARE(w.pageObjectCount(), before);
        doc->redo();
        doc->redo();
        QTRY_COMPARE(w.pageObjectCount(), before + 2);
        QCOMPARE(countOf<actcraft::core::Line>(shownPage(*doc)), linesBefore + 1);
    }

    // P21 : glisser la poignée d'un quart de tour autour du centre tourne le rapporteur de 90° ; le
    // centre ne bouge pas, rien n'est versé.
    void protractorTurnsByItsHandle()
    {
        MainWindow w;
        QVERIFY(w.window());
        QVERIFY(w.run(QStringLiteral("math.protractor")));
        MathToolsController* tools = mathTools(w);
        QVERIFY(tools);
        QQuickItem* item = w.item(QStringLiteral("protractorTool"));
        QQuickItem* handle = w.item(QStringLiteral("protractorHandle"));
        QVERIFY(item);
        QVERIFY(handle);
        QVERIFY(qAbs(item->rotation()) < 1e-6);
        const Dial d = dialOf(*tools, 0);
        const int before = w.pageObjectCount();
        const QPointF c = scene(w, d.centre());
        const QPointF h = handle->mapToScene(QPointF(handle->width() / 2, handle->height() / 2));
        const double reach = std::hypot(h.x() - c.x(), h.y() - c.y());
        const double a0 = std::atan2(h.y() - c.y(), h.x() - c.x());
        QVERIFY(reach > 20);
        pointerStep(w.window(), Pointer::Stylus, QEvent::TabletPress, h);
        QPointF last = h;
        for (int k = 1; k <= 12; ++k) {
            const double a = a0 + qDegreesToRadians(90.0) * k / 12;
            last = c + QPointF(reach * std::cos(a), reach * std::sin(a));
            pointerStep(w.window(), Pointer::Stylus, QEvent::TabletMove, last);
        }
        pointerStep(w.window(), Pointer::Stylus, QEvent::TabletRelease, last);
        QTRY_VERIFY2(qAbs(item->rotation() - 90) <= 1, "rotation lue 90 ± 1");
        const Dial turned = dialOf(*tools, 0);
        QVERIFY(qAbs(turned.angle - 90) <= 1);
        QVERIFY2(samePoint(turned.cx, turned.cy, d.cx, d.cy), "le centre ne bouge pas");
        QTest::qWait(50);
        QCOMPARE(w.pageObjectCount(), before);
        QCOMPARE(w.pageView()->ink()->pointCount(), std::size_t{0});
    }

    // Translucidité du corps : kDefaultToolOpacity (100 sur 255), jamais opaque.
    void protractorOpacityIsTheDefault()
    {
        MainWindow w;
        QVERIFY(w.window());
        QVERIFY(w.run(QStringLiteral("math.protractor")));
        QQuickItem* body = w.item(QStringLiteral("protractorBody"));
        QVERIFY(body);
        QVERIFY(qAbs(body->opacity() - actcraft::features::kDefaultToolOpacity / 255.0) < 1e-6);
        QVERIFY(body->opacity() < 1.0);
    }

    // Déplacer sur le disque, tourner à la poignée, changer le rayon le long du pourtour intérieur ; ni
    // dans la bande extérieure du pourtour (au stylo), ni hors du disque.
    void protractorCursorFollowsTheZone()
    {
        MainWindow w;
        QVERIFY(w.window());
        QVERIFY(w.run(QStringLiteral("math.protractor")));
        MathToolsController* tools = mathTools(w);
        QVERIFY(tools);
        QQuickItem* handle = w.item(QStringLiteral("protractorHandle"));
        QVERIFY(handle);
        const Dial d = dialOf(*tools, 0);
        QQuickWindow* window = w.window();
        const auto hoverScene = [&](QPointF at) {
            QTest::mouseMove(window, at.toPoint());
            QTest::mouseMove(window, at.toPoint() + QPoint(1, 0));
        };
        const auto hoverAt = [&](QPointF page) { hoverScene(scene(w, page)); };
        hoverAt(QPointF(d.cx - d.radius / 2, d.cy + 20));
        QTRY_COMPARE(window->cursor().shape(), Qt::SizeAllCursor);
        hoverScene(handle->mapToScene(QPointF(handle->width() / 2, handle->height() / 2)));
        QTRY_COMPARE(window->cursor().shape(), Qt::CrossCursor);
        hoverAt(onDial(d, 180, -6));
        QTRY_COMPARE(window->cursor().shape(), Qt::SizeFDiagCursor);
        hoverAt(onDial(d, 180, 6)); // bande extérieure du pourtour : rien à saisir
        QTRY_VERIFY(window->cursor().shape() != Qt::SizeFDiagCursor);
        QVERIFY(window->cursor().shape() != Qt::SizeAllCursor);
        QVERIFY(window->cursor().shape() != Qt::CrossCursor);
        hoverAt(onDial(d, 180, 60)); // hors du rapporteur
        QTRY_VERIFY(window->cursor().shape() != Qt::SizeAllCursor);
        QVERIFY(window->cursor().shape() != Qt::SizeFDiagCursor);
    }

    void protractorBlocksTheStylus_data() { presentationharness::addPointerRows(); }

    // Appui sur le corps (le disque) : le rapporteur bouge, rien n'est versé. Pendant : le même pointeur
    // dans la bande extérieure du pourtour verse un arc guidé ; dans la bande intérieure, le rayon change
    // et rien n'est versé.
    void protractorBlocksTheStylus()
    {
        QFETCH(Pointer, pointer);
        MainWindow w;
        QVERIFY(w.window());
        QVERIFY(w.run(QStringLiteral("math.protractor")));
        MathToolsController* tools = mathTools(w);
        QVERIFY(tools);
        const Dial d = dialOf(*tools, 0);
        const int before = w.pageObjectCount();
        // Un seul geste : encre lue d'abord sous le seuil de glisser, puis glisser jusqu'au bout.
        const QPointF from = scene(w, QPointF(d.cx - d.radius / 2, d.cy + 20));
        const QPointF to = scene(w, QPointF(d.cx - d.radius / 2, d.cy - 40));
        pointerStep(w.window(), pointer, QEvent::TabletPress, from);
        pointerStep(w.window(), pointer, QEvent::TabletMove, from + QPointF(0, -2));
        QCOMPARE(w.pageView()->ink()->pointCount(), std::size_t{0});
        for (int k = 1; k <= 8; ++k) {
            pointerStep(w.window(), pointer, QEvent::TabletMove, from + (to - from) * (k / 8.0));
        }
        pointerStep(w.window(), pointer, QEvent::TabletRelease, to);
        QTRY_VERIFY2(qAbs(dialOf(*tools, 0).cy - (d.cy - 60)) <= 2,
                     "rapporteur déplacé par son gestionnaire");
        QTest::qWait(50);
        QCOMPARE(w.pageObjectCount(), before);
        QCOMPARE(w.pageView()->ink()->pointCount(), std::size_t{0});

        const Dial moved = dialOf(*tools, 0);
        presentationharness::strokeAt(w.window(), pointer, scene(w, onDial(moved, -100, 6)),
                                      scene(w, onDial(moved, -80, 6)));
        QTRY_COMPARE(w.pageObjectCount(), before + 1);
        const actcraft::core::Stroke* outside = lastStroke(w);
        QVERIFY(outside);
        QVERIFY2(allOnRim(*outside, moved), "bande extérieure : arc versé et guidé");
        // Bande intérieure : sur le corps, donc couverte ; le pourtour suit le pointeur.
        presentationharness::strokeAt(w.window(), pointer, scene(w, onDial(moved, 180, -8)),
                                      scene(w, onDial(moved, 180, 32)));
        QTRY_VERIFY2(qAbs(dialOf(*tools, 0).radius - (moved.radius + 40)) <= 2,
                     "bande intérieure : le rayon change");
        QTest::qWait(50);
        QCOMPARE(w.pageObjectCount(), before + 1);
        if (pointer == Pointer::Mouse) {
            // Comme pour la règle : le clic droit, que la vue teste par sa couverture, discrimine.
            QSignalSpy menu(w.pageView(), &actcraft::ui::PageView::contextMenuRequested);
            const Dial now = dialOf(*tools, 0);
            QTest::mouseClick(w.window(), Qt::RightButton, Qt::NoModifier,
                              scene(w, QPointF(now.cx - now.radius / 2, now.cy + 20)).toPoint());
            QTest::qWait(50);
            QCOMPARE(menu.count(), 0);
            QTest::mouseClick(w.window(), Qt::RightButton, Qt::NoModifier,
                              scene(w, onDial(now, 0, 60)).toPoint());
            QTRY_COMPARE(menu.count(), 1);
            QTest::keyClick(w.window(), Qt::Key_Escape);
        }
    }
};

} // namespace

ACTCRAFT_LOT7_TEST("mathtools", TstUiMathTools)
#include "tst_ui_mathtools.moc"
