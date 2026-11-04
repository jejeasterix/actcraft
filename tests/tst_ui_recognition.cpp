#include "recognitioncontroller.h"
#include "support/lot7registry.h"
#include "support/presentationharness.h"

#include <actcraft/core/document.h>
#include <actcraft/core/model.h>
#include <actcraft/features/handwriting.h>
#include <actcraft/features/recognitionsession.h>
#include <actcraft/ui/commands.h>
#include <actcraft/ui/documentcontroller.h>
#include <actcraft/ui/toolhost.h>

#include <QColor>
#include <QTemporaryDir>
#include <QtTest>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <mutex>
#include <numbers>
#include <utility>
#include <variant>
#include <vector>

namespace {

// Tout le fichier dans cet espace de noms anonyme (exécutable unique : aucun nom global hors du registre).
namespace core = actcraft::core;
using actcraft::features::HandwritingRecognizer;
using actcraft::features::InkStrokes;
using actcraft::features::RecognitionSession;
using actcraft::ui::Commands;
using actcraft::ui::DocumentController;
using actcraft::ui::RecognitionController;
using actcraft::ui::ToolHost;
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

// Outils de l'utilisateur (lot 3) : la sélection passe par eux.
ToolHost* toolsOf(const MainWindow& main)
{
    return main.window() ? main.window()->findChild<ToolHost*>() : nullptr;
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

// Journal du reconnaisseur factice, écrit sur le fil de la reconnaissance, lu par le test.
struct FakeLog
{
    std::mutex mutex;
    std::vector<QThread*> creations;    // fil de chaque création du reconnaisseur
    std::vector<QThread*> recognitions; // fil de chaque reconnaissance
    std::vector<InkStrokes> requests;   // traits reçus à chaque reconnaissance
};

// Reconnaisseur factice : rend, à son n-ième appel, la n-ième liste de `answers` (la dernière ensuite).
class FakeRecognizer final : public HandwritingRecognizer
{
public:
    FakeRecognizer(std::shared_ptr<FakeLog> log, bool isAvailable, QString statusText,
                   std::vector<QStringList> answerList)
        : m_log(std::move(log)), m_available(isAvailable), m_status(std::move(statusText)),
          m_answers(std::move(answerList))
    {
        const std::lock_guard lock(m_log->mutex);
        m_log->creations.push_back(QThread::currentThread());
    }
    bool available() const override { return m_available; }
    QString status() const override { return m_status; }
    QStringList recognize(const InkStrokes& strokes) override
    {
        const std::lock_guard lock(m_log->mutex);
        m_log->recognitions.push_back(QThread::currentThread());
        m_log->requests.push_back(strokes);
        if (m_answers.empty())
            return {};
        return m_answers[std::min(m_log->recognitions.size(), m_answers.size()) - 1];
    }

private:
    std::shared_ptr<FakeLog> m_log;
    bool m_available;
    QString m_status;
    std::vector<QStringList> m_answers;
};

std::shared_ptr<FakeLog> installFake(RecognitionController& recognition, bool available,
                                     std::vector<QStringList> answers,
                                     const QString& status = QStringLiteral("Reconnaisseur factice"))
{
    auto log = std::make_shared<FakeLog>();
    recognition.setHandwritingFactoryForTest([log, available, answers, status] {
        return std::make_unique<FakeRecognizer>(log, available, status, answers);
    });
    return log;
}

// Trait d'écriture de (x0, y0) à (x1, y1).
std::vector<QPointF> inkLine(double x0, double y0, double x1, double y1)
{
    std::vector<QPointF> points;
    for (int i = 0; i <= 12; ++i) {
        const double t = static_cast<double>(i) / 12;
        points.emplace_back(x0 + (x1 - x0) * t, y0 + (y1 - y0) * t);
    }
    return points;
}

// Objets texte de la page affichée, dans l'ordre de la page.
std::vector<const core::PageObject*> textsOf(const DocumentController& controller)
{
    std::vector<const core::PageObject*> found;
    for (const core::PageObject& object : shownObjects(controller))
        if (core::kindOf(object) == core::ObjectKind::Text)
            found.push_back(&object);
    return found;
}

QString htmlOf(const core::PageObject& object)
{
    return QString::fromStdString(std::get<core::Text>(core::body(object)).html);
}

// L'écriture manuscrite et sa conversion n'existent que sous Windows (« platforms » de tool.handwriting et de
// recognition.convert-to-text dans commands.json) ; le reconnaisseur factice ne dépend d'aucun moteur. La fenêtre
// se déclare donc Windows : sans cela, sous macOS, ces commandes sont absentes et le test ne prouve rien
// (L7-T41). La restriction de plateforme elle-même est prouvée par handwritingAbsentOnAndroid.
bool declareWindows(const MainWindow& main)
{
    auto* commands = main.window() ? main.window()->findChild<Commands*>(QStringLiteral("commands")) : nullptr;
    if (!commands) {
        return false;
    }
    commands->setPlatform(actcraft::actions::Platform::Windows);
    return commands->isAvailable(QStringLiteral("tool.handwriting")) &&
           commands->isAvailable(QStringLiteral("recognition.convert-to-text"));
}

// Fenêtre prête, mode écriture ouvert avec un reconnaisseur factice disponible ; horloge injectée.
struct Writing
{
    MainWindow main;
    DocumentController* controller = nullptr;
    RecognitionController* recognition = nullptr;
    std::shared_ptr<FakeLog> log;
    std::int64_t now = 0;

    bool open(std::vector<QStringList> answers)
    {
        controller = controllerOf(main);
        recognition = recognitionOf(main);
        if (!main.window() || !controller || !recognition || !declareWindows(main))
            return false;
        log = installFake(*recognition, true, std::move(answers));
        recognition->setClock([this] { return now; });
        if (!QTest::qWaitFor([this] { return recognition->handwritingAvailable(); }, 10000))
            return false;
        return main.run(QStringLiteral("tool.handwriting")) && recognition->handwritingMode();
    }
};

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

    // F-296 a posteriori : `recognition.convert-to-shape` agit sur les annotations SÉLECTIONNÉES, mode fermé
    // ou non, en une étape. Cercle A tracé mode fermé (jamais en attente), sélectionné ; puis mode ouvert et
    // cercle B tracé (en attente, non sélectionné) : seul A devient une forme, B reste un trait en attente.
    // Sous l'ancien gestionnaire (convertPending) c'est B qui serait converti, et A resterait un trait.
    void convertSelectionToShape()
    {
        std::int64_t now = 0;
        MainWindow main;
        QVERIFY(main.window() != nullptr);
        DocumentController* controller = controllerOf(main);
        RecognitionController* recognition = recognitionOf(main);
        ToolHost* tools = toolsOf(main);
        QVERIFY(controller != nullptr);
        QVERIFY(recognition != nullptr);
        QVERIFY(tools != nullptr);
        recognition->setClock([&now] { return now; });
        QVERIFY(!recognition->shapeMode());
        QVERIFY(main.run(QStringLiteral("tool.select")));

        controller->commitStroke(strokeThrough(circlePoints({400, 300}, 120))); // A
        QCOMPARE(recognition->pendingCount(), 0);
        tools->selectAll();
        QTRY_COMPARE(tools->selectionCount(), 1);
        recognition->setShapeMode(true);
        controller->commitStroke(strokeThrough(circlePoints({800, 500}, 60))); // B
        QCOMPARE(recognition->pendingCount(), 1);
        QCOMPARE(tools->selectionCount(), 1); // B n'entre pas dans la sélection

        QVERIFY(main.run(QStringLiteral("recognition.convert-to-shape")));
        QCOMPARE(countOf(*controller, core::ObjectKind::Shape), 1);
        QCOMPARE(countOf(*controller, core::ObjectKind::Stroke), 1);
        QCOMPARE(recognition->pendingCount(), 1); // B attend toujours son délai
        const core::PageObject* object = onlyShape(*controller);
        QVERIFY(object != nullptr);
        const core::Shape& shape = shapeOf(*object);
        QVERIFY(shape.kind == core::ShapeKind::Ellipse);
        const core::PointF centre = object->common.transform.map(
            {shape.bounds.x + shape.bounds.width / 2, shape.bounds.y + shape.bounds.height / 2});
        QVERIFY2(near(centre.x, 400, 6) && near(centre.y, 300, 6),
                 qPrintable(QStringLiteral("%1, %2").arg(centre.x).arg(centre.y)));
        QVERIFY(object->common.layer == core::Layer::Middle);

        controller->undo(); // une étape : A revient en trait
        QCOMPARE(countOf(*controller, core::ObjectKind::Shape), 0);
        QCOMPARE(countOf(*controller, core::ObjectKind::Stroke), 2);
    }

    // F-295 a posteriori : `recognition.convert-to-text` reconnaît les annotations SÉLECTIONNÉES comme de
    // l'écriture, mode écriture fermé, en une étape. Mot A tracé puis sélectionné, trait B tracé ensuite (non
    // sélectionné) : seul A part au reconnaisseur et devient un texte, B reste un trait. Sous convertPending
    // (aucun mode ouvert, rien en attente), rien ne serait converti.
    void convertSelectionToText()
    {
        MainWindow main;
        QVERIFY(main.window() != nullptr);
        DocumentController* controller = controllerOf(main);
        RecognitionController* recognition = recognitionOf(main);
        ToolHost* tools = toolsOf(main);
        QVERIFY(controller != nullptr);
        QVERIFY(recognition != nullptr);
        QVERIFY(tools != nullptr);
        QVERIFY(declareWindows(main));
        const std::shared_ptr<FakeLog> log =
            installFake(*recognition, true, {{QStringLiteral("bonjour"), QStringLiteral("bonjours")}});
        if (!QTest::qWaitFor([recognition] { return recognition->handwritingAvailable(); }, 10000))
            QSKIP("Sans reconnaisseur d'écriture : conversion en texte non vérifiée.");
        QVERIFY(!recognition->handwritingMode());
        QVERIFY(!recognition->shapeMode());
        QVERIFY(main.run(QStringLiteral("tool.select")));

        controller->commitStroke(strokeThrough(inkLine(300, 300, 500, 310))); // A
        tools->selectAll();
        QTRY_COMPARE(tools->selectionCount(), 1);
        controller->commitStroke(strokeThrough(inkLine(700, 500, 800, 510))); // B
        QCOMPARE(tools->selectionCount(), 1);
        QCOMPARE(recognition->pendingCount(), 0);

        QVERIFY(main.run(QStringLiteral("recognition.convert-to-text")));
        QTRY_COMPARE_WITH_TIMEOUT(countOf(*controller, core::ObjectKind::Text), 1, 10000);
        QCOMPARE(countOf(*controller, core::ObjectKind::Stroke), 1); // B
        const std::vector<const core::PageObject*> texts = textsOf(*controller);
        QVERIFY(htmlOf(*texts.front()).contains(QStringLiteral(">bonjour</p>")));
        QVERIFY(texts.front()->common.layer == core::Layer::Middle);
        QVERIFY(near(std::get<core::Text>(core::body(*texts.front())).bounds.x, 300, 1));
        {
            const std::lock_guard lock(log->mutex);
            QCOMPARE(log->requests.size(), std::size_t{1});
            QCOMPARE(log->requests.front().size(), std::size_t{1}); // le seul trait A
            QVERIFY(near(log->requests.front().front().front().x(), 300, 1));
        }

        controller->undo(); // une étape : A revient en trait
        QCOMPARE(countOf(*controller, core::ObjectKind::Text), 0);
        QCOMPARE(countOf(*controller, core::ObjectKind::Stroke), 2);
    }

    // Cercles A puis B tracés, B annulé avant l'échéance : aucune conversion n'est poussée ensuite. Sinon la
    // conversion différée de A entrerait dans la pile et UndoStack::record effacerait la branche à rétablir :
    // B serait perdu sans geste de l'utilisateur.
    void undoWhileAStrokeIsPendingKeepsRedo()
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
        QVERIFY(recognition->delayMs() > 600);

        controller->commitStroke(strokeThrough(circlePoints({400, 300}, 120))); // A
        now = atMs(400);
        controller->commitStroke(strokeThrough(circlePoints({600, 400}, 80))); // B
        QCOMPARE(countOf(*controller, core::ObjectKind::Stroke), 2);
        now = atMs(600);
        controller->undo(); // B part, pendant le délai
        QCOMPARE(countOf(*controller, core::ObjectKind::Stroke), 1);
        QVERIFY(controller->canRedo());

        now = atMs(10 * RecognitionSession::kMaxDelayMs); // échéances de A et de B passées
        recognition->processDue();
        QVERIFY(controller->canRedo()); // aucune étape poussée : Rétablir tient
        QCOMPARE(countOf(*controller, core::ObjectKind::Shape), 0);
        QCOMPARE(countOf(*controller, core::ObjectKind::Stroke), 1);

        controller->redo(); // B revient
        QCOMPARE(countOf(*controller, core::ObjectKind::Stroke), 2);
        QVERIFY(!controller->canRedo());
        // Rétablir n'arme aucune reconnaissance : le délai échu ne convertit ni A ni B.
        QCOMPARE(recognition->pendingCount(), 0);
        now = atMs(20 * RecognitionSession::kMaxDelayMs);
        recognition->processDue();
        QCOMPARE(countOf(*controller, core::ObjectKind::Shape), 0);
        QCOMPARE(countOf(*controller, core::ObjectKind::Stroke), 2);
    }

    // Mode écriture ouvert par la commande : un mot écrit reste un trait jusqu'au délai, puis devient UN
    // objet texte (couche intermédiaire, meilleure lecture) ; un seul Annuler rend le trait.
    void handwritingMakesOneStep()
    {
        Writing w;
        QVERIFY(w.open({{QStringLiteral("bonjour"), QStringLiteral("bonjours")}}));
        QVERIFY(!w.recognition->shapeMode());

        w.controller->commitStroke(strokeThrough(inkLine(300, 300, 500, 310)));
        QCOMPARE(countOf(*w.controller, core::ObjectKind::Stroke), 1);
        w.now = atMs(w.recognition->delayMs() - 1);
        w.recognition->processDue();
        QTest::qWait(100);
        QCOMPARE(countOf(*w.controller, core::ObjectKind::Text), 0); // délai pas encore écoulé

        w.now = atMs(w.recognition->delayMs());
        w.recognition->processDue();
        QTRY_COMPARE_WITH_TIMEOUT(countOf(*w.controller, core::ObjectKind::Text), 1, 10000);
        QCOMPARE(countOf(*w.controller, core::ObjectKind::Stroke), 0);
        const std::vector<const core::PageObject*> texts = textsOf(*w.controller);
        QVERIFY(htmlOf(*texts.front()).contains(QStringLiteral(">bonjour</p>")));
        QVERIFY(texts.front()->common.layer == core::Layer::Middle);
        QCOMPARE(w.recognition->alternatives(), QStringList{QStringLiteral("bonjours")});

        w.controller->undo(); // une étape : le trait revient, le texte part
        QCOMPARE(countOf(*w.controller, core::ObjectKind::Stroke), 1);
        QCOMPARE(countOf(*w.controller, core::ObjectKind::Text), 0);
        w.controller->undo();
        QVERIFY(shownObjects(*w.controller).empty());
        QVERIFY(!w.controller->canUndo());
    }

    // Choisir une alternative remplace le texte, en une étape ; Annuler rend la première lecture.
    void alternativeReplacesTheText()
    {
        Writing w;
        QVERIFY(w.open({{QStringLiteral("bonjour"), QStringLiteral("bonjours")}}));
        w.controller->commitStroke(strokeThrough(inkLine(300, 300, 500, 310)));
        w.now = atMs(w.recognition->delayMs());
        w.recognition->processDue();
        QTRY_COMPARE_WITH_TIMEOUT(countOf(*w.controller, core::ObjectKind::Text), 1, 10000);

        w.recognition->chooseAlternative(0);
        QCOMPARE(textsOf(*w.controller).size(), std::size_t{1});
        QVERIFY(htmlOf(*textsOf(*w.controller).front()).contains(QStringLiteral(">bonjours</p>")));
        QVERIFY(w.recognition->alternatives().contains(QStringLiteral("bonjour")));
        w.controller->undo();
        QCOMPARE(textsOf(*w.controller).size(), std::size_t{1});
        QVERIFY(htmlOf(*textsOf(*w.controller).front()).contains(QStringLiteral(">bonjour</p>")));
    }

    // Relecture du lot 7 : conversion en texte limitée au stylo (penStrokeOf, comme la conversion en formes).
    // Un trait de marqueur sélectionné avec le mot n'est pas lu : il reste un trait. Mutant : penStrokeOf
    // accepte tout trait → le marqueur part avec le mot.
    void convertToTextTakesThePenOnly()
    {
        MainWindow main;
        QVERIFY(main.window() != nullptr);
        DocumentController* controller = controllerOf(main);
        RecognitionController* recognition = recognitionOf(main);
        ToolHost* tools = toolsOf(main);
        QVERIFY(controller != nullptr);
        QVERIFY(recognition != nullptr);
        QVERIFY(tools != nullptr);
        QVERIFY(declareWindows(main));
        const std::shared_ptr<FakeLog> log = installFake(*recognition, true, {{QStringLiteral("bonjour")}});
        QTRY_VERIFY_WITH_TIMEOUT(recognition->handwritingAvailable(), 10000);
        QVERIFY(main.run(QStringLiteral("tool.select")));
        controller->commitStroke(strokeThrough(inkLine(300, 300, 500, 310))); // mot au stylo
        core::Stroke marker = strokeThrough(inkLine(300, 360, 500, 370));
        marker.kind = core::StrokeKind::Highlighter;
        controller->commitStroke(marker);
        tools->selectAll();
        QTRY_COMPARE(tools->selectionCount(), 2);

        QVERIFY(main.run(QStringLiteral("recognition.convert-to-text")));
        QTRY_COMPARE_WITH_TIMEOUT(countOf(*controller, core::ObjectKind::Text), 1, 10000);
        QCOMPARE(countOf(*controller, core::ObjectKind::Stroke), 1);
        for (const core::PageObject& object : shownObjects(*controller)) {
            if (const auto* stroke = std::get_if<core::Stroke>(&core::body(object)))
                QVERIFY(stroke->kind == core::StrokeKind::Highlighter);
        }
    }

    // Relecture du lot 7 : un trait en attente (mode écriture ouvert), sélectionné puis converti par
    // « Convertir en texte », quitte la session : plus rien n'attend, et le délai échu ne relance rien. Avant :
    // compté en attente après sa conversion (pendingCount 1), son délai courait encore.
    void selectionTakesThePendingStrokeOutOfTheSession()
    {
        Writing w;
        QVERIFY(w.open({{QStringLiteral("bonjour")}}));
        ToolHost* tools = toolsOf(w.main);
        QVERIFY(tools != nullptr);
        w.controller->commitStroke(strokeThrough(inkLine(300, 300, 500, 310)));
        QVERIFY(w.main.run(QStringLiteral("tool.select")));
        tools->selectAll();
        QTRY_COMPARE(tools->selectionCount(), 1);
        QVERIFY(w.recognition->handwritingMode());
        QCOMPARE(w.recognition->pendingCount(), 1); // en attente ET sélectionné

        QVERIFY(w.main.run(QStringLiteral("recognition.convert-to-text")));
        QCOMPARE(w.recognition->pendingCount(), 0);
        QTRY_COMPARE_WITH_TIMEOUT(countOf(*w.controller, core::ObjectKind::Text), 1, 10000);
        w.now = atMs(10 * RecognitionSession::kMaxDelayMs);
        w.recognition->processDue();
        QCOMPARE(w.recognition->pendingCount(), 0);
        QCOMPARE(countOf(*w.controller, core::ObjectKind::Text), 1);
        QCOMPARE(countOf(*w.controller, core::ObjectKind::Stroke), 0);
    }

    // Relecture du lot 7 : autres lectures d'un mot écrit au bas de la page, six lectures. Le panneau reste
    // entier dans la couche (placeAlternatives le borne avec sa hauteur à jour).
    void alternativesStayInsideTheLayer()
    {
        Writing w;
        QVERIFY(w.open({{QStringLiteral("bonjour"), QStringLiteral("bonjours"), QStringLiteral("bonsoir"),
                         QStringLiteral("bonjou"), QStringLiteral("bonjoux"), QStringLiteral("bonjoul")}}));
        QQuickItem* panel = presentationharness::findItem(w.main.window()->contentItem(),
                                                          QStringLiteral("recognitionAlternatives"));
        QVERIFY(panel != nullptr);
        QQuickItem* layer = panel->parentItem();
        QVERIFY(layer != nullptr);
        const double bottom = w.controller->pageSize().height() - 4;
        w.controller->commitStroke(strokeThrough(inkLine(300, bottom - 30, 500, bottom)));
        w.now = atMs(w.recognition->delayMs());
        w.recognition->processDue();
        QTRY_COMPARE_WITH_TIMEOUT(countOf(*w.controller, core::ObjectKind::Text), 1, 10000);
        QTRY_VERIFY(panel->isVisible());
        QVERIFY(w.recognition->alternatives().size() >= 5);
        QVERIFY2(panel->y() >= 0 && panel->y() + panel->height() <= layer->height() + 0.5,
                 qPrintable(QStringLiteral("%1 + %2 / %3").arg(panel->y()).arg(panel->height()).arg(layer->height())));
    }

    // Moteur indisponible : mode refusé, commande grisée, explication du moteur gardée (info-bulle).
    void handwritingDisabledWithExplanation()
    {
        MainWindow main;
        QVERIFY(main.window() != nullptr);
        RecognitionController* recognition = recognitionOf(main);
        auto* commands = main.window()->findChild<Commands*>(QStringLiteral("commands"));
        QVERIFY(recognition != nullptr);
        QVERIFY(commands != nullptr);
        // Fenêtre déclarée Windows : ailleurs, les deux commandes sont absentes, donc grisées d'office.
        QVERIFY(declareWindows(main));
        installFake(*recognition, false, {}, QStringLiteral("Moteur d'essai absent"));
        QTRY_COMPARE_WITH_TIMEOUT(recognition->handwritingStatus(), QStringLiteral("Moteur d'essai absent"),
                                  10000);
        QVERIFY(!recognition->handwritingAvailable());
        QTRY_VERIFY(!commands->isEnabled(QStringLiteral("tool.handwriting")));
        QVERIFY(!commands->isEnabled(QStringLiteral("recognition.convert-to-text"))); // relecture du lot 7
        recognition->setHandwritingMode(true);
        QVERIFY(!recognition->handwritingMode()); // refusé : aucun trait ne partirait

        installFake(*recognition, true,
                    {{QStringLiteral("a")}}); // moteur disponible : la commande se dégrise
        QTRY_VERIFY(commands->isEnabled(QStringLiteral("tool.handwriting")));
        QVERIFY(commands->isEnabled(QStringLiteral("recognition.convert-to-text")));
        QVERIFY(recognition->handwritingAvailable());
    }

    // Absente d'Android (platforms de commands.json), présente sous Windows.
    void handwritingAbsentOnAndroid()
    {
        QTemporaryDir profile;
        Commands commands;
        commands.setProfileDir(profile.path());
        commands.setPlatform(actcraft::actions::Platform::Windows);
        QVERIFY(commands.isAvailable(QStringLiteral("tool.handwriting")));
        commands.setPlatform(actcraft::actions::Platform::Android);
        QVERIFY(!commands.isAvailable(QStringLiteral("tool.handwriting")));
        QVERIFY(!commands.trigger(QStringLiteral("tool.handwriting")));
    }

    // Un seul fil, créé une fois et gardé : le reconnaisseur y est créé, toutes les reconnaissances y
    // passent, jamais sur le fil principal ; fermer puis rouvrir le mode ne le recrée pas.
    void controllerOwnsOneHandwritingThread()
    {
        Writing w;
        QVERIFY(w.open({{QStringLiteral("un")}, {QStringLiteral("deux")}}));
        QThread* const thread = w.recognition->handwritingThread();
        QVERIFY(thread != nullptr);
        QVERIFY(thread != QThread::currentThread());
        QVERIFY(thread->isRunning());

        w.controller->commitStroke(strokeThrough(inkLine(100, 100, 200, 110)));
        w.now = atMs(w.recognition->delayMs());
        w.recognition->processDue();
        QTRY_COMPARE_WITH_TIMEOUT(countOf(*w.controller, core::ObjectKind::Text), 1, 10000);
        w.recognition->setHandwritingMode(false);
        w.recognition->setHandwritingMode(true);
        w.controller->commitStroke(strokeThrough(inkLine(500, 400, 600, 410)));
        w.now += atMs(w.recognition->delayMs());
        w.recognition->processDue();
        QTRY_COMPARE_WITH_TIMEOUT(countOf(*w.controller, core::ObjectKind::Text), 2, 10000);

        QCOMPARE(w.recognition->handwritingThread(), thread);
        const std::lock_guard lock(w.log->mutex);
        QCOMPARE(w.log->creations.size(), std::size_t{1});
        QCOMPARE(w.log->creations.front(), thread);
        QCOMPARE(w.log->recognitions.size(), std::size_t{2});
        for (QThread* const used : w.log->recognitions)
            QCOMPARE(used, thread);
    }

    // Deux mots séparés d'une pause : deux objets texte, une seule étape ; deux traits rapprochés : un mot.
    void eachWordBecomesItsOwnText()
    {
        Writing w;
        QVERIFY(w.open({{QStringLiteral("un")}, {QStringLiteral("deux")}, {QStringLiteral("trois")}}));
        QVERIFY(w.recognition->delayMs() > 800);

        w.controller->commitStroke(strokeThrough(inkLine(100, 100, 200, 110)));
        w.now = atMs(800); // pause entre les deux mots, plus courte que le délai
        w.controller->commitStroke(strokeThrough(inkLine(500, 100, 600, 110)));
        w.now = atMs(800 + w.recognition->delayMs());
        w.recognition->processDue();
        QTRY_COMPARE_WITH_TIMEOUT(countOf(*w.controller, core::ObjectKind::Text), 2, 10000);
        QCOMPARE(countOf(*w.controller, core::ObjectKind::Stroke), 0);
        const std::vector<const core::PageObject*> texts = textsOf(*w.controller);
        QVERIFY(htmlOf(*texts[0]).contains(QStringLiteral(">un</p>")));
        QVERIFY(htmlOf(*texts[1]).contains(QStringLiteral(">deux</p>")));
        QVERIFY(std::get<core::Text>(core::body(*texts[0])).bounds.x <
                std::get<core::Text>(core::body(*texts[1])).bounds.x);
        w.controller->undo(); // les deux mots d'un coup
        QCOMPARE(countOf(*w.controller, core::ObjectKind::Text), 0);
        QCOMPARE(countOf(*w.controller, core::ObjectKind::Stroke), 2);
        w.controller->redo();
        QCOMPARE(countOf(*w.controller, core::ObjectKind::Text), 2);

        w.now += atMs(20000);
        w.controller->commitStroke(strokeThrough(inkLine(100, 300, 200, 310)));
        w.now += atMs(100); // moins d'une pause : même mot
        w.controller->commitStroke(strokeThrough(inkLine(220, 300, 300, 310)));
        w.now += atMs(w.recognition->delayMs());
        w.recognition->processDue();
        QTRY_COMPARE_WITH_TIMEOUT(countOf(*w.controller, core::ObjectKind::Text), 3, 10000);
        QCOMPARE(countOf(*w.controller, core::ObjectKind::Stroke), 0);
        const std::lock_guard lock(w.log->mutex);
        QCOMPARE(w.log->recognitions.size(), std::size_t{3});
        QCOMPARE(w.log->requests.back().size(), std::size_t{2}); // deux traits, un appel
    }

    // Un trait loin du texte reconnu valide la lecture (les alternatives se ferment, le texte reste) ; un
    // trait qui le recoupe ne la valide pas.
    void clickOutsideValidatesTheText()
    {
        Writing w;
        QVERIFY(w.open({{QStringLiteral("bonjour"), QStringLiteral("bonjours")}}));
        w.controller->commitStroke(strokeThrough(inkLine(300, 300, 500, 310)));
        w.now = atMs(w.recognition->delayMs());
        w.recognition->processDue();
        QTRY_COMPARE_WITH_TIMEOUT(countOf(*w.controller, core::ObjectKind::Text), 1, 10000);
        QQuickItem* panel = w.main.item(QStringLiteral("recognitionAlternatives"));
        QVERIFY(panel != nullptr);
        QTRY_VERIFY(panel->isVisible());

        w.controller->commitStroke(strokeThrough(inkLine(350, 305, 450, 305))); // sur le texte
        QCOMPARE(w.recognition->alternatives().size(), 1);
        QVERIFY(panel->isVisible());

        w.controller->commitStroke(strokeThrough(inkLine(900, 600, 1000, 650))); // loin du texte
        QVERIFY(w.recognition->alternatives().isEmpty());
        QTRY_VERIFY(!panel->isVisible());
        QCOMPARE(countOf(*w.controller, core::ObjectKind::Text), 1); // le texte reste
    }

    void alternativesBlockTheStylus_data() { presentationharness::addPointerRows(); }

    // Les alternatives sont une couverture (FloatingPanel) : ni stylet, ni doigt, ni souris ne tracent
    // dessous ; un trait ailleurs sur la page est versé.
    void alternativesBlockTheStylus()
    {
        QFETCH(presentationharness::Pointer, pointer);
        Writing w;
        QVERIFY(w.open({{QStringLiteral("bonjour"), QStringLiteral("bonjours")}}));
        w.controller->commitStroke(strokeThrough(inkLine(300, 300, 500, 310)));
        w.now = atMs(w.recognition->delayMs());
        w.recognition->processDue();
        QTRY_COMPARE_WITH_TIMEOUT(countOf(*w.controller, core::ObjectKind::Text), 1, 10000);
        QQuickItem* panel = w.main.item(QStringLiteral("recognitionAlternatives"));
        QVERIFY(panel != nullptr);
        QTRY_VERIFY(panel->isVisible());
        QTest::qWait(100);
        const int before = w.main.pageObjectCount();
        QVERIFY(before >= 1);

        const QPointF inside = panel->mapToScene(QPointF(panel->width() * 0.5, panel->height() - 6));
        presentationharness::strokeAt(w.main.window(), pointer, inside, inside + QPointF(30, 0));
        QTest::qWait(150);
        QCOMPARE(w.main.pageObjectCount(), before);
        QCOMPARE(countOf(*w.controller, core::ObjectKind::Stroke), 0);

        // Hors du panneau (en bas à droite de la fenêtre) : le trait est versé.
        const QPointF outside(w.main.window()->width() * 0.8, w.main.window()->height() * 0.8);
        presentationharness::strokeAt(w.main.window(), pointer, outside, outside + QPointF(60, 0));
        QTRY_COMPARE_WITH_TIMEOUT(countOf(*w.controller, core::ObjectKind::Stroke), 1, 5000);
    }
};

} // namespace

ACTCRAFT_LOT7_TEST("recognition", TstUiRecognition)
#include "tst_ui_recognition.moc"
