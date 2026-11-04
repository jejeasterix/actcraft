#include "mathtoolscontroller.h"
#include "presentationdocument.h"
#include "support/lot7registry.h"
#include "support/presentationharness.h"

#include <actcraft/core/document.h>
#include <actcraft/core/edits.h>
#include <actcraft/core/model.h>
#include <actcraft/core/undo.h>
#include <actcraft/features/mathtools.h>
#include <actcraft/render/fixedpageitem.h>
#include <actcraft/render/inkitem.h>
#include <actcraft/tools/selectiongeometry.h>
#include <actcraft/tools/userstate.h>
#include <actcraft/ui/commands.h>
#include <actcraft/ui/documentcontroller.h>
#include <actcraft/ui/pageview.h>
#include <actcraft/ui/toolhost.h>

#include <QColor>
#include <QCursor>
#include <QLineF>
#include <QQuickItem>
#include <QSignalSpy>
#include <QUrl>
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

// Menu d'un outil ouvert : objet non visuel, enfant direct de l'élément de l'outil. Cherché en suivant l'arbre
// visuel (les délégués du Repeater de la couche n'ont pas la fenêtre pour ancêtre dans l'arbre des objets).
QObject* menuIn(QQuickItem* item, const QString& name)
{
    if (!item) {
        return nullptr;
    }
    if (QObject* found = item->findChild<QObject*>(name, Qt::FindDirectChildrenOnly)) {
        return found;
    }
    const QList<QQuickItem*> children = item->childItems();
    for (QQuickItem* child : children) {
        if (QObject* found = menuIn(child, name)) {
            return found;
        }
    }
    return nullptr;
}

QObject* menuOf(const MainWindow& w, const QString& name)
{
    return w.window() ? menuIn(w.window()->contentItem(), name) : nullptr;
}

QQuickItem* menuEntry(QObject* menu, const QString& text)
{
    const int count = menu ? menu->property("count").toInt() : 0;
    for (int i = 0; i < count; ++i) {
        QQuickItem* item = nullptr;
        QMetaObject::invokeMethod(menu, "itemAt", Q_RETURN_ARG(QQuickItem*, item), Q_ARG(int, i));
        if (item && item->property("text").toString() == text) {
            return item;
        }
    }
    return nullptr;
}

// Clic de souris sur l'entrée `text` du menu, une fois ouvert (une entrée à sous-menu ouvre celui-ci).
bool clickEntry(const MainWindow& w, QObject* menu, const QString& text)
{
    if (!menu || !QTest::qWaitFor([menu] { return menu->property("opened").toBool(); }, 3000)) {
        return false;
    }
    QQuickItem* entry = menuEntry(menu, text);
    if (!entry) {
        return false;
    }
    // Menu dans sa propre fenêtre si le style en ouvre une ; sinon dans la couche de la fenêtre principale.
    QQuickWindow* target = entry->window() ? entry->window() : w.window();
    QTest::mouseClick(target, Qt::LeftButton, Qt::NoModifier,
                      entry->mapToScene(QPointF(entry->width() / 2, entry->height() / 2)).toPoint());
    QCoreApplication::processEvents();
    return true;
}

// Le menu ouvert, l'entrée cliquée comme à la main ; le menu se referme.
bool chooseFromMenu(const MainWindow& w, QObject* menu, const QString& text)
{
    if (!menu || !QMetaObject::invokeMethod(menu, "open") || !clickEntry(w, menu, text)) {
        return false;
    }
    return QTest::qWaitFor([menu] { return !menu->property("visible").toBool(); }, 3000);
}

bool entryChecked(QObject* menu, const QString& text)
{
    QQuickItem* entry = menuEntry(menu, text);
    return entry && entry->property("checked").toBool();
}

QVariant toolValue(const MathToolsController& tools, const char* key)
{
    return tools.toolAt(0).value(QLatin1String(key));
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

// Compas ouvert, lu du contrôleur : pointe, rayon, longueur des bras, angle du bras, mine.
struct Comp
{
    double cx = 0;
    double cy = 0;
    double radius = 0;
    double leg = 0;
    double angle = 0;
    double leadX = 0;
    double leadY = 0;
    QPointF pivot() const { return {cx, cy}; }
    QPointF lead() const { return {leadX, leadY}; }
    // Repère du compas (pointe à l'origine, x vers la mine, y vers le bas à l'angle 0) → page.
    QPointF at(double lx, double ly) const
    {
        const double rad = qDegreesToRadians(angle);
        return {cx + lx * std::cos(rad) - ly * std::sin(rad), cy + lx * std::sin(rad) + ly * std::cos(rad)};
    }
    double hingeHeight() const { return std::sqrt(std::max(leg * leg - radius * radius / 4, 0.0)); }
    // Point du bras de la mine à `fromLead` pixels de page de la mine, vers la charnière (non réfléchi).
    QPointF onPencilArm(double fromLead) const
    {
        const QPointF tip(radius, 0);
        const QPointF toHinge = QPointF(radius / 2, -hingeHeight()) - tip;
        const QPointF local = tip + toHinge * (fromLead / std::hypot(toHinge.x(), toHinge.y()));
        return at(local.x(), local.y());
    }
    // Milieu du bras de la pointe (déplacer).
    QPointF onNeedleArm() const { return at(radius / 4, -hingeHeight() / 2); }
    // Point du cercle de la mine à `degrees` (repère de page), à `fromCircle` pixels du cercle.
    QPointF around(double degrees, double fromCircle) const
    {
        const double rad = qDegreesToRadians(degrees);
        return {cx + (radius + fromCircle) * std::cos(rad), cy + (radius + fromCircle) * std::sin(rad)};
    }
};

Comp compassOf(const MathToolsController& tools, int index)
{
    const QVariantMap m = tools.toolAt(index);
    return {m.value(QStringLiteral("centerX")).toDouble(), m.value(QStringLiteral("centerY")).toDouble(),
            m.value(QStringLiteral("length")).toDouble(),  m.value(QStringLiteral("thickness")).toDouble(),
            m.value(QStringLiteral("angle")).toDouble(),   m.value(QStringLiteral("leadX")).toDouble(),
            m.value(QStringLiteral("leadY")).toDouble()};
}

// Points de `from` à `to` degrés autour de la pointe, un tous les 10°, à `fromCircle` du cercle de la mine.
std::vector<QPointF> circlePath(const Comp& c, double from, double to, double fromCircle)
{
    std::vector<QPointF> points;
    const int steps = std::max(1, qRound(std::abs(to - from) / 10));
    for (int i = 0; i <= steps; ++i) {
        points.push_back(c.around(from + (to - from) * i / steps, fromCircle));
    }
    return points;
}

bool allOnCircle(const actcraft::core::Stroke& stroke, const Comp& c)
{
    for (const auto& point : stroke.points) {
        if (qAbs(std::hypot(point.x - c.cx, point.y - c.cy) - c.radius) > 0.5) {
            return false;
        }
    }
    return !stroke.points.empty();
}

// Survol au stylet : aucun bouton, aucune pression. Qt (QGuiApplication) envoie l'événement de tablette
// non accepté ; si la fenêtre ne l'accepte pas, il le rejoue en souris
// (Qt::AA_SynthesizeMouseForUnhandledTabletEvents, actif par défaut). Ce rejeu est celui de Qt : le banc le
// refait à la même condition (hors écran, l'événement de tablette seul n'atteint pas les HoverHandler,
// constaté). Faux si la fenêtre a pris le survol : Qt ne rejouerait rien, un vrai stylet ne montrerait rien.
bool stylusHover(QQuickWindow* window, QPointF at)
{
    QTabletEvent event(QEvent::TabletMove, presentationharness::stylusDevice(), at, window->mapToGlobal(at),
                       0.0, 0, 0, 0, 0, 0, Qt::NoModifier, Qt::NoButton, Qt::NoButton);
    event.setAccepted(false); // comme QGuiApplication avant l'envoi
    QCoreApplication::sendEvent(window, &event);
    if (event.isAccepted()) {
        return false;
    }
    QTest::mouseMove(window, at.toPoint());
    QCoreApplication::processEvents();
    return true;
}

// Opacité rendue d'un élément dans son délégué : la sienne et celles de ses parents, jusqu'à la racine du
// délégué (`root`) comprise.
double renderedOpacity(const QQuickItem* item, const QString& root)
{
    double opacity = 1;
    for (; item; item = item->parentItem()) {
        opacity *= item->opacity();
        if (item->objectName() == root) {
            break;
        }
    }
    return opacity;
}

const actcraft::core::PageObject* objectWithId(const actcraft::core::Page& page, actcraft::core::ObjectId id)
{
    const auto it =
        std::find_if(page.objects.begin(), page.objects.end(),
                     [id](const actcraft::core::PageObject& object) { return object.common.id == id; });
    return it == page.objects.end() ? nullptr : &*it;
}

// `p` tourné de `degrees` autour de `pivot`, repère de page (y vers le bas : le sens de atan2).
QPointF turnedAbout(QPointF pivot, QPointF p, double degrees)
{
    const double rad = qDegreesToRadians(degrees);
    const QPointF v = p - pivot;
    return pivot + QPointF(v.x() * std::cos(rad) - v.y() * std::sin(rad),
                           v.x() * std::sin(rad) + v.y() * std::cos(rad));
}

// Cadre de la sélection tel que l'outil Sélectionner le pose : la barre d'objet lue comme l'outil la lit
// (Commands::objectBar, via ViewInfoAdapter::objectBarCommands), pas une copie de layout.json, que le lot 5 a
// rendue fausse (5 → 11 boutons : la poignée « Tourner » visée n'était plus sous le stylet).
actcraft::tools::SelectionView selectionLayout(const MainWindow& w, actcraft::ui::DocumentController& doc,
                                               actcraft::ui::ToolHost& host)
{
    const auto* commands =
        w.window() ? w.window()->findChild<actcraft::ui::Commands*>(QStringLiteral("commands")) : nullptr;
    QStringList bar;
    for (const QVariant& entry : commands ? commands->objectBar() : QVariantList()) {
        bar.push_back(entry.toMap().value(QStringLiteral("id")).toString());
    }
    return actcraft::tools::layoutSelection(doc.document().page(static_cast<std::size_t>(doc.currentIndex())),
                                            host.user().selection(), w.pageView()->zoom(), bar);
}

QPointF frameCentre(const actcraft::tools::SelectionView& view)
{
    return {(view.corners[0].x + view.corners[2].x) / 2, (view.corners[0].y + view.corners[2].y) / 2};
}

QPointF rotateGrip(const actcraft::tools::SelectionView& view)
{
    for (const auto& spot : view.bar) {
        if (spot.command == QStringLiteral("object.rotate")) {
            return {spot.center.x, spot.center.y};
        }
    }
    return {-1e9, -1e9};
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
        // Lot 3 : le curseur du stylo est la croix, celui d'une zone de rotation aussi ; avec la sélection
        // (flèche), le retour au curseur de l'outil hors des zones reste discriminant.
        QVERIFY(w.run(QStringLiteral("tool.select")));
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
        // Lot 3 : le curseur du stylo est la croix, celui d'une zone de rotation aussi ; avec la sélection
        // (flèche), le retour au curseur de l'outil hors des zones reste discriminant.
        QVERIFY(w.run(QStringLiteral("tool.select")));
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
        // Lot 3 : l'outil tient l'encre de la vue (ToolHost la réécrit) ; la couleur se règle sur lui.
        QVERIFY(w.run(QStringLiteral("pen.color"), QStringLiteral("#2266aa"))); // pas le noir par défaut
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
        // Lot 3 : l'épaisseur se règle sur l'outil (ToolHost réécrit celle de la vue) ; 7 n'est ni le défaut
        // du stylo ni celle d'un Line.
        QVERIFY(w.run(QStringLiteral("pen.width"), 7));
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
        // Lot 3 : le curseur du stylo est la croix, celui d'une zone de rotation aussi ; avec la sélection
        // (flèche), le retour au curseur de l'outil hors des zones reste discriminant.
        QVERIFY(w.run(QStringLiteral("tool.select")));
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

    // F-282 : le stylo posé près de la mine trace le cercle de la mine (filtre de la vue, le stylo inchangé)
    // ; au-delà d'un tour, le trait se ferme exactement sur son premier point. Le compas fermé, le même geste
    // est libre.
    void compassStrokeIsACircle()
    {
        MainWindow w;
        QVERIFY(w.window());
        QVERIFY(w.run(QStringLiteral("math.compass")));
        MathToolsController* tools = mathTools(w);
        QVERIFY(tools);
        QCOMPARE(tools->count(), 1);
        QCOMPARE(tools->toolAt(0).value(QStringLiteral("kind")).toString(), QStringLiteral("compass"));
        QVERIFY(w.item(QStringLiteral("compassTool")));
        const Comp c = compassOf(*tools, 0);
        QVERIFY(c.radius > 100);
        QVERIFY2(samePoint(c.leadX, c.leadY, c.cx + c.radius, c.cy),
                 "angle 0 : la mine à droite de la pointe");
        const int before = w.pageObjectCount();
        const std::vector<QPointF> turn = circlePath(c, 0, 400, 5); // plus d'un tour, à 5 px du cercle
        stylusPath(w, turn);
        QTRY_COMPARE(w.pageObjectCount(), before + 1);
        const actcraft::core::Stroke* circle = lastStroke(w);
        QVERIFY(circle);
        QVERIFY(circle->points.size() >= 8);
        QVERIFY2(allOnCircle(*circle, c), "tous les points du trait sur le cercle de la mine");
        QVERIFY2(samePoint(circle->points.back().x, circle->points.back().y, circle->points.front().x,
                           circle->points.front().y),
                 "un tour : le trait se ferme sur son premier point");
        QVERIFY2(samePoint(circle->points.front().x, circle->points.front().y, c.leadX, c.leadY),
                 "le trait part de la mine");

        tools->close(0);
        QCOMPARE(tools->count(), 0);
        stylusPath(w, turn);
        QTRY_COMPARE(w.pageObjectCount(), before + 2);
        const actcraft::core::Stroke* free = lastStroke(w);
        QVERIFY(free);
        QVERIFY2(qAbs(std::hypot(free->points.front().x - c.cx, free->points.front().y - c.cy) -
                      (c.radius + 5)) <= 0.5,
                 "compas fermé : coordonnées d'origine");
    }

    // P5 : le stylet qui survole le porte-mine fait apparaître la mine, à la couleur de l'encre du stylo,
    // celle que la vue donne au trait (PageView::inkColor, réglée par « pen.color ») ; elle suit ce réglage.
    // Elle s'éteint quand le stylet s'éloigne, et sur le guide d'un autre outil (la règle ouverte après le
    // compas, à l'index 1).
    void compassLeadTakesThePenColour()
    {
        MainWindow w;
        QVERIFY(w.window());
        const QColor red(Qt::red);
        QVERIFY(w.run(QStringLiteral("pen.color"), red.name())); // argument « #rrggbb »
        QCOMPARE(w.pageView()->inkColor(), red);
        QVERIFY(w.run(QStringLiteral("math.compass")));
        MathToolsController* tools = mathTools(w);
        QVERIFY(tools);
        QQuickItem* lead = w.item(QStringLiteral("compassLead"));
        QVERIFY(lead);
        const Comp c = compassOf(*tools, 0);
        // La règle, au-dessus de la charnière, loin du compas.
        QVERIFY(w.run(QStringLiteral("math.ruler")));
        QCOMPARE(tools->count(), 2);
        tools->setFrame(1, QPointF(c.cx - 150, c.cy - c.hingeHeight() - 120), 600, 80, 0);
        const Frame ruler = frameOf(*tools, 1);
        QQuickItem* snapMarker = w.item(QStringLiteral("mathToolSnap"));
        QVERIFY(snapMarker);
        const auto hover = [&](QPointF page) {
            return stylusHover(w.window(), scene(w, page)) &&
                   stylusHover(w.window(), scene(w, page + QPointF(1, 0)));
        };

        QVERIFY2(hover(c.lead() + QPointF(2, 1)), "survol au stylet non accepté : Qt le rejoue en souris");
        QTRY_VERIFY2(lead->isVisible(), "stylet près du porte-mine : la mine apparaît");
        QCOMPARE(lead->property("color").value<QColor>(), red);
        QVERIFY2(qAbs(renderedOpacity(lead, QStringLiteral("compassTool")) - 1.0) < 1e-6,
                 "la mine n'est pas voilée par le corps translucide");
        const QPointF shown = lead->mapToScene(QPointF(lead->width() / 2, lead->height() / 2));
        const QPointF tip = scene(w, c.lead());
        QVERIFY2(std::hypot(shown.x() - tip.x(), shown.y() - tip.y()) <= 1.5, "la mine est au bout du bras");
        QVERIFY(hover(QPointF(ruler.cx, ruler.top() - 4)));
        QTRY_VERIFY2(snapMarker->isVisible(), "bord de la règle : cercle d'accroche");
        QTRY_VERIFY2(!lead->isVisible(), "bord de la règle : la mine du compas reste éteinte");
        QVERIFY(hover(c.lead() + QPointF(3, 1)));
        QTRY_VERIFY(lead->isVisible());
        QVERIFY(hover(c.at(-120, 120)));
        QTRY_VERIFY2(!lead->isVisible(), "le stylet s'éloigne : la mine disparaît");

        // Le trait versé depuis la mine a la couleur que la mine montrait.
        const int before = w.pageObjectCount();
        stylusPath(w, circlePath(c, 0, -60, 4));
        QTRY_COMPARE(w.pageObjectCount(), before + 1);
        const actcraft::core::Stroke* stroke = lastStroke(w);
        QVERIFY(stroke);
        QVERIFY(allOnCircle(*stroke, c));
        QCOMPARE(QColor::fromRgba(stroke->color), red);

        // La mine suit le réglage du stylo, pas la couleur de l'ouverture ; le trait suivant aussi.
        const QColor blue(Qt::blue);
        QVERIFY(w.run(QStringLiteral("pen.color"), blue.name()));
        QTRY_COMPARE(lead->property("color").value<QColor>(), blue);
        const Comp turned = compassOf(*tools, 0);
        stylusPath(w, circlePath(turned, turned.angle, turned.angle - 30, 4));
        QTRY_COMPARE(w.pageObjectCount(), before + 2);
        const actcraft::core::Stroke* second = lastStroke(w);
        QVERIFY(second);
        QVERIFY(allOnCircle(*second, turned));
        QCOMPARE(QColor::fromRgba(second->color), blue);
    }

    // F-280, L7-T34 : le marqueur du lot 3 suit le bord de la règle comme le stylo (même filtre
    // d'échantillons, en amont de l'outil actif) ; le marqueur lui-même ne sait pas qu'une règle existe.
    void markerFollowsTheRuler()
    {
        MainWindow w;
        QVERIFY(w.window());
        QVERIFY(w.run(QStringLiteral("tool.highlighter")));
        QVERIFY(w.run(QStringLiteral("math.ruler")));
        MathToolsController* tools = mathTools(w);
        QVERIFY(tools);
        QVERIFY(tools->toolHost());
        QCOMPARE(tools->toolHost()->currentTool(), QStringLiteral("highlighter"));
        const double top = frameOf(*tools, 0).top();
        const int before = w.pageObjectCount();
        stylusPath(w, {{300, top - 4}, {400, top - 1}, {500, top + 2}, {600, top + 3}, {700, top + 3}});
        QTRY_COMPARE(w.pageObjectCount(), before + 1);
        const actcraft::core::Stroke* guided = lastStroke(w);
        QVERIFY(guided);
        QVERIFY(guided->points.size() >= 2);
        QVERIFY2(allOnLine(*guided, top), "marqueur : tous les points du trait sur le bord haut");
        QVERIFY(qAbs(guided->points.front().x - 300) <= 0.5);
        QVERIFY(qAbs(guided->points.back().x - 700) <= 0.5);
    }

    // Relecture du lot 7 : un test par menu d'outil de maths, chaque entrée cliquée à la souris. Règle :
    // angle affiché, sous-menus des unités présents, fermeture.
    void rulerMenu()
    {
        MainWindow w;
        QVERIFY(w.window());
        QVERIFY(w.run(QStringLiteral("math.ruler")));
        MathToolsController* tools = mathTools(w);
        QVERIFY(tools);
        QObject* menu = nullptr; // délégué de l'outil créé par la couche, après la commande
        QTRY_VERIFY((menu = menuOf(w, QStringLiteral("rulerMenu"))) != nullptr);
        const bool angle = toolValue(*tools, "showAngle").toBool();
        QVERIFY(chooseFromMenu(w, menu, QStringLiteral("Afficher l'angle")));
        QTRY_COMPARE(toolValue(*tools, "showAngle").toBool(), !angle);
        QCOMPARE(entryChecked(menu, QStringLiteral("Afficher l'angle")), !angle);
        // Sous-menus des unités : leur présence seulement (cliquer « Pouces » dans le sous-menu ouvert fait
        // planter l'exécutable hors écran, le 2026-11-04 ; consigné, à analyser hors de L7-T37).
        QVERIFY(menuEntry(menu, QStringLiteral("Unités du bord haut")));
        QVERIFY(menuEntry(menu, QStringLiteral("Unités du bord bas")));
        QVERIFY(chooseFromMenu(w, menu, QStringLiteral("Fermer")));
        QTRY_COMPARE(tools->count(), 0);
    }

    // Équerre : angle affiché, fermeture.
    void setSquareMenu()
    {
        MainWindow w;
        QVERIFY(w.window());
        QVERIFY(w.run(QStringLiteral("math.set-square")));
        MathToolsController* tools = mathTools(w);
        QVERIFY(tools);
        QObject* menu = nullptr; // délégué de l'outil créé par la couche, après la commande
        QTRY_VERIFY((menu = menuOf(w, QStringLiteral("setSquareMenu"))) != nullptr);
        const bool angle = toolValue(*tools, "showAngle").toBool();
        QVERIFY(chooseFromMenu(w, menu, QStringLiteral("Afficher l'angle")));
        QTRY_COMPARE(toolValue(*tools, "showAngle").toBool(), !angle);
        QCOMPARE(entryChecked(menu, QStringLiteral("Afficher l'angle")), !angle);
        QVERIFY(chooseFromMenu(w, menu, QStringLiteral("Fermer")));
        QTRY_COMPARE(tools->count(), 0);
    }

    // Compas : chaque réglage cochable bascule sa valeur, et sa coche suit ; fermeture.
    void compassMenu()
    {
        MainWindow w;
        QVERIFY(w.window());
        QVERIFY(w.run(QStringLiteral("math.compass")));
        MathToolsController* tools = mathTools(w);
        QVERIFY(tools);
        QObject* menu = nullptr; // délégué de l'outil créé par la couche, après la commande
        QTRY_VERIFY((menu = menuOf(w, QStringLiteral("compassMenu"))) != nullptr);
        const std::pair<const char*, const char*> entries[] = {{"Réfléchi", "mirrored"},
                                                               {"Afficher l'angle", "showAngle"},
                                                               {"Afficher le rayon", "showRadius"},
                                                               {"Marquer le centre", "markCentre"}};
        for (const auto& [text, key] : entries) {
            const QString entry = QString::fromUtf8(text);
            const bool before = toolValue(*tools, key).toBool();
            QVERIFY2(chooseFromMenu(w, menu, entry), text);
            QTRY_COMPARE(toolValue(*tools, key).toBool(), !before);
            QCOMPARE(entryChecked(menu, entry), !before);
        }
        QVERIFY(chooseFromMenu(w, menu, QStringLiteral("Fermer")));
        QTRY_COMPARE(tools->count(), 0);
    }

    // Rapporteur : coches de la forme et du style d'arc. Cliquer l'entrée déjà cochée la laisse cochée ; cocher
    // l'autre décoche la première (le cadre relu après chaque réglage rétablit les liaisons `checked`, sans
    // groupe exclusif : vert aussi sans ButtonGroup, vu le 2026-11-04).
    void protractorMenu()
    {
        MainWindow w;
        QVERIFY(w.window());
        QVERIFY(w.run(QStringLiteral("math.protractor")));
        MathToolsController* tools = mathTools(w);
        QVERIFY(tools);
        QObject* menu = nullptr; // délégué de l'outil créé par la couche, après la commande
        QTRY_VERIFY((menu = menuOf(w, QStringLiteral("protractorMenu"))) != nullptr);
        const QString full = QStringLiteral("Rapporteur complet");
        const QString half = QStringLiteral("Demi-rapporteur");
        QVERIFY(toolValue(*tools, "shape").toString() != QLatin1String("half"));
        QVERIFY(entryChecked(menu, full));
        QVERIFY(chooseFromMenu(w, menu, full));
        QVERIFY(entryChecked(menu, full));
        QVERIFY(toolValue(*tools, "shape").toString() != QLatin1String("half"));
        QVERIFY(chooseFromMenu(w, menu, half));
        QTRY_COMPARE(toolValue(*tools, "shape").toString(), QStringLiteral("half"));
        QVERIFY(entryChecked(menu, half));
        QVERIFY(!entryChecked(menu, full));

        const QString filled = QStringLiteral("Arc plein");
        QVERIFY(chooseFromMenu(w, menu, filled));
        QTRY_COMPARE(toolValue(*tools, "arcStyle").toString(), QStringLiteral("filled"));
        QVERIFY(chooseFromMenu(w, menu, filled));
        QVERIFY(entryChecked(menu, filled));
        QVERIFY(!entryChecked(menu, QStringLiteral("Arc")));
        QVERIFY(!entryChecked(menu, QStringLiteral("Arc fermé")));
        QCOMPARE(toolValue(*tools, "arcStyle").toString(), QStringLiteral("filled"));

        QVERIFY(chooseFromMenu(w, menu, QStringLiteral("Fermer")));
        QTRY_COMPARE(tools->count(), 0);
    }

    // F-282, L7-T34 : la gomme du lot 3 suit la mine du compas (même filtre) : le bras tourne avec elle. Elle
    // ne verse aucun trait : « marquer le centre » n'ajoute aucune croix, ni à son relâché, ni au trait libre
    // que le stylo verse ensuite.
    void eraserFollowsTheCompass()
    {
        MainWindow w;
        QVERIFY(w.window());
        actcraft::ui::DocumentController* doc = documentController(w);
        QVERIFY(doc);
        QVERIFY(w.run(QStringLiteral("math.compass")));
        MathToolsController* tools = mathTools(w);
        QVERIFY(tools);
        tools->setMarkCentre(0, true);
        const Comp c = compassOf(*tools, 0);
        QVERIFY(qAbs(c.angle) < 1e-6);
        QVERIFY(w.run(QStringLiteral("tool.eraser")));
        QVERIFY(tools->toolHost());
        QCOMPARE(tools->toolHost()->currentTool(), QStringLiteral("eraser"));
        const int lines = countOf<actcraft::core::Line>(doc->document().page(0));
        stylusPath(w, circlePath(c, 0, 60, 5));
        QTRY_VERIFY2(qAbs(compassOf(*tools, 0).angle - 60) <= 1, "gomme guidée : le bras suit jusqu'à 60°");
        QTest::qWait(50);
        QCOMPARE(countOf<actcraft::core::Line>(doc->document().page(0)), lines);

        // Trait libre au stylo, loin du compas : il ne reçoit pas la croix restée armée par la gomme.
        QVERIFY(w.run(QStringLiteral("tool.pen")));
        const int before = w.pageObjectCount();
        stylusPath(w, {c.at(-60, 260), c.at(40, 260)});
        QTRY_COMPARE(w.pageObjectCount(), before + 1);
        QTest::qWait(50);
        QCOMPARE(countOf<actcraft::core::Line>(doc->document().page(0)), lines);
        QCOMPARE(w.pageObjectCount(), before + 1);
    }

    // Report de L7-T33, arbitré par l'original (extras/mathtools.py:967-970, _ink_style : couleur de l'outil
    // d'encre, translucidité du marqueur rendue à part) : sous le marqueur, la mine prend sa couleur, opaque,
    // pas PageView::inkColor (qui porte l'alpha du marqueur, toolhost.cpp:578-583).
    void compassLeadTakesTheMarkerColour()
    {
        MainWindow w;
        QVERIFY(w.window());
        QVERIFY(w.run(QStringLiteral("tool.highlighter")));
        const QColor marker(0x22, 0x66, 0xaa);
        QVERIFY(w.run(QStringLiteral("pen.color"), marker.name()));
        QVERIFY(w.run(QStringLiteral("math.compass")));
        MathToolsController* tools = mathTools(w);
        QVERIFY(tools);
        QVERIFY(tools->toolHost());
        QCOMPARE(tools->toolHost()->currentTool(), QStringLiteral("highlighter"));
        QVERIFY2(w.pageView()->inkColor().alpha() < 255, "la vue porte l'alpha du marqueur");
        QQuickItem* lead = w.item(QStringLiteral("compassLead"));
        QVERIFY(lead);
        const Comp c = compassOf(*tools, 0);
        QVERIFY(stylusHover(w.window(), scene(w, c.lead() + QPointF(2, 1))));
        QVERIFY(stylusHover(w.window(), scene(w, c.lead() + QPointF(3, 1))));
        QTRY_VERIFY2(lead->isVisible(), "stylet près du porte-mine : la mine apparaît");
        QCOMPARE(lead->property("color").value<QColor>(), marker);
        // Le trait du marqueur versé depuis la mine : même couleur (sa translucidité est à part).
        const int before = w.pageObjectCount();
        stylusPath(w, circlePath(c, 0, -60, 4));
        QTRY_COMPARE(w.pageObjectCount(), before + 1);
        const actcraft::core::Stroke* stroke = lastStroke(w);
        QVERIFY(stroke);
        QVERIFY(allOnCircle(*stroke, c));
        QCOMPARE(QColor::fromRgba(stroke->color).rgb(), marker.rgb());
    }

    // F-284, L7-T34 : l'origine XY montrée, le bouton de rotation de la barre d'objet (lot 3) fait tourner la
    // sélection autour de l'origine, non du centre de son cadre ; l'origine retirée, il retrouve ce centre.
    void rotationPivotsOnTheOrigin()
    {
        MainWindow w;
        QVERIFY(w.window());
        actcraft::ui::DocumentController* doc = documentController(w);
        QVERIFY(doc);
        MathToolsController* tools = mathTools(w);
        QVERIFY(tools);
        actcraft::ui::ToolHost* host = tools->toolHost();
        QVERIFY(host);
        const int before = w.pageObjectCount();
        stylusPath(w, {{600, 350}, {700, 350}, {700, 410}, {600, 410}, {600, 352}});
        QTRY_COMPARE(w.pageObjectCount(), before + 1);
        QVERIFY(w.run(QStringLiteral("math.xy-origin")));
        QVERIFY(tools->originVisible());
        const QPointF pivot(500, 300);
        QVERIFY(tools->moveOrigin(pivot));
        QTRY_COMPARE(tools->origin(), pivot);
        QVERIFY(w.run(QStringLiteral("tool.select")));
        QVERIFY(w.run(QStringLiteral("edit.select-all")));
        QCOMPARE(host->user().selection().size(), std::size_t(1));

        // Glisser le bouton de rotation de 90° autour de `about`, par pas de 15°.
        const auto rotateBy = [&](QPointF about) {
            const QPointF grip = rotateGrip(selectionLayout(w, *doc, *host));
            QVERIFY(grip.x() > -1e8);
            std::vector<QPointF> path;
            for (int turn = 0; turn <= 90; turn += 15) {
                path.push_back(turnedAbout(about, grip, turn));
            }
            stylusPath(w, path);
        };
        const QPointF c0 = frameCentre(selectionLayout(w, *doc, *host));
        const QPointF expected = turnedAbout(pivot, c0, 90);
        rotateBy(pivot);
        QTRY_VERIFY2(QLineF(frameCentre(selectionLayout(w, *doc, *host)), expected).length() <= 1,
                     "origine montrée : le centre du cadre tourne autour de l'origine");
        QCOMPARE(host->user().selection().size(), std::size_t(1));

        // Origine retirée : la rotation se fait autour du centre du cadre, qui ne bouge pas.
        QVERIFY(w.run(QStringLiteral("math.xy-origin")));
        QVERIFY(!tools->originVisible());
        const actcraft::tools::SelectionView turned = selectionLayout(w, *doc, *host);
        const QPointF c1 = frameCentre(turned);
        const QPointF corner(turned.corners[0].x, turned.corners[0].y);
        rotateBy(c1);
        QTRY_VERIFY2(QLineF(QPointF(selectionLayout(w, *doc, *host).corners[0].x,
                                    selectionLayout(w, *doc, *host).corners[0].y),
                            corner)
                             .length() > 20,
                     "le cadre a tourné");
        QVERIFY2(QLineF(frameCentre(selectionLayout(w, *doc, *host)), c1).length() <= 1,
                 "sans origine : autour du centre du cadre");
    }

    // Revue de L7-T9 : avec l'outil Sélectionner, la règle ne guide pas. Un cadre de sélection tiré depuis
    // le bord garde ses coordonnées et prend le trait placé au-dessus de la règle ; guidé, il serait plat
    // (y = bord) et ne prendrait rien.
    void selectionIsNotGuided()
    {
        MainWindow w;
        QVERIFY(w.window());
        QVERIFY(w.run(QStringLiteral("math.ruler")));
        MathToolsController* tools = mathTools(w);
        QVERIFY(tools);
        QVERIFY(tools->toolHost());
        const Frame f = frameOf(*tools, 0);
        const double top = f.top();
        // Règle écartée le temps de verser le trait libre, puis remise.
        tools->setFrame(0, QPointF(f.cx, f.cy + 300), f.length, f.thickness, 0);
        const int before = w.pageObjectCount();
        stylusPath(w, {{400, top - 16}, {450, top - 16}, {500, top - 16}});
        QTRY_COMPARE(w.pageObjectCount(), before + 1);
        tools->setFrame(0, QPointF(f.cx, f.cy), f.length, f.thickness, 0);
        QVERIFY(qAbs(frameOf(*tools, 0).top() - top) < 1e-6);
        QVERIFY(w.run(QStringLiteral("tool.select")));
        QCOMPARE(tools->toolHost()->currentTool(), QStringLiteral("select"));
        QCOMPARE(tools->toolHost()->selectionCount(), 0);
        stylusPath(w, {{300, top - 3}, {500, top - 20}, {700, top - 30}});
        QTRY_COMPARE(tools->toolHost()->selectionCount(), 1);
        QCOMPARE(w.pageObjectCount(), before + 1);
    }

    // Translucidité du corps : kDefaultToolOpacity (100 sur 255), jamais opaque.
    void compassOpacityIsTheDefault()
    {
        MainWindow w;
        QVERIFY(w.window());
        QVERIFY(w.run(QStringLiteral("math.compass")));
        QQuickItem* body = w.item(QStringLiteral("compassBody"));
        QVERIFY(body);
        QVERIFY(qAbs(body->opacity() - actcraft::features::kDefaultToolOpacity / 255.0) < 1e-6);
        QVERIFY(body->opacity() < 1.0);
    }

    // P21 : déplacer par le bras de la pointe, tourner par le bras de la mine, changer le rayon par le
    // porte-mine ; près de la mine, rien à saisir (le stylo s'y pose) ; hors du compas, rien.
    void compassCursorFollowsTheZone()
    {
        MainWindow w;
        QVERIFY(w.window());
        QVERIFY(w.run(QStringLiteral("math.compass")));
        // Lot 3 : le curseur du stylo est la croix, celui d'une zone de rotation aussi ; avec la sélection
        // (flèche), le retour au curseur de l'outil hors des zones reste discriminant.
        QVERIFY(w.run(QStringLiteral("tool.select")));
        MathToolsController* tools = mathTools(w);
        QVERIFY(tools);
        const Comp c = compassOf(*tools, 0);
        const double band = tools->guideBand();
        QVERIFY2(band < 50, "rayon de capture plus court que le porte-mine (60 px)");
        QQuickWindow* window = w.window();
        const auto hoverAt = [&](QPointF page) {
            QTest::mouseMove(window, scene(w, page).toPoint());
            QTest::mouseMove(window, scene(w, page).toPoint() + QPoint(1, 0));
        };
        hoverAt(c.onNeedleArm());
        QTRY_COMPARE(window->cursor().shape(), Qt::SizeAllCursor);
        hoverAt(c.onPencilArm(c.leg / 2));
        QTRY_COMPARE(window->cursor().shape(), Qt::CrossCursor);
        hoverAt(c.at(c.radius / 2, -c.hingeHeight())); // la charnière déplace, comme le bras de la pointe
        QTRY_COMPARE(window->cursor().shape(), Qt::SizeAllCursor);
        hoverAt(c.onPencilArm(c.leg / 2));
        QTRY_COMPARE(window->cursor().shape(), Qt::CrossCursor);
        hoverAt(c.onPencilArm((band + 60) / 2));
        QTRY_COMPARE(window->cursor().shape(), Qt::SizeFDiagCursor);
        hoverAt(c.lead() + QPointF(2, 0)); // la mine : au stylo
        QTRY_VERIFY(window->cursor().shape() != Qt::SizeFDiagCursor);
        QVERIFY(window->cursor().shape() != Qt::SizeAllCursor);
        QVERIFY(window->cursor().shape() != Qt::CrossCursor);
        QObject* snapHover = window->findChild<QObject*>(QStringLiteral("mathToolSnapHover"));
        QVERIFY(snapHover);
        QTRY_VERIFY2(snapHover->property("hovered").toBool(), "près de la mine : survol de la couche");
        hoverAt(c.at(-120, 120)); // hors du compas
        QTRY_VERIFY(window->cursor().shape() != Qt::SizeAllCursor);
        QVERIFY(window->cursor().shape() != Qt::SizeFDiagCursor);
        QTRY_VERIFY2(!snapHover->property("hovered").toBool(),
                     "hors du compas : la couche ne prend pas le survol");
    }

    void compassBlocksTheStylus_data() { presentationharness::addPointerRows(); }

    // Appui sur le bras de la pointe : le compas bouge, rien n'est versé. Pendant : le même pointeur posé
    // près de la mine verse un trait guidé sur le cercle.
    void compassBlocksTheStylus()
    {
        QFETCH(Pointer, pointer);
        MainWindow w;
        QVERIFY(w.window());
        QVERIFY(w.run(QStringLiteral("math.compass")));
        MathToolsController* tools = mathTools(w);
        QVERIFY(tools);
        const Comp c = compassOf(*tools, 0);
        const int before = w.pageObjectCount();
        // Un seul geste : encre lue d'abord sous le seuil de glisser, puis glisser jusqu'au bout.
        const QPointF from = scene(w, c.onNeedleArm());
        const QPointF to = scene(w, c.onNeedleArm() + QPointF(0, -60));
        pointerStep(w.window(), pointer, QEvent::TabletPress, from);
        pointerStep(w.window(), pointer, QEvent::TabletMove, from + QPointF(0, -2));
        QCOMPARE(w.pageView()->ink()->pointCount(), std::size_t{0});
        for (int k = 1; k <= 8; ++k) {
            pointerStep(w.window(), pointer, QEvent::TabletMove, from + (to - from) * (k / 8.0));
        }
        pointerStep(w.window(), pointer, QEvent::TabletRelease, to);
        QTRY_VERIFY2(qAbs(compassOf(*tools, 0).cy - (c.cy - 60)) <= 2, "compas déplacé par son gestionnaire");
        QTest::qWait(50);
        QCOMPARE(w.pageObjectCount(), before);
        QCOMPARE(w.pageView()->ink()->pointCount(), std::size_t{0});

        const Comp moved = compassOf(*tools, 0);
        presentationharness::strokeAt(w.window(), pointer, scene(w, moved.lead() + QPointF(3, 1)),
                                      scene(w, moved.around(-20, 4)));
        QTRY_COMPARE(w.pageObjectCount(), before + 1);
        const actcraft::core::Stroke* guided = lastStroke(w);
        QVERIFY(guided);
        QVERIFY2(allOnCircle(*guided, moved), "près de la mine : trait versé et guidé");
        if (pointer == Pointer::Mouse) {
            // Comme pour la règle : le clic droit, que la vue teste par sa couverture, discrimine.
            QSignalSpy menu(w.pageView(), &actcraft::ui::PageView::contextMenuRequested);
            const Comp now = compassOf(*tools, 0);
            QTest::mouseClick(w.window(), Qt::RightButton, Qt::NoModifier,
                              scene(w, now.onNeedleArm()).toPoint());
            QTest::qWait(50);
            QCOMPARE(menu.count(), 0);
            QTest::mouseClick(w.window(), Qt::RightButton, Qt::NoModifier,
                              scene(w, now.at(-120, 120)).toPoint());
            QTRY_COMPARE(menu.count(), 1);
            QTest::keyClick(w.window(), Qt::Key_Escape);
        }
    }

    // « Marquer le centre » (bascule, comme la référence) : après chaque trait guidé par la mine, une croix
    // de 12 pixels sur la pointe, un seul core::Line, ajouté par DocumentInsertions en une étape après le
    // trait, à sa couleur, sur sa couche. Le stylo n'est pas modifié : son trait reste sur le cercle. Couche
    // : le trait passe sur Bottom entre son versement et l'insertion différée ; la croix doit l'y suivre.
    void markCentreAddsACross()
    {
        MainWindow w;
        QVERIFY(w.window());
        // Lot 3 : l'outil tient l'encre de la vue (ToolHost la réécrit) ; la couleur se règle sur lui.
        QVERIFY(w.run(QStringLiteral("pen.color"), QStringLiteral("#2266aa"))); // pas le noir par défaut
        QVERIFY(w.run(QStringLiteral("pen.width"), 7));
        QVERIFY(w.run(QStringLiteral("math.compass")));
        MathToolsController* tools = mathTools(w);
        QVERIFY(tools);
        actcraft::ui::DocumentController* doc = documentController(w);
        QVERIFY(doc);
        const int linesBefore = countOf<actcraft::core::Line>(shownPage(*doc));
        const int before = w.pageObjectCount();

        // Centre non marqué (défaut) : le trait seul.
        QCOMPARE(tools->toolAt(0).value(QStringLiteral("markCentre")).toBool(), false);
        stylusPath(w, circlePath(compassOf(*tools, 0), 0, -90, 5));
        QTRY_COMPARE(w.pageObjectCount(), before + 1);
        QTest::qWait(50);
        QCOMPARE(w.pageObjectCount(), before + 1);
        QCOMPARE(countOf<actcraft::core::Line>(shownPage(*doc)), linesBefore);

        tools->setMarkCentre(0, true);
        QCOMPARE(tools->toolAt(0).value(QStringLiteral("markCentre")).toBool(), true);
        // Un trait que le compas ne tient pas : pas de croix.
        const Comp c = compassOf(*tools, 0);
        stylusPath(w, {c.at(-200, 150), c.at(-120, 150)});
        QTRY_COMPARE(w.pageObjectCount(), before + 2);
        QTest::qWait(50);
        QCOMPARE(w.pageObjectCount(), before + 2);

        const std::vector<QPointF> arc = circlePath(c, c.angle, c.angle + 120, 5);
        stylusPath(w, arc, false);
        presentationharness::sendTablet(w.window(), QEvent::TabletRelease, scene(w, arc.back()));
        actcraft::core::ObjectId drawn = 0;
        {
            // Le trait seul est versé : la croix est différée (contrat de addChangeObserver).
            const actcraft::core::Page& page = shownPage(*doc);
            QCOMPARE(countOf<actcraft::core::Line>(page), linesBefore);
            QVERIFY(!page.objects.empty());
            QVERIFY(std::get_if<actcraft::core::Stroke>(&actcraft::core::body(page.objects.back())));
            drawn = page.objects.back().common.id;
            doc->execute(std::make_unique<MoveToLayer>(static_cast<std::size_t>(doc->currentIndex()), drawn,
                                                       actcraft::core::Layer::Bottom));
        }
        QTRY_COMPARE(w.pageObjectCount(), before + 4);
        QTest::qWait(50);
        QCOMPARE(w.pageObjectCount(), before + 4);
        {
            const actcraft::core::Page& page = shownPage(*doc);
            QCOMPARE(countOf<actcraft::core::Line>(page), linesBefore + 1);
            const actcraft::core::PageObject* strokeObject = objectWithId(page, drawn);
            const actcraft::core::PageObject* crossObject = lastObjectOf<actcraft::core::Line>(page);
            QVERIFY(strokeObject && crossObject);
            const auto* stroke = std::get_if<actcraft::core::Stroke>(&actcraft::core::body(*strokeObject));
            const auto* cross = std::get_if<actcraft::core::Line>(&actcraft::core::body(*crossObject));
            QVERIFY(stroke && cross);
            QVERIFY2(allOnCircle(*stroke, c),
                     "le trait du stylo reste sur le cercle (aucun point au centre)");
            QVERIFY(cross->points.size() >= 4);
            double minX = cross->points.front().x;
            double maxX = minX;
            double minY = cross->points.front().y;
            double maxY = minY;
            bool throughCentre = false;
            for (const auto& point : cross->points) {
                minX = std::min(minX, point.x);
                maxX = std::max(maxX, point.x);
                minY = std::min(minY, point.y);
                maxY = std::max(maxY, point.y);
                throughCentre = throughCentre || samePoint(point.x, point.y, c.cx, c.cy);
            }
            QVERIFY2(throughCentre, "la croix passe par la pointe");
            QVERIFY2(qAbs(minX - (c.cx - 6)) <= 0.5 && qAbs(maxX - (c.cx + 6)) <= 0.5 &&
                         qAbs(minY - (c.cy - 6)) <= 0.5 && qAbs(maxY - (c.cy + 6)) <= 0.5,
                     "croix de 12 pixels centrée sur la pointe");
            QCOMPARE(cross->pen.color, stroke->color);
            QCOMPARE(stroke->baseWidth, 7.0);
            QCOMPARE(cross->pen.width, 3.5);
            QVERIFY(strokeObject->common.layer == actcraft::core::Layer::Bottom);
            QVERIFY2(crossObject->common.layer == strokeObject->common.layer,
                     "croix : sur la couche du trait");
        }
        doc->undo();
        QTRY_COMPARE(w.pageObjectCount(), before + 3);
        QCOMPARE(countOf<actcraft::core::Line>(shownPage(*doc)), linesBefore);
        QVERIFY2(lastStroke(w), "une annulation : la croix part, le trait reste");
        doc->undo(); // couche du trait (commande du banc)
        doc->undo(); // le trait
        QTRY_COMPARE(w.pageObjectCount(), before + 2);
        doc->redo();
        doc->redo();
        doc->redo();
        QTRY_COMPARE(w.pageObjectCount(), before + 4);
        QCOMPARE(countOf<actcraft::core::Line>(shownPage(*doc)), linesBefore + 1);
    }

    // F-284 : la commande cochable montre ou retire le marqueur ; déplacé au stylet, il écrit l'origine dans
    // la présentation de la page affichée (les autres réglages de la page sont gardés) ; une autre page garde
    // la sienne (défaut : centre) ; revenue sur la page, l'origine est relue du document.
    void originSavedWithThePage()
    {
        MainWindow w;
        QVERIFY(w.window());
        MathToolsController* tools = mathTools(w);
        actcraft::ui::DocumentController* doc = documentController(w);
        QVERIFY(tools && doc);
        QCOMPARE(tools->originVisible(), false);
        QVERIFY(w.run(QStringLiteral("math.xy-origin")));
        QCOMPARE(tools->originVisible(), true);
        QQuickItem* marker = w.item(QStringLiteral("originMarker"));
        QVERIFY(marker);
        QVERIFY(marker->isVisible());
        const QPointF centre = tools->origin();

        doc->nextPage();
        QTRY_COMPARE(doc->currentIndex(), 1);
        actcraft::core::PagePresentation settings;
        settings.curtainDistance = 77;
        actcraft::ui::DocumentInsertions(*doc).setPagePresentation(settings);
        QTRY_COMPARE(tools->origin(), centre);
        QVERIFY(!doc->document().page(1).presentation.hasOrigin);

        const QPointF target = centre + QPointF(120, 80);
        stylusPath(w, {tools->origin(), target});
        QTRY_VERIFY(doc->document().page(1).presentation.hasOrigin);
        const actcraft::core::PagePresentation& written = doc->document().page(1).presentation;
        QVERIFY(samePoint(written.origin.x, written.origin.y, target.x(), target.y()));
        QCOMPARE(written.curtainDistance, 77.0);
        QVERIFY(!doc->document().page(0).presentation.hasOrigin);
        const QPointF moved = tools->origin();
        QVERIFY(samePoint(moved.x(), moved.y(), target.x(), target.y()));

        doc->pageBack();
        QTRY_COMPARE(doc->currentIndex(), 0);
        QTRY_COMPARE(tools->origin(), centre);
        doc->nextPage();
        QTRY_COMPARE(doc->currentIndex(), 1);
        QTRY_COMPARE(tools->origin(), moved);

        QVERIFY(w.run(QStringLiteral("math.xy-origin")));
        QCOMPARE(tools->originVisible(), false);
        QVERIFY(!marker->isVisible());
    }

    // Un déplacement du marqueur (appui, nombreux déplacements, relâché) est une seule étape : un annuler
    // rend l'origine d'avant, un rétablir la rend.
    void originMoveIsUndoneAtOnce()
    {
        MainWindow w;
        QVERIFY(w.window());
        MathToolsController* tools = mathTools(w);
        actcraft::ui::DocumentController* doc = documentController(w);
        QVERIFY(tools && doc);
        QVERIFY(w.run(QStringLiteral("math.xy-origin")));
        const QPointF before = tools->origin();
        const std::size_t page = static_cast<std::size_t>(doc->currentIndex());

        const QPointF target = before + QPointF(150, 90);
        stylusPath(w, {before, before + QPointF(100, 60), target});
        QTRY_VERIFY(doc->document().page(page).presentation.hasOrigin);
        QVERIFY(samePoint(tools->origin().x(), tools->origin().y(), target.x(), target.y()));

        doc->undo();
        QVERIFY2(!doc->document().page(page).presentation.hasOrigin,
                 "un seul annuler rend l'origine d'avant");
        QCOMPARE(tools->origin(), before);
        doc->redo();
        QVERIFY(doc->document().page(page).presentation.hasOrigin);
        QVERIFY(samePoint(tools->origin().x(), tools->origin().y(), target.x(), target.y()));
    }

    // Document ouvert (remplacé, la page affichée gardant son indice) : l'origine des pages est lue du
    // document, sans aucun déplacement ni changement de page.
    void originReadFromAnOpenedDocument()
    {
        MainWindow w;
        QVERIFY(w.window());
        MathToolsController* tools = mathTools(w);
        actcraft::ui::DocumentController* doc = documentController(w);
        QVERIFY(tools && doc);
        QCOMPARE(doc->currentIndex(), 0);
        std::vector<actcraft::core::Page> pages(2);
        pages[0].presentation.hasOrigin = true;
        pages[0].presentation.origin = {123, 456};
        pages[1].presentation.hasOrigin = true;
        pages[1].presentation.origin = {700, 50};
        QVERIFY(tools->origin() != QPointF(123, 456));
        doc->replaceDocument(actcraft::core::Document(std::move(pages)), QUrl(), QStringLiteral("Origine"));
        QCOMPARE(doc->currentIndex(), 0);
        QTRY_COMPARE(tools->origin(), QPointF(123, 456));
        doc->nextPage();
        QTRY_COMPARE(doc->currentIndex(), 1);
        QTRY_COMPARE(tools->origin(), QPointF(700, 50));
    }

    // Non-régression : l'origine déplacée et mémorisée, un trait au stylet ne la bouge pas (le document
    // change, la page affichée est la même).
    void strokeKeepsTheOrigin()
    {
        MainWindow w;
        QVERIFY(w.window());
        MathToolsController* tools = mathTools(w);
        actcraft::ui::DocumentController* doc = documentController(w);
        QVERIFY(tools && doc);
        QVERIFY(w.run(QStringLiteral("math.xy-origin")));
        const QPointF target = tools->origin() + QPointF(200, -100);
        QVERIFY(tools->moveOrigin(target));
        QTRY_COMPARE(tools->origin(), target);

        const int before = w.pageObjectCount();
        stylusPath(w, {QPointF(150, 150), QPointF(300, 150)});
        QTRY_COMPARE(w.pageObjectCount(), before + 1);
        QTest::qWait(50);
        QCOMPARE(tools->origin(), target);
        const std::size_t page = static_cast<std::size_t>(doc->currentIndex());
        QVERIFY(doc->document().page(page).presentation.hasOrigin);
        QQuickItem* marker = w.item(QStringLiteral("originMarker"));
        QVERIFY(marker);
        QVERIFY(samePoint(marker->x() + marker->width() / 2, marker->y() + marker->height() / 2, target.x(),
                          target.y()));
    }

    // Translucidité du corps : kDefaultToolOpacity (100 sur 255), jamais opaque.
    void originMarkerOpacityIsTheDefault()
    {
        MainWindow w;
        QVERIFY(w.window());
        QVERIFY(w.run(QStringLiteral("math.xy-origin")));
        QQuickItem* body = w.item(QStringLiteral("originMarkerBody"));
        QVERIFY(body);
        QVERIFY(qAbs(body->opacity() - actcraft::features::kDefaultToolOpacity / 255.0) < 1e-6);
        QVERIFY(body->opacity() < 1.0);
    }

    void originMarkerBlocksTheStylus_data() { presentationharness::addPointerRows(); }

    // Appui sur le marqueur : il bouge, rien n'est versé. Pendant : le même pointeur hors du marqueur verse
    // un trait.
    void originMarkerBlocksTheStylus()
    {
        QFETCH(Pointer, pointer);
        MainWindow w;
        QVERIFY(w.window());
        QVERIFY(w.run(QStringLiteral("math.xy-origin")));
        MathToolsController* tools = mathTools(w);
        actcraft::ui::DocumentController* doc = documentController(w);
        QVERIFY(tools && doc);
        const QPointF start = tools->origin();
        const int before = w.pageObjectCount();
        // Un seul geste : encre lue d'abord sous le seuil de glisser, puis glisser jusqu'au bout.
        const QPointF from = scene(w, start);
        const QPointF to = from + QPointF(0, -60);
        pointerStep(w.window(), pointer, QEvent::TabletPress, from);
        pointerStep(w.window(), pointer, QEvent::TabletMove, from + QPointF(0, -2));
        QCOMPARE(w.pageView()->ink()->pointCount(), std::size_t{0});
        for (int k = 1; k <= 8; ++k) {
            pointerStep(w.window(), pointer, QEvent::TabletMove, from + (to - from) * (k / 8.0));
        }
        pointerStep(w.window(), pointer, QEvent::TabletRelease, to);
        QTRY_VERIFY2(QLineF(tools->origin(), start).length() > 20, "marqueur déplacé par son gestionnaire");
        QTest::qWait(50);
        QCOMPARE(w.pageObjectCount(), before);
        QCOMPARE(w.pageView()->ink()->pointCount(), std::size_t{0});
        QVERIFY(doc->document().page(static_cast<std::size_t>(doc->currentIndex())).presentation.hasOrigin);

        presentationharness::strokeAt(w.window(), pointer, scene(w, QPointF(150, 150)),
                                      scene(w, QPointF(300, 150)));
        QTRY_COMPARE(w.pageObjectCount(), before + 1);
        if (pointer == Pointer::Mouse) {
            // Comme pour les autres outils : le clic droit, que la vue teste par sa couverture, discrimine.
            QSignalSpy menu(w.pageView(), &actcraft::ui::PageView::contextMenuRequested);
            QTest::mouseClick(w.window(), Qt::RightButton, Qt::NoModifier,
                              scene(w, tools->origin()).toPoint());
            QTest::qWait(50);
            QCOMPARE(menu.count(), 0);
            QTest::mouseClick(w.window(), Qt::RightButton, Qt::NoModifier,
                              scene(w, QPointF(150, 600)).toPoint());
            QTRY_COMPARE(menu.count(), 1);
            QTest::keyClick(w.window(), Qt::Key_Escape);
        }
    }
};

} // namespace

ACTCRAFT_LOT7_TEST("mathtools", TstUiMathTools)
#include "tst_ui_mathtools.moc"
