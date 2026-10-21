#include "recognitioncontroller.h"
#include "support/lot7registry.h"
#include "support/presentationharness.h"

#include <actcraft/core/document.h>
#include <actcraft/core/model.h>
#include <actcraft/features/recognitionsession.h>
#include <actcraft/ui/documentcontroller.h>

#include <QColor>
#include <QtTest>

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <numbers>
#include <utility>
#include <variant>
#include <vector>

namespace {

// Tout le fichier dans cet espace de noms anonyme (exécutable unique : aucun nom global hors du registre).
namespace core = actcraft::core;
using actcraft::features::RecognitionSession;
using actcraft::ui::DocumentController;
using actcraft::ui::RecognitionController;
using presentationharness::MainWindow;

constexpr std::int64_t kNsPerMs = 1000000;
constexpr core::Argb kInk = 0xFF2040C0u;
constexpr double kInkWidth = 4;

DocumentController* controllerOf(const MainWindow& main)
{
    return main.window() ? main.window()->findChild<DocumentController*>(QStringLiteral("documentController"))
                         : nullptr;
}

RecognitionController* recognitionOf(const MainWindow& main)
{
    return main.window() ? main.window()->findChild<RecognitionController*>(QStringLiteral("recognition"))
                         : nullptr;
}

// Trait du stylo tel que la vue le verse (DocumentController::commitStroke : origine Stroke).
core::Stroke strokeThrough(const std::vector<QPointF>& points)
{
    core::Stroke stroke;
    stroke.baseWidth = kInkWidth;
    stroke.color = kInk;
    for (const QPointF& p : points)
        stroke.points.push_back({p.x(), p.y(), 0.5f});
    return stroke;
}

// Cercle à main levée de 40 points, fermé : un seul tour.
std::vector<QPointF> circlePoints(QPointF centre, double radius)
{
    std::vector<QPointF> points;
    for (int i = 0; i <= 40; ++i) {
        const double a = 2 * std::numbers::pi * static_cast<double>(i) / 40;
        points.emplace_back(centre.x() + radius * std::cos(a), centre.y() + radius * std::sin(a));
    }
    return points;
}

// Côtés droits entre les coins, `perSide` points par côté, refermés sur le premier coin.
std::vector<QPointF> closedPolyline(const std::vector<QPointF>& corners, int perSide)
{
    std::vector<QPointF> points;
    for (std::size_t i = 0; i < corners.size(); ++i) {
        const QPointF a = corners[i];
        const QPointF b = corners[(i + 1) % corners.size()];
        for (int k = 0; k < perSide; ++k)
            points.push_back(a + (b - a) * (static_cast<double>(k) / perSide));
    }
    points.push_back(corners.front());
    return points;
}

// Zigzag ouvert : aucune forme (tst_features_shapefinder, openScribbleIsNothing).
std::vector<QPointF> scribblePoints()
{
    return {{100, 100}, {180, 260}, {120, 90}, {300, 310}, {90, 200}, {260, 80}};
}

const std::vector<core::PageObject>& shownObjects(const DocumentController& controller)
{
    return controller.document().page(static_cast<std::size_t>(controller.currentIndex())).objects;
}

int countOf(const DocumentController& controller, core::ObjectKind kind)
{
    int count = 0;
    for (const core::PageObject& object : shownObjects(controller))
        count += core::kindOf(object) == kind ? 1 : 0;
    return count;
}

// Seule forme de la page affichée, ou nullptr.
const core::PageObject* onlyShape(const DocumentController& controller)
{
    const core::PageObject* found = nullptr;
    for (const core::PageObject& object : shownObjects(controller)) {
        if (core::kindOf(object) != core::ObjectKind::Shape)
            continue;
        if (found != nullptr)
            return nullptr;
        found = &object;
    }
    return found;
}

const core::Shape& shapeOf(const core::PageObject& object)
{
    return std::get<core::Shape>(core::body(object));
}

bool near(double value, double expected, double tolerance)
{
    return std::abs(value - expected) <= tolerance;
}

std::int64_t atMs(int ms)
{
    return std::int64_t{ms} * kNsPerMs;
}

class TstUiRecognition : public QObject
{
    Q_OBJECT

private slots:
    // Mode ouvert par la commande ; un cercle tracé reste un trait jusqu'au délai, puis devient UNE forme
    // `Ellipse` (contour du trait, sans remplissage, couche intermédiaire) ; un seul Annuler rend le trait.
    void recognizedStrokeMakesOneStep()
    {
        std::int64_t now = 0;
        MainWindow main;
        QVERIFY(main.window() != nullptr);
        DocumentController* controller = controllerOf(main);
        RecognitionController* recognition = recognitionOf(main);
        QVERIFY(controller != nullptr);
        QVERIFY(recognition != nullptr);
        recognition->setClock([&now] { return now; });
        QVERIFY(!recognition->shapeMode());
        QVERIFY(main.run(QStringLiteral("tool.shape-recognition")));
        QVERIFY(recognition->shapeMode());

        controller->commitStroke(strokeThrough(circlePoints({400, 300}, 120)));
        QCOMPARE(countOf(*controller, core::ObjectKind::Stroke), 1);
        now = atMs(recognition->delayMs() - 1);
        recognition->processDue();
        QCOMPARE(countOf(*controller, core::ObjectKind::Stroke), 1); // délai pas encore écoulé
        QCOMPARE(countOf(*controller, core::ObjectKind::Shape), 0);

        now = atMs(recognition->delayMs());
        recognition->processDue();
        QCOMPARE(shownObjects(*controller).size(), std::size_t{1});
        const core::PageObject* object = onlyShape(*controller);
        QVERIFY(object != nullptr);
        const core::Shape& shape = shapeOf(*object);
        QVERIFY(shape.kind == core::ShapeKind::Ellipse);
        QCOMPARE(shape.outline, kInk);
        QCOMPARE(shape.outlineWidth, kInkWidth);
        QCOMPARE(shape.fill >> 24, 0u); // transparent : option de remplissage fermée par défaut
        QVERIFY(object->common.layer == core::Layer::Middle);
        const core::PointF centre = object->common.transform.map(
            {shape.bounds.x + shape.bounds.width / 2, shape.bounds.y + shape.bounds.height / 2});
        QVERIFY2(near(centre.x, 400, 6) && near(centre.y, 300, 6),
                 qPrintable(QStringLiteral("%1, %2").arg(centre.x).arg(centre.y)));
        QVERIFY2(near(shape.bounds.width, 240, 12) && near(shape.bounds.height, 240, 12),
                 qPrintable(QStringLiteral("%1 x %2").arg(shape.bounds.width).arg(shape.bounds.height)));
        QCOMPARE(recognition->lastKind(), QStringLiteral("circle"));

        controller->undo(); // une étape : le trait revient, la forme part
        QCOMPARE(countOf(*controller, core::ObjectKind::Stroke), 1);
        QCOMPARE(countOf(*controller, core::ObjectKind::Shape), 0);
        controller->undo(); // puis le trait lui-même
        QVERIFY(shownObjects(*controller).empty());
        QVERIFY(!controller->canUndo());
    }

    // Option « remplir les formes reconnues » : remplissage de la couleur du contour, ou de la couleur
    // choisie.
    void fillOptionFillsTheShape()
    {
        std::int64_t now = 0;
        MainWindow main;
        QVERIFY(main.window() != nullptr);
        DocumentController* controller = controllerOf(main);
        RecognitionController* recognition = recognitionOf(main);
        QVERIFY(controller != nullptr);
        QVERIFY(recognition != nullptr);
        recognition->setClock([&now] { return now; });
        recognition->setShapeMode(true);
        QVERIFY(!recognition->fillShapes());
        recognition->setFillShapes(true);

        controller->commitStroke(strokeThrough(circlePoints({400, 300}, 120)));
        now = atMs(recognition->delayMs());
        recognition->processDue();
        const core::PageObject* inkFilled = onlyShape(*controller);
        QVERIFY(inkFilled != nullptr);
        QCOMPARE(shapeOf(*inkFilled).fill, kInk); // couleur vide : celle du contour
        QCOMPARE(shapeOf(*inkFilled).outline, kInk);

        controller->undo();
        controller->undo();
        QVERIFY(shownObjects(*controller).empty());
        recognition->setFillColor(QColor(255, 0, 0));
        controller->commitStroke(strokeThrough(circlePoints({400, 300}, 120)));
        now += atMs(recognition->delayMs());
        recognition->processDue();
        const core::PageObject* redFilled = onlyShape(*controller);
        QVERIFY(redFilled != nullptr);
        QCOMPARE(shapeOf(*redFilled).fill, core::Argb{0xFFFF0000u});
        QCOMPARE(shapeOf(*redFilled).outline, kInk);
    }

    // Trapèze tracé (angles de 76° et 104°) : ni rectangle ni carré, un polygone convexe d'un seul tour,
    // rangé en `Shape` `Polygon` dont les sommets sont les coins tracés.
    void roughQuadrilateralBecomesAPolygonShape()
    {
        std::int64_t now = 0;
        MainWindow main;
        QVERIFY(main.window() != nullptr);
        DocumentController* controller = controllerOf(main);
        RecognitionController* recognition = recognitionOf(main);
        QVERIFY(controller != nullptr);
        QVERIFY(recognition != nullptr);
        recognition->setClock([&now] { return now; });
        recognition->setShapeMode(true);

        const std::vector<QPointF> corners{{100, 100}, {300, 100}, {350, 300}, {50, 300}};
        controller->commitStroke(strokeThrough(closedPolyline(corners, 25)));
        now = atMs(recognition->delayMs());
        recognition->processDue();
        QCOMPARE(countOf(*controller, core::ObjectKind::Stroke), 0);
        const core::PageObject* object = onlyShape(*controller);
        QVERIFY(object != nullptr);
        const core::Shape& shape = shapeOf(*object);
        QVERIFY(shape.kind == core::ShapeKind::Polygon);
        QCOMPARE(shape.polygon.size(), std::size_t{4});
        for (const QPointF& corner : corners) {
            bool matched = false;
            for (const core::PointF& vertex : shape.polygon) {
                const core::PointF onPage = object->common.transform.map(vertex);
                matched = matched || (near(onPage.x, corner.x(), 12) && near(onPage.y, corner.y(), 12));
            }
            QVERIFY2(matched, qPrintable(QStringLiteral("coin %1, %2").arg(corner.x()).arg(corner.y())));
        }
        QCOMPARE(shape.outline, kInk);
        QCOMPARE(recognition->lastKind(), QStringLiteral("polygon"));
    }

    // Délai du contrôleur = RecognitionSession::kDefaultDelayMs (1 s, jamais recopié) ; borné de 0 à 5 s ;
    // 0 : la forme part au relâché, sans que le temps avance.
    void controllerDelayIsTheSessionDefault()
    {
        std::int64_t now = 0;
        MainWindow main;
        QVERIFY(main.window() != nullptr);
        DocumentController* controller = controllerOf(main);
        RecognitionController* recognition = recognitionOf(main);
        QVERIFY(controller != nullptr);
        QVERIFY(recognition != nullptr);
        recognition->setClock([&now] { return now; });
        QCOMPARE(recognition->delayMs(), RecognitionSession::kDefaultDelayMs);
        recognition->setShapeMode(true);

        controller->commitStroke(strokeThrough(circlePoints({400, 300}, 120)));
        now = atMs(RecognitionSession::kDefaultDelayMs - 1);
        recognition->processDue();
        QCOMPARE(countOf(*controller, core::ObjectKind::Shape), 0);
        now = atMs(RecognitionSession::kDefaultDelayMs);
        recognition->processDue();
        QCOMPARE(countOf(*controller, core::ObjectKind::Shape), 1);

        recognition->setDelayMs(9000);
        QCOMPARE(recognition->delayMs(), RecognitionSession::kMaxDelayMs);
        recognition->setDelayMs(-3);
        QCOMPARE(recognition->delayMs(), 0);
        controller->commitStroke(strokeThrough(circlePoints({600, 400}, 80)));
        QTRY_COMPARE(countOf(*controller, core::ObjectKind::Shape), 2); // au relâché : le temps n'a pas bougé
        QCOMPARE(countOf(*controller, core::ObjectKind::Stroke), 0);
    }

    // Document remplacé (ouverture) : la session est vidée (documentReplaced), le mode reste ; rien n'est
    // reconnu après coup dans le document neuf.
    void sessionResetsOnAnOpenedDocument()
    {
        std::int64_t now = 0;
        MainWindow main;
        QVERIFY(main.window() != nullptr);
        DocumentController* controller = controllerOf(main);
        RecognitionController* recognition = recognitionOf(main);
        QVERIFY(controller != nullptr);
        QVERIFY(recognition != nullptr);
        recognition->setClock([&now] { return now; });
        recognition->setShapeMode(true);

        controller->commitStroke(strokeThrough(circlePoints({400, 300}, 120)));
        QCOMPARE(recognition->pendingCount(), 1);
        controller->replaceDocument(core::Document(std::vector<core::Page>(1)), QUrl(),
                                    QStringLiteral("Ouvert"));
        QCOMPARE(recognition->pendingCount(), 0);
        QVERIFY(recognition->shapeMode());
        now = atMs(10 * RecognitionSession::kMaxDelayMs);
        recognition->processDue();
        QVERIFY(shownObjects(*controller).empty());
        QVERIFY(!controller->canUndo());

        // La session sert encore dans le document neuf.
        controller->commitStroke(strokeThrough(circlePoints({400, 300}, 120)));
        QCOMPARE(recognition->pendingCount(), 1);
        now += atMs(recognition->delayMs());
        recognition->processDue();
        QCOMPARE(countOf(*controller, core::ObjectKind::Shape), 1);
    }

    // Un cercle tracé, puis un second trait avant la fin du délai : un trait ne remet pas la session à zéro
    // (documentChanged part après chaque trait) ; les deux traits restent dans la session et la
    // reconnaissance part au délai compté depuis le dernier.
    void strokeKeepsThePendingRecognition()
    {
        std::int64_t now = 0;
        MainWindow main;
        QVERIFY(main.window() != nullptr);
        DocumentController* controller = controllerOf(main);
        RecognitionController* recognition = recognitionOf(main);
        QVERIFY(controller != nullptr);
        QVERIFY(recognition != nullptr);
        recognition->setClock([&now] { return now; });
        recognition->setShapeMode(true);
        const int delay = recognition->delayMs();
        QVERIFY(delay > 500);

        controller->commitStroke(strokeThrough(circlePoints({400, 300}, 120)));
        QCOMPARE(recognition->pendingCount(), 1);
        now = atMs(500);
        controller->commitStroke(strokeThrough(scribblePoints()));
        QCOMPARE(recognition->pendingCount(), 2);

        now = atMs(delay + 400); // délai du cercle écoulé, pas celui du second trait
        recognition->processDue();
        QCOMPARE(recognition->pendingCount(), 2);
        QCOMPARE(countOf(*controller, core::ObjectKind::Stroke), 2);
        QCOMPARE(countOf(*controller, core::ObjectKind::Shape), 0);

        now = atMs(500 + delay);
        recognition->processDue();
        QCOMPARE(recognition->pendingCount(), 0);
        QCOMPARE(countOf(*controller, core::ObjectKind::Shape), 1);  // le cercle
        QCOMPARE(countOf(*controller, core::ObjectKind::Stroke), 1); // le zigzag, non reconnu, reste
        QCOMPARE(recognition->lastKind(), QStringLiteral("circle"));
    }

    // Trait qui ne ressemble à aucune forme : il reste, rien n'entre dans la pile, la session l'oublie.
    void unrecognizedStrokeStays()
    {
        std::int64_t now = 0;
        MainWindow main;
        QVERIFY(main.window() != nullptr);
        DocumentController* controller = controllerOf(main);
        RecognitionController* recognition = recognitionOf(main);
        QVERIFY(controller != nullptr);
        QVERIFY(recognition != nullptr);
        recognition->setClock([&now] { return now; });
        recognition->setShapeMode(true);

        controller->commitStroke(strokeThrough(scribblePoints()));
        now = atMs(recognition->delayMs());
        recognition->processDue();
        QCOMPARE(recognition->pendingCount(), 0);
        QCOMPARE(countOf(*controller, core::ObjectKind::Stroke), 1);
        QCOMPARE(countOf(*controller, core::ObjectKind::Shape), 0);
        controller->undo(); // la seule étape est le trait
        QVERIFY(shownObjects(*controller).empty());
        QVERIFY(!controller->canUndo());
    }

    // Mode fermé : aucun trait n'entre dans la session, un cercle reste un trait.
    void modeOffLeavesStrokes()
    {
        std::int64_t now = 0;
        MainWindow main;
        QVERIFY(main.window() != nullptr);
        DocumentController* controller = controllerOf(main);
        RecognitionController* recognition = recognitionOf(main);
        QVERIFY(controller != nullptr);
        QVERIFY(recognition != nullptr);
        recognition->setClock([&now] { return now; });
        QVERIFY(!recognition->shapeMode());

        controller->commitStroke(strokeThrough(circlePoints({400, 300}, 120)));
        QCOMPARE(recognition->pendingCount(), 0);
        now = atMs(10 * RecognitionSession::kMaxDelayMs);
        recognition->processDue();
        QCOMPARE(countOf(*controller, core::ObjectKind::Stroke), 1);
        QCOMPARE(countOf(*controller, core::ObjectKind::Shape), 0);
    }

    // `recognition.convert-to-shape` : les traits en attente sont convertis tout de suite, sans le délai.
    void convertToShapeConvertsThePendingStrokes()
    {
        std::int64_t now = 0;
        MainWindow main;
        QVERIFY(main.window() != nullptr);
        DocumentController* controller = controllerOf(main);
        RecognitionController* recognition = recognitionOf(main);
        QVERIFY(controller != nullptr);
        QVERIFY(recognition != nullptr);
        recognition->setClock([&now] { return now; });
        recognition->setShapeMode(true);

        controller->commitStroke(strokeThrough(circlePoints({400, 300}, 120)));
        QCOMPARE(recognition->pendingCount(), 1);
        QVERIFY(main.run(QStringLiteral("recognition.convert-to-shape")));
        QCOMPARE(recognition->pendingCount(), 0);
        QCOMPARE(countOf(*controller, core::ObjectKind::Shape), 1);
        QCOMPARE(countOf(*controller, core::ObjectKind::Stroke), 0);
    }
};

} // namespace

ACTCRAFT_LOT7_TEST("recognition", TstUiRecognition)
#include "tst_ui_recognition.moc"
