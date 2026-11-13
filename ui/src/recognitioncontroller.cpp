#include "recognitioncontroller.h"

#include "presentationdocument.h"

#include <actcraft/features/shapefinder.h>

#include <QPointF>
#include <QRectF>

#include <algorithm>
#include <cmath>
#include <memory>
#include <optional>
#include <set>
#include <utility>
#include <variant>

namespace actcraft::ui {

namespace {

// Échéance relue tous les dixièmes de seconde tant qu'un trait attend (7b, L7-T21, étape 3).
constexpr int kPollMs = 100;
// Option de remplissage fermée : forme transparente, seul le contour se voit.
constexpr core::Argb kTransparent = 0x00000000u;
// Pause entre deux traits qui sépare deux mots (P17) : plus courte que le délai de reconnaissance.
constexpr std::int64_t kWordPauseNs = 500 * 1000000;
// Marge, en pixels de page, autour du texte reconnu : un trait qui la touche corrige le texte, il ne le
// valide pas.
constexpr double kValidationMargin = 5;
// Corps du texte posé, en points : déduit de la hauteur des traits (N2-3 pour la police), borné.
constexpr double kTextHeightRatio = 0.6;
constexpr double kMinPointSize = 10;
constexpr double kMaxPointSize = 96;

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

// Boîte, en pixels de page, d'un trait du stylo ; vide si le trait n'a aucun point.
QRectF pageBoundsOf(const core::PageObject& object, const core::Stroke& stroke)
{
    double left = 0, top = 0, right = 0, bottom = 0;
    bool first = true;
    for (const core::StrokePoint& p : stroke.points) {
        const core::PointF q = object.common.transform.map({p.x, p.y});
        left = first ? q.x : std::min(left, q.x);
        right = first ? q.x : std::max(right, q.x);
        top = first ? q.y : std::min(top, q.y);
        bottom = first ? q.y : std::max(bottom, q.y);
        first = false;
    }
    return first ? QRectF() : QRectF(left, top, right - left, bottom - top);
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
    m_thread.reset(); // d'abord : le fil attend sa fin, ses réponses n'ont plus de receveur
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
    disconnect(m_liftingConnection);
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
        // Fin d'un soulèvement : un trait gommé en entier ne laisse aucun changement au dépôt
        // (core::changesAfterLift ne rend pas le retrait d'un objet soulevé), donc rien n'est diffusé.
        m_liftingConnection = connect(controller, &DocumentController::liftingChanged, this,
                                      &RecognitionController::forgetArrivalsOffTheDocument);
        m_observer = controller->addChangeObserver(
            [this](const core::Changes& applied, DocumentController::ChangeOrigin origin) {
                onChanges(applied, origin);
            });
    }
    emit documentControllerChanged();
}

ToolHost* RecognitionController::tools() const noexcept
{
    return m_tools;
}

void RecognitionController::setTools(ToolHost* tools)
{
    if (m_tools == tools) {
        return;
    }
    m_tools = tools;
    emit toolsChanged();
}

std::vector<core::ObjectId> RecognitionController::selectedObjects() const
{
    // Sans les objets que l'autre utilisateur tient (F-303, comme les panneaux de L11-C10 G).
    return m_tools ? m_tools->usableSelection() : std::vector<core::ObjectId>{};
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
    if (on) {
        setHandwritingMode(false); // exclusifs : un seul des deux reçoit les traits
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

bool RecognitionController::handwritingMode() const noexcept
{
    return m_handwritingMode;
}

void RecognitionController::setHandwritingMode(bool on)
{
    if (m_handwritingMode == on || (on && !m_available)) {
        return;
    }
    if (on) {
        setShapeMode(false);
    }
    m_handwritingMode = on;
    if (!on) {
        resetSession();
    }
    emit handwritingModeChanged();
}

bool RecognitionController::handwritingAvailable() const noexcept
{
    return m_available;
}

QString RecognitionController::handwritingStatus() const
{
    return m_status;
}

QStringList RecognitionController::alternatives() const
{
    QStringList others = m_readings;
    if (m_current >= 0 && m_current < others.size()) {
        others.removeAt(m_current);
    }
    return others;
}

QRectF RecognitionController::alternativesAnchor() const
{
    return m_anchor;
}

QThread* RecognitionController::handwritingThread() const noexcept
{
    return m_thread.get();
}

void RecognitionController::setHandwritingFactoryForTest(features::HandwritingThread::Factory factory)
{
    m_factory = std::move(factory);
    startThread();
    probeHandwriting();
}

void RecognitionController::startThread()
{
    ++m_threadGen; // les réponses en vol de l'ancien fil sont abandonnées
    m_thread.reset();
    if (m_available || !m_status.isEmpty()) {
        m_available = false;
        m_status.clear();
        emit handwritingProbed();
    }
    if (m_handwritingMode) {
        setHandwritingMode(false);
    }
    m_thread = m_factory ? std::make_unique<features::HandwritingThread>(m_factory)
                         : std::make_unique<features::HandwritingThread>();
}

void RecognitionController::probeHandwriting()
{
    if (!m_thread) {
        startThread();
    }
    const std::uint64_t gen = m_threadGen;
    m_thread->probe(this, [this, gen](const features::RecognitionReply& reply) {
        if (gen != m_threadGen) {
            return;
        }
        m_available = reply.available;
        m_status = reply.status;
        if (!m_available) {
            setHandwritingMode(false);
        }
        emit handwritingProbed();
    });
}

void RecognitionController::validate()
{
    closeAlternatives();
}

void RecognitionController::chooseAlternative(int index)
{
    const QStringList others = alternatives();
    if (!m_document || m_lastText == 0 || index < 0 || index >= others.size()) {
        return;
    }
    DocumentInsertions insertions(*m_document);
    const core::Page* page = insertions.shownPage();
    if (!page || !objectOf(*page, m_lastText)) {
        closeAlternatives();
        return;
    }
    const int chosen = static_cast<int>(m_readings.indexOf(others.at(index)));
    std::vector<core::PageObject> added;
    added.push_back(textFor(m_anchor, m_readings.at(chosen)));
    const std::vector<core::ObjectId> ids = insertions.replaceObjects({m_lastText}, std::move(added));
    if (ids.empty()) {
        closeAlternatives();
        return;
    }
    m_lastText = ids.front();
    m_current = chosen;
    emit alternativesChanged();
}

void RecognitionController::closeAlternatives()
{
    if (m_readings.isEmpty() && m_lastText == 0) {
        return;
    }
    m_readings.clear();
    m_current = 0;
    m_lastText = 0;
    m_anchor = QRectF();
    emit alternativesChanged();
}

void RecognitionController::showAlternatives(QStringList readings, QRectF bounds, core::ObjectId text)
{
    m_readings = std::move(readings);
    m_current = 0;
    m_anchor = bounds;
    m_lastText = text;
    emit alternativesChanged();
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
        dispatch(due);
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
    dispatch(pending);
    updateTimer();
}

void RecognitionController::convertToShape()
{
    // A posteriori (F-296) : la sélection d'abord, quel que soit le mode ; les traits en attente qui n'en
    // sont pas gardent leur délai.
    const std::vector<core::ObjectId> selected = selectedObjects();
    if (selected.empty()) {
        convertPending();
        return;
    }
    forgetPending(selected);
    recognizeShapes(selected);
}

void RecognitionController::convertToText()
{
    if (!m_thread || !m_available) {
        return; // aucun reconnaisseur : rien n'est lu, rien n'est retiré de la session
    }
    std::vector<core::ObjectId> strokes = selectedObjects(); // a posteriori (F-295), quel que soit le mode
    if (strokes.empty()) {
        strokes = m_session.pending();
        if (strokes.empty()) {
            return;
        }
        m_session.clear();
        emit pendingCountChanged();
        updateTimer();
    } else {
        forgetPending(strokes);
    }
    recognizeHandwriting(strokes);
}

// Un trait en attente converti par la sélection quitte la session : sinon il resterait compté en attente, et
// son délai courrait encore après la conversion.
void RecognitionController::forgetPending(const std::vector<core::ObjectId>& converted)
{
    const std::size_t before = m_session.pending().size();
    m_session.remove(converted);
    if (m_session.pending().size() != before) {
        emit pendingCountChanged();
        updateTimer();
    }
}

// Par identifiant, sur le document entier : pendant la diffusion des changements, currentIndex() vaut encore
// l'indice d'avant (DocumentController::settle), qui désigne une autre page, ou aucune, après un ajout, un
// déplacement ou un retrait de page. Pendant un soulèvement, les traits soulevés sont hors du document le
// temps du geste : rien n'est purgé, le dépôt (Drop, puis liftingChanged) le fera.
void RecognitionController::forgetArrivalsOffTheDocument()
{
    if (m_arrivedAt.empty() || !m_document || m_document->isLifting()) {
        return;
    }
    const core::Document& document = m_document->document();
    std::set<core::ObjectId> present;
    for (std::size_t i = 0; i < document.pageCount() && present.size() < m_arrivedAt.size(); ++i) {
        for (const core::PageObject& object : document.page(i).objects) {
            if (m_arrivedAt.contains(object.common.id)) {
                present.insert(object.common.id);
            }
        }
    }
    std::erase_if(m_arrivedAt, [&present](const auto& arrival) { return !present.contains(arrival.first); });
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
    // Un trait retiré du document (gomme au dépôt, commande) sort de la table des arrivées ; pas pendant un
    // soulèvement (voir forgetArrivalsOffTheDocument).
    if (origin == DocumentController::ChangeOrigin::Command ||
        origin == DocumentController::ChangeOrigin::Drop) {
        forgetArrivalsOffTheDocument();
    }
    if (origin != DocumentController::ChangeOrigin::Stroke || (!m_shapeMode && !m_handwritingMode) ||
        !m_document) {
        return;
    }
    const core::Document& document = m_document->document();
    bool added = false;
    bool touchesText = false;
    for (const core::Change& change : applied) {
        if (change.kind != core::Change::Kind::ObjectInserted || change.object == 0) {
            continue;
        }
        const core::PageObject* object = objectOf(document.page(change.page), change.object);
        if (!penStrokeOf(object)) {
            continue;
        }
        touchesText = touchesText || pageBoundsOf(*object, *penStrokeOf(object))
                                         .adjusted(-kValidationMargin, -kValidationMargin, kValidationMargin,
                                                   kValidationMargin)
                                         .intersects(m_anchor.adjusted(-kValidationMargin, -kValidationMargin,
                                                                       kValidationMargin, kValidationMargin));
        m_session.addStroke(change.object, now());
        m_arrivedAt[change.object] = now(); // lu par recognizeHandwriting, quel que soit le mode du tracé
        added = true;
    }
    // P19 : un trait loin du texte reconnu le valide ; un trait qui le recoupe le corrige, rien ne se ferme.
    if (added && m_handwritingMode && !touchesText) {
        closeAlternatives();
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
    m_arrivedAt.clear();
    ++m_epoch; // une reconnaissance d'écriture en vol est abandonnée
    closeAlternatives();
    if (had) {
        emit pendingCountChanged();
    }
    updateTimer();
}

void RecognitionController::dispatch(const std::vector<core::ObjectId>& strokes)
{
    if (m_handwritingMode) {
        recognizeHandwriting(strokes);
    } else {
        recognizeShapes(strokes);
    }
}

core::PageObject RecognitionController::textFor(const QRectF& bounds, const QString& text) const
{
    const double points =
        std::clamp(std::round(bounds.height() * kTextHeightRatio), kMinPointSize, kMaxPointSize);
    core::Text body;
    body.bounds = rectOf(bounds);
    body.bounds.height = std::max(bounds.height(), points * 1.5);
    body.html =
        QStringLiteral("<p style=\"font-family:sans-serif; font-size:%1pt; white-space:pre-wrap\">%2</p>")
            .arg(points)
            .arg(text.toHtmlEscaped())
            .toStdString();
    core::PageObject object = std::move(body);
    object.common.layer = core::Layer::Middle;
    return object;
}

void RecognitionController::recognizeHandwriting(const std::vector<core::ObjectId>& strokes)
{
    struct Word
    {
        std::vector<core::ObjectId> ids;
        features::InkStrokes ink;
        QRectF bounds;
    };
    struct Batch
    {
        std::vector<Word> words;
        std::vector<QStringList> readings;
        std::size_t remaining = 0;
    };
    if (!m_document || !m_thread || strokes.empty()) {
        return;
    }
    DocumentInsertions insertions(*m_document);
    const core::Page* page = insertions.shownPage();
    if (!page) {
        return;
    }
    auto batch = std::make_shared<Batch>();
    std::int64_t lastAt = 0;
    for (const core::ObjectId id : strokes) {
        const core::PageObject* object = objectOf(*page, id);
        const core::Stroke* stroke = penStrokeOf(object);
        const auto arrived = m_arrivedAt.find(id);
        const std::int64_t at = arrived != m_arrivedAt.end() ? arrived->second : 0;
        if (!stroke) {
            continue;
        }
        // Une pause plus longue que celle d'un mot : le trait ouvre un nouveau mot.
        if (batch->words.empty() || at - lastAt > kWordPauseNs) {
            batch->words.emplace_back();
        }
        lastAt = at;
        Word& word = batch->words.back();
        word.ids.push_back(id);
        std::vector<QPointF> points;
        points.reserve(stroke->points.size());
        for (const core::StrokePoint& p : stroke->points) {
            const core::PointF onPage = object->common.transform.map({p.x, p.y});
            points.emplace_back(onPage.x, onPage.y);
        }
        word.ink.push_back(std::move(points));
        const QRectF box = pageBoundsOf(*object, *stroke);
        word.bounds = word.bounds.united(box);
    }
    for (const core::ObjectId id : strokes) {
        m_arrivedAt.erase(id);
    }
    if (batch->words.empty()) {
        return;
    }
    batch->readings.resize(batch->words.size());
    batch->remaining = batch->words.size();
    const std::uint64_t epoch = m_epoch;
    const std::uint64_t gen = m_threadGen;
    for (std::size_t i = 0; i < batch->words.size(); ++i) {
        m_thread->recognize(batch->words[i].ink, this,
                            [this, batch, i, epoch, gen](const features::RecognitionReply& reply) {
                                if (epoch != m_epoch || gen != m_threadGen || !m_document) {
                                    return; // session remise à zéro ou fil recréé entre-temps
                                }
                                batch->readings[i] = reply.texts;
                                if (--batch->remaining != 0) {
                                    return;
                                }
                                // Tous les mots sont revenus : un seul remplacement, une seule étape.
                                DocumentInsertions replacing(*m_document);
                                const core::Page* shown = replacing.shownPage();
                                if (!shown) {
                                    return;
                                }
                                std::vector<core::ObjectId> removed;
                                std::vector<core::PageObject> added;
                                std::size_t last = batch->words.size();
                                for (std::size_t w = 0; w < batch->words.size(); ++w) {
                                    const Word& word = batch->words[w];
                                    const bool present =
                                        std::all_of(word.ids.begin(), word.ids.end(), [&](core::ObjectId id) {
                                            return penStrokeOf(objectOf(*shown, id)) != nullptr;
                                        });
                                    if (batch->readings[w].isEmpty() || !present) {
                                        continue;
                                    }
                                    removed.insert(removed.end(), word.ids.begin(), word.ids.end());
                                    added.push_back(textFor(word.bounds, batch->readings[w].front()));
                                    last = w;
                                }
                                if (removed.empty()) {
                                    return;
                                }
                                const std::size_t count = added.size();
                                const std::vector<core::ObjectId> ids =
                                    replacing.replaceObjects(std::move(removed), std::move(added));
                                showAlternatives(batch->readings[last], batch->words[last].bounds,
                                                 ids.size() == count ? ids.back() : 0);
                            });
    }
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
    // Remplacés par une forme : plus suivis. Les traits restés traits gardent leur heure d'arrivée.
    for (const core::ObjectId id : removed) {
        m_arrivedAt.erase(id);
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
