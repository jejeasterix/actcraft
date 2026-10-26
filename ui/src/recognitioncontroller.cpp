#include "recognitioncontroller.h"

#include "presentationdocument.h"

#include <actcraft/features/shapefinder.h>

#include <QPointF>
#include <QRectF>

#include <algorithm>
#include <optional>
#include <utility>
#include <variant>

namespace actcraft::ui {

namespace {

// Échéance relue tous les dixièmes de seconde tant qu'un trait attend (7b, L7-T21, étape 3).
constexpr int kPollMs = 100;
// Option de remplissage fermée : forme transparente, seul le contour se voit.
constexpr core::Argb kTransparent = 0x00000000u;

QString kindName(features::RecognizedShape::Kind kind)
{
    using Kind = features::RecognizedShape::Kind;
    switch (kind) {
    case Kind::Line:
        return QStringLiteral("line");
    case Kind::Rectangle:
        return QStringLiteral("rectangle");
    case Kind::Square:
        return QStringLiteral("square");
    case Kind::Ellipse:
        return QStringLiteral("ellipse");
    case Kind::Circle:
        return QStringLiteral("circle");
    case Kind::Triangle:
        return QStringLiteral("triangle");
    case Kind::Polygon:
        return QStringLiteral("polygon");
    }
    return {};
}

core::PointF pointOf(QPointF p)
{
    return {p.x(), p.y()};
}

core::RectF rectOf(const QRectF& r)
{
    return {r.x(), r.y(), r.width(), r.height()};
}

// Rotation de `degrees` autour du centre de `bounds` : repère de la forme → page (sens de core::rotation).
core::Transform turnedAbout(const QRectF& bounds, double degrees)
{
    if (degrees == 0) {
        return {};
    }
    const QPointF c = bounds.center();
    return core::compose(core::compose(core::translation(-c.x(), -c.y()), core::rotation(degrees)),
                         core::translation(c.x(), c.y()));
}

// Forme posée à la place du trait, couche intermédiaire : contour de la couleur et de l'épaisseur du trait ;
// remplissage de la couleur choisie, sinon de celle du contour, si l'option est ouverte.
core::PageObject shapeFor(const features::RecognizedShape& found, const core::Stroke& stroke, bool fill,
                          const QColor& fillColor)
{
    using Kind = features::RecognizedShape::Kind;
    core::ObjectCommon common;
    common.layer = core::Layer::Middle;
    if (found.kind == Kind::Line) {
        core::Line line;
        for (const QPointF& p : found.corners) {
            line.points.push_back(pointOf(p));
        }
        line.pen.color = stroke.color;
        line.pen.width = stroke.baseWidth;
        core::PageObject object(std::move(line));
        object.common = std::move(common);
        return object;
    }
    core::Shape shape;
    shape.bounds = rectOf(found.bounds);
    shape.outline = stroke.color;
    shape.outlineWidth = stroke.baseWidth;
    shape.fill = !fill                 ? kTransparent
                 : fillColor.isValid() ? static_cast<core::Argb>(fillColor.rgba())
                                       : stroke.color;
    switch (found.kind) {
    case Kind::Rectangle:
    case Kind::Square:
        shape.kind = core::ShapeKind::Rectangle;
        common.transform = turnedAbout(found.bounds, found.angleDegrees);
        break;
    case Kind::Ellipse:
    case Kind::Circle:
        shape.kind = core::ShapeKind::Ellipse;
        common.transform = turnedAbout(found.bounds, found.angleDegrees);
        break;
    case Kind::Line:
    case Kind::Triangle:
    case Kind::Polygon:
        // Sommets en coordonnées de page, dans l'ordre du tracé : le peintre lit Shape::polygon tel quel.
        shape.kind = core::ShapeKind::Polygon;
        for (const QPointF& p : found.corners) {
            shape.polygon.push_back(pointOf(p));
        }
        break;
    }
    core::PageObject object(std::move(shape));
    object.common = std::move(common);
    return object;
}

const core::PageObject* objectOf(const core::Page& page, core::ObjectId id)
{
    const auto found = std::find_if(page.objects.begin(), page.objects.end(),
                                    [id](const core::PageObject& object) { return object.common.id == id; });
    return found != page.objects.end() ? &*found : nullptr;
}

// Trait du stylo seulement : le marqueur (commitAnnotation, même origine) n'est jamais reconnu.
const core::Stroke* penStrokeOf(const core::PageObject* object)
{
    const core::Stroke* stroke = object ? std::get_if<core::Stroke>(&core::body(*object)) : nullptr;
    return stroke && stroke->kind == core::StrokeKind::Pen ? stroke : nullptr;
}

} // namespace

RecognitionController::RecognitionController(QObject* parent) : QObject(parent)
{
    m_elapsed.start();
    m_timer.setInterval(kPollMs);
    connect(&m_timer, &QTimer::timeout, this, &RecognitionController::processDue);
}

RecognitionController::~RecognitionController()
{
    if (m_document && m_observer > 0) {
        m_document->removeChangeObserver(m_observer);
    }
}

DocumentController* RecognitionController::documentController() const noexcept
{
    return m_document;
}

void RecognitionController::setDocumentController(DocumentController* controller)
{
    if (m_document == controller) {
        return;
    }
    disconnect(m_replacedConnection);
    if (m_document && m_observer > 0) {
        m_document->removeChangeObserver(m_observer);
    }
    m_observer = 0;
    m_document = controller;
    resetSession();
    if (controller) {
        // Document remplacé (ouverture, « Nouveau », banc) : ses identifiants ne sont plus ceux de la
        // session. Jamais documentChanged : il part après chaque trait et viderait la session que ce trait
        // remplit.
        m_replacedConnection = connect(controller, &DocumentController::documentReplaced, this,
                                       &RecognitionController::resetSession);
        m_observer = controller->addChangeObserver(
            [this](const core::Changes& applied, DocumentController::ChangeOrigin origin) {
                onChanges(applied, origin);
            });
    }
    emit documentControllerChanged();
}

bool RecognitionController::shapeMode() const noexcept
{
    return m_shapeMode;
}

void RecognitionController::setShapeMode(bool on)
{
    if (m_shapeMode == on) {
        return;
    }
    m_shapeMode = on;
    if (!on) {
        resetSession();
    }
    emit shapeModeChanged();
}

bool RecognitionController::fillShapes() const noexcept
{
    return m_fillShapes;
}

void RecognitionController::setFillShapes(bool on)
{
    if (m_fillShapes == on) {
        return;
    }
    m_fillShapes = on;
    emit fillShapesChanged();
}

QColor RecognitionController::fillColor() const
{
    return m_fillColor;
}

void RecognitionController::setFillColor(const QColor& color)
{
    if (m_fillColor == color) {
        return;
    }
    m_fillColor = color;
    emit fillColorChanged();
}

int RecognitionController::delayMs() const noexcept
{
    return m_session.delayMs();
}

void RecognitionController::setDelayMs(int delayMs)
{
    const int before = m_session.delayMs();
    m_session.setDelayMs(delayMs);
    if (m_session.delayMs() != before) {
        emit delayMsChanged();
    }
}

QString RecognitionController::lastKind() const
{
    return m_lastKind;
}

int RecognitionController::pendingCount() const noexcept
{
    return static_cast<int>(m_session.pending().size());
}

void RecognitionController::setClock(Clock clock)
{
    m_clock = std::move(clock);
}

void RecognitionController::processDue()
{
    const std::vector<core::ObjectId> due = m_session.due(now());
    if (!due.empty()) {
        emit pendingCountChanged();
        recognizeShapes(due);
    }
    updateTimer();
}

void RecognitionController::convertPending()
{
    const std::vector<core::ObjectId> pending = m_session.pending();
    if (pending.empty()) {
        return;
    }
    m_session.clear();
    emit pendingCountChanged();
    recognizeShapes(pending);
    updateTimer();
}

void RecognitionController::onChanges(const core::Changes& applied, DocumentController::ChangeOrigin origin)
{
    // Un Annuler (ou Rétablir) pendant le délai abandonne les traits en attente : une conversion poussée
    // ensuite passerait par UndoStack::record et effacerait la branche à rétablir.
    if (origin == DocumentController::ChangeOrigin::Undo ||
        origin == DocumentController::ChangeOrigin::Redo) {
        resetSession();
        return;
    }
    if (origin != DocumentController::ChangeOrigin::Stroke || !m_shapeMode || !m_document) {
        return;
    }
    const core::Document& document = m_document->document();
    bool added = false;
    for (const core::Change& change : applied) {
        if (change.kind != core::Change::Kind::ObjectInserted || change.object == 0 ||
            !penStrokeOf(objectOf(document.page(change.page), change.object))) {
            continue;
        }
        m_session.addStroke(change.object, now());
        added = true;
    }
    if (!added) {
        return;
    }
    emit pendingCountChanged();
    // Aucune commande pendant la diffusion des changements (contrat de addChangeObserver) : un délai nul
    // convertit au tour suivant de la boucle, juste après le relâché.
    if (m_session.delayMs() == 0) {
        QTimer::singleShot(0, this, &RecognitionController::processDue);
    }
    updateTimer();
}

void RecognitionController::resetSession()
{
    const bool had = !m_session.pending().empty();
    m_session.clear();
    if (had) {
        emit pendingCountChanged();
    }
    updateTimer();
}

void RecognitionController::recognizeShapes(const std::vector<core::ObjectId>& strokes)
{
    if (!m_document || strokes.empty()) {
        return;
    }
    DocumentInsertions insertions(*m_document);
    const core::Page* page = insertions.shownPage();
    if (!page) {
        return;
    }
    std::vector<core::ObjectId> removed;
    std::vector<core::PageObject> added;
    QString kind;
    for (const core::ObjectId id : strokes) {
        // Trait annulé, ou resté sur une page quittée entre-temps : rien à remplacer ici.
        const core::PageObject* object = objectOf(*page, id);
        const core::Stroke* stroke = penStrokeOf(object);
        if (!stroke) {
            continue;
        }
        std::vector<QPointF> points;
        points.reserve(stroke->points.size());
        for (const core::StrokePoint& p : stroke->points) {
            const core::PointF onPage = object->common.transform.map({p.x, p.y});
            points.emplace_back(onPage.x, onPage.y);
        }
        const std::optional<features::RecognizedShape> found = features::recognizeShape(points);
        if (!found) {
            continue;
        }
        removed.push_back(id);
        added.push_back(shapeFor(*found, *stroke, m_fillShapes, m_fillColor));
        kind = kindName(found->kind);
    }
    if (removed.empty()) {
        return;
    }
    // Une étape : Annuler rend les traits d'un coup (core::ReplaceObjectsCommand par DocumentInsertions).
    insertions.replaceObjects(std::move(removed), std::move(added));
    if (kind != m_lastKind) {
        m_lastKind = kind;
        emit lastKindChanged();
    }
}

void RecognitionController::updateTimer()
{
    if (m_session.pending().empty()) {
        m_timer.stop();
    } else if (!m_timer.isActive()) {
        m_timer.start();
    }
}

std::int64_t RecognitionController::now() const
{
    return m_clock ? m_clock() : m_elapsed.nsecsElapsed();
}

} // namespace actcraft::ui
