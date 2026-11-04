#include <actcraft/core/clipboardedits.h>
#include <actcraft/core/connectors.h>
#include <actcraft/core/document.h>
#include <actcraft/core/edits.h>
#include <actcraft/core/gridsnap.h>
#include <actcraft/core/hittest.h>
#include <actcraft/core/objectedits.h>
#include <actcraft/core/shapeedits.h>
#include <actcraft/core/shapegeometry.h>
#include <actcraft/core/shapepoints.h>
#include <actcraft/tools/selectiongeometry.h>
#include <actcraft/tools/selecttool.h>
#include <actcraft/tools/userstate.h>

#include <QtCore/qnamespace.h>

#include <algorithm>
#include <cmath>
#include <memory>
#include <numbers>
#include <optional>
#include <utility>

namespace actcraft::tools {

namespace {

// Glissement : au-delà de 2 px d'écran (D6, P27) ; un clic ne soulève rien.
constexpr double kDragThresholdScreen = 2.0;
constexpr int kExtendModifiers = Qt::ShiftModifier | Qt::ControlModifier;
constexpr double kSnapStepDegrees = 15.0; // rotation avec Maj
constexpr double kMinExtent = 1.0; // plus petite largeur ou hauteur d'un redimensionnement (pixels de page)

double toDegrees(double radians) noexcept
{
    return radians * 180.0 / std::numbers::pi;
}

// Signe horizontal (-1 gauche, +1 droite, 0 aucun) et vertical (-1 haut, +1 bas) d'une poignée.
int horizontalSign(HandleKind handle) noexcept
{
    switch (handle) {
    case HandleKind::TopLeft:
    case HandleKind::BottomLeft:
    case HandleKind::Left:
        return -1;
    case HandleKind::TopRight:
    case HandleKind::BottomRight:
    case HandleKind::Right:
        return 1;
    case HandleKind::Top:
    case HandleKind::Bottom:
        return 0;
    }
    return 0;
}

int verticalSign(HandleKind handle) noexcept
{
    switch (handle) {
    case HandleKind::TopLeft:
    case HandleKind::Top:
    case HandleKind::TopRight:
        return -1;
    case HandleKind::BottomLeft:
    case HandleKind::Bottom:
    case HandleKind::BottomRight:
        return 1;
    case HandleKind::Left:
    case HandleKind::Right:
        return 0;
    }
    return 0;
}

bool contains(const std::vector<core::ObjectId>& ids, core::ObjectId id)
{
    return std::find(ids.begin(), ids.end(), id) != ids.end();
}

// Verrouillé ou non déplaçable : sélectionné, jamais déplacé (F-210, P28).
// Mode conception (lockIgnored, F-560) : le verrou ne retient plus, les restrictions si.
bool movable(const core::PageObject& object, bool lockIgnored)
{
    return (lockIgnored || !object.common.locked) && object.common.restrictions.movable;
}

// Dimensionnable : la règle des poignées (layoutSelection, beginTransform ; P28, P30), pas celle du glisser.
bool resizable(const core::PageObject& object, bool lockIgnored = false)
{
    return (lockIgnored || !object.common.locked) && object.common.restrictions.resizable;
}

// Pivotable : la même règle pour le bouton de rotation (beginTransform ; P28).
bool rotatable(const core::PageObject& object, bool lockIgnored)
{
    return (lockIgnored || !object.common.locked) && object.common.restrictions.rotatable;
}

// Parmi `ids` (présents), ceux qu'Agrandir et Réduire changent (M1, L3-C9).
std::vector<core::ObjectId> resizableAmong(const core::Page& page, const std::vector<core::ObjectId>& ids)
{
    std::vector<core::ObjectId> out;
    for (const core::ObjectId id : ids) {
        if (resizable(page.objects[core::indexOf(page, id)])) {
            out.push_back(id);
        }
    }
    return out;
}

// Identifiants encore sur la page et sélectionnables, dans l'ordre reçu.
std::vector<core::ObjectId> present(const core::Page& page, const std::vector<core::ObjectId>& ids)
{
    std::vector<core::ObjectId> out;
    for (const core::ObjectId id : ids) {
        const std::size_t index = core::indexOf(page, id);
        if (index != page.objects.size() && core::isSelectable(page, page.objects[index])) {
            out.push_back(id);
        }
    }
    return out;
}

// Point de forme sous `point` (le plus proche, au rayon d'une poignée) ; aucun hors de ce rayon.
std::optional<std::size_t> shapePointAt(const std::vector<core::PointF>& points, core::PointF point,
                                        double zoom)
{
    const double radius = kHandleRadiusScreen / zoom;
    std::optional<std::size_t> found;
    double best = radius;
    for (std::size_t i = 0; i < points.size(); ++i) {
        const double distance = std::hypot(point.x - points[i].x, point.y - points[i].y);
        if (distance <= best) {
            best = distance;
            found = i;
        }
    }
    return found;
}

// Dépôt d'un glissement mêlé (D9) : objets ordinaires transformés, objets à copier laissés en place et copiés
// ; une étape.
class MoveAndPasteCommand final : public core::UndoCommand
{
public:
    MoveAndPasteCommand(std::unique_ptr<core::UndoCommand> move, std::unique_ptr<core::UndoCommand> paste)
        : m_move(std::move(move)), m_paste(std::move(paste))
    {}
    void redo(core::Document& document, core::Changes& changes) override
    {
        m_move->redo(document, changes);
        m_paste->redo(document, changes);
    }
    void undo(core::Document& document, core::Changes& changes) override
    {
        m_paste->undo(document, changes);
        m_move->undo(document, changes);
    }

private:
    std::unique_ptr<core::UndoCommand> m_move;
    std::unique_ptr<core::UndoCommand> m_paste;
};

// Commande du dépôt d'un glissement (lot 3 et D9) : `lifted` transformés par `delta`, sauf ceux de `copied`,
// qui restent et dont la fermeture `sources` est collée à la position de dépôt (PasteObjectsCommand, L5-T4).
std::unique_ptr<core::UndoCommand> dragDropCommand(std::size_t page,
                                                   const std::vector<core::ObjectId>& lifted,
                                                   const std::vector<core::ObjectId>& copied,
                                                   std::vector<core::PageObject> sources,
                                                   const core::Transform& delta)
{
    std::vector<core::ObjectId> moved;
    for (const core::ObjectId id : lifted) {
        if (!contains(copied, id)) {
            moved.push_back(id);
        }
    }
    std::unique_ptr<core::UndoCommand> move;
    if (!moved.empty()) {
        move = std::make_unique<core::TransformObjectsCommand>(page, std::move(moved), delta);
    }
    if (copied.empty()) {
        return move;
    }
    auto paste = std::make_unique<core::PasteObjectsCommand>(
        page, core::preparePaste(std::move(sources), delta.dx, delta.dy));
    if (!move) {
        return paste;
    }
    return std::make_unique<MoveAndPasteCommand>(std::move(move), std::move(paste));
}

} // namespace

SelectTool::SelectTool(ToolContext& context) : m_context(context) {}

QString SelectTool::id() const
{
    return QStringLiteral("select");
}

void SelectTool::activate()
{
    showFrame();
}

void SelectTool::deactivate()
{
    cancel();
    m_shapePointsMode = false;
    publish(SelectionView{}); // la sélection reste dans UserState, le cadre se masque
}

void SelectTool::onSample(const input::Sample& sample)
{
    switch (sample.phase) {
    case input::Phase::Press:
        if (m_state == State::Idle) {
            press(sample);
        }
        break;
    case input::Phase::Move:
        if (m_state != State::Idle && sample.pointer == m_pointer) {
            move(sample);
        }
        break;
    case input::Phase::Release:
        if (m_state != State::Idle && sample.pointer == m_pointer) {
            release(sample);
        }
        break;
    }
}

bool SelectTool::isBusy() const noexcept
{
    return m_state != State::Idle;
}

void SelectTool::cancel()
{
    switch (m_state) {
    case State::Idle:
        return;
    case State::Pressed:
        reset();
        return;
    case State::RubberBand:
        reset();
        showFrame(); // le cadre sans rectangle
        return;
    case State::Dragging:
    case State::Resizing:
    case State::Rotating:
        endDrag(m_start, true);
        return;
    case State::SmartPoint:
        endSmartPoint(true);
        return;
    case State::ShapePoint:
        endShapePoint(true);
        return;
    }
}

bool SelectTool::onKey(int key, int modifiers)
{
    m_modifiers = modifiers;
    if (key == Qt::Key_Escape) {
        if (m_state == State::Idle) {
            if (!m_shapePointsMode) {
                return false;
            }
            setShapePointsMode(false); // Échap au repos quitte le mode points de forme
            return true;
        }
        cancel();
        return true;
    }
    if (key == Qt::Key_Delete) {
        if (m_context.user.selection().empty()) {
            return false;
        }
        deleteSelection();
        return true;
    }
    return false;
}

void SelectTool::selectAll()
{
    std::vector<core::ObjectId> ids;
    for (const core::PageObject& object : m_context.document.page().objects) {
        if (core::isSelectable(object)) {
            ids.push_back(object.common.id);
        }
    }
    m_context.user.setSelection(std::move(ids));
    showFrame();
}

void SelectTool::clearSelection()
{
    m_context.user.setSelection({});
    showFrame();
}

void SelectTool::deleteSelection()
{
    const std::vector<core::ObjectId> ids = present(m_context.document.page(), m_context.user.selection());
    if (ids.empty()) {
        return; // F-541 : sans sélection, rien
    }
    m_context.document.execute(
        std::make_unique<core::RemoveObjectsCommand>(m_context.document.pageIndex(), ids));
    m_context.user.setSelection({});
    showFrame();
}

void SelectTool::enlarge()
{
    scaleSelection(kEnlargeStep);
}

void SelectTool::reduce()
{
    scaleSelection(1.0 / kEnlargeStep);
}

void SelectTool::scaleSelection(double factor)
{
    const core::Page& page = m_context.document.page();
    const std::vector<core::ObjectId> ids = present(page, m_context.user.selection());
    const std::vector<core::ObjectId> scaled = resizableAmong(page, ids);
    if (scaled.empty()) {
        return;
    }
    const core::PointF center = core::selectionCenter(page, ids); // centre du cadre, comme les poignées
    m_context.document.execute(std::make_unique<core::TransformObjectsCommand>(
        m_context.document.pageIndex(), scaled, core::scalingAbout(center, factor, factor)));
    showFrame();
}

void SelectTool::setTranslucency(std::uint8_t value)
{
    const std::vector<core::ObjectId> ids = present(m_context.document.page(), m_context.user.selection());
    if (ids.empty()) {
        return;
    }
    m_context.document.execute(
        std::make_unique<core::SetTranslucencyCommand>(m_context.document.pageIndex(), ids, value));
}

void SelectTool::refresh()
{
    std::vector<core::ObjectId> kept = present(m_context.document.page(), m_context.user.selection());
    if (kept != m_context.user.selection()) {
        m_context.user.setSelection(std::move(kept));
    }
    showFrame();
}

void SelectTool::setShapePointsMode(bool on)
{
    if (m_state != State::Idle) {
        cancel();
    }
    m_shapePointsMode = on;
    showFrame();
}

bool SelectTool::shapePointsMode() const noexcept
{
    return m_shapePointsMode;
}

std::vector<core::PointF> SelectTool::shownShapePoints(const std::vector<core::ObjectId>& selection) const
{
    if (!m_shapePointsMode || selection.size() != 1) {
        return {};
    }
    const core::Page& page = m_context.document.page();
    const std::size_t index = core::indexOf(page, selection.front());
    if (index == page.objects.size()) {
        return {};
    }
    return core::shapePoints(page.objects[index]); // L5-T9 : sommets puis milieux, vide hors d'une forme
}

void SelectTool::press(const input::Sample& sample)
{
    const core::Page& page = m_context.document.page();
    m_pointer = sample.pointer;
    m_start = {sample.x, sample.y};
    m_pendingSelect = 0;
    m_dragSelection = false;
    m_additive = false;
    m_base.clear();
    m_grabbed.clear();
    m_grab = Grab::None;
    m_state = State::Pressed;

    const double zoom = m_context.view.zoom();
    const std::optional<core::ObjectId> hit = core::objectAt(page, m_start, core::kHitToleranceScreen / zoom);
    const std::vector<core::ObjectId>& selection = m_context.user.selection();
    const std::vector<core::PointF> points = shownShapePoints(selection);
    if (const std::optional<std::size_t> point = shapePointAt(points, m_start, zoom)) {
        m_grab = Grab::ShapePoint; // les carrés remplacent les poignées (mode points de forme)
        m_shapePoint = *point;
        return;
    }
    if (!selection.empty()) { // barre et poignées passent avant tout, même avec Maj ou Ctrl
        const SelectionView grabFrame =
            layoutSelection(page, selection, zoom, m_context.view.objectBarCommands(), m_context.lockIgnored);
        const SelectionHit grabbed = hitSelection(grabFrame, m_start, zoom);
        if (grabbed.kind == SelectionHitKind::BarButton) {
            m_command = grabbed.command;
            if (m_command == QStringLiteral("object.rotate")) {
                m_grab = Grab::Rotate;
            } else if (m_command == QStringLiteral("object.move-free")) {
                m_grab = Grab::FreeMove;
                m_grabbed = selection;
                m_dragSelection = true;
            } else {
                m_grab = Grab::Command;
            }
            return;
        }
        if (grabbed.kind == SelectionHitKind::Handle && points.empty()) {
            m_grab = Grab::Handle;
            m_handle = grabbed.handle;
            return;
        }
        if (grabbed.kind == SelectionHitKind::SmartPoint && points.empty()) {
            m_grab = Grab::SmartPoint;
            m_smartPoint = grabbed.smartPoint;
            return;
        }
    }
    if ((m_modifiers & kExtendModifiers) != 0) {
        if (hit) { // Maj ou Ctrl + clic : bascule (P26)
            std::vector<core::ObjectId> next = selection;
            const auto found = std::find(next.begin(), next.end(), *hit);
            if (found == next.end()) {
                next.push_back(*hit);
            } else {
                next.erase(found);
            }
            m_context.user.setSelection(std::move(next));
            showFrame();
        } else { // rectangle qui ajoute sans vider
            m_grab = Grab::Band;
            m_additive = true;
            m_base = selection;
        }
        return;
    }
    if (hit) {
        m_grab = Grab::Drag;
        if (contains(selection, *hit)) {
            m_grabbed = selection;
            m_dragSelection = true;
        } else {
            m_grabbed = {*hit};
            m_pendingSelect = *hit; // sélectionné seulement si ce n'est qu'un clic
        }
        return;
    }
    const SelectionView frame = layoutSelection(page, selection, zoom, {}, m_context.lockIgnored);
    if (hitSelection(frame, m_start, zoom).kind == SelectionHitKind::Inside) {
        m_grab = Grab::Drag; // dans le cadre, hors de tout objet : déplace la sélection
        m_grabbed = selection;
        m_dragSelection = true;
        return;
    }
    m_grab = Grab::Band;
}

void SelectTool::move(const input::Sample& sample)
{
    const core::PointF point{sample.x, sample.y};
    if (m_state == State::Pressed) {
        const double distance = std::hypot(point.x - m_start.x, point.y - m_start.y) * m_context.view.zoom();
        if (!(distance > kDragThresholdScreen)) {
            return;
        }
        switch (m_grab) {
        case Grab::Band:
            m_state = State::RubberBand;
            break;
        case Grab::Drag:
            beginDrag(false);
            break;
        case Grab::FreeMove:
            beginDrag(true);
            break;
        case Grab::Handle:
            beginTransform(State::Resizing);
            break;
        case Grab::Rotate:
            beginTransform(State::Rotating);
            break;
        case Grab::SmartPoint:
            beginSmartPoint();
            break;
        case Grab::ShapePoint:
            beginShapePoint();
            break;
        case Grab::None:
        case Grab::Command:
            return;
        }
        if (m_state == State::Pressed) {
            m_grab = Grab::None; // rien à faire bouger : le geste ne fait plus rien
            return;
        }
    }
    if (m_state == State::RubberBand) {
        showBand(point);
    } else if (m_state == State::SmartPoint) {
        updateSmartPoint(point);
    } else if (m_state == State::ShapePoint) {
        updateShapePoint(point);
    } else {
        updateDrag(point);
    }
}

void SelectTool::beginDrag(bool force)
{
    const core::Page& page = m_context.document.page();
    std::vector<core::ObjectId> ids;
    for (const core::ObjectId id : m_grabbed) {
        const std::size_t index = core::indexOf(page, id);
        if (index != page.objects.size() && (force || movable(page.objects[index], m_context.lockIgnored))) {
            ids.push_back(id);
        }
    }
    if (ids.empty()) {
        return;
    }
    m_copied.clear();
    m_copySources.clear();
    if (!force) { // D9 : la poignée « Déplacer » déplace ; un glissement d'objet à dragCopy le copie
        for (const core::ObjectId id : ids) {
            if (page.objects[core::indexOf(page, id)].common.dragCopy) {
                m_copied.push_back(id);
            }
        }
        if (!m_copied.empty()) {
            m_copySources = core::closureForCopy(page, m_copied); // soulevés, ils quittent la page
        }
    }
    m_frame =
        layoutSelection(page, m_context.user.selection(), m_context.view.zoom(), {}, m_context.lockIgnored);
    liftForGesture(std::move(ids), State::Dragging);
}

void SelectTool::beginTransform(State kind)
{
    const core::Page& page = m_context.document.page();
    std::vector<core::ObjectId> ids;
    for (const core::ObjectId id : m_context.user.selection()) {
        const std::size_t index = core::indexOf(page, id);
        if (index == page.objects.size()) {
            continue;
        }
        const core::PageObject& object = page.objects[index];
        const bool allowed = kind == State::Resizing ? resizable(object, m_context.lockIgnored)
                                                     : rotatable(object, m_context.lockIgnored); // P28
        if (allowed) {
            ids.push_back(id);
        }
    }
    m_frame =
        layoutSelection(page, m_context.user.selection(), m_context.view.zoom(), {}, m_context.lockIgnored);
    if (ids.empty() || m_frame.corners.size() != 4 ||
        (kind == State::Resizing && m_frame.handles.size() != 8)) {
        return;
    }
    liftForGesture(std::move(ids), kind);
}

// Une forme seule à préréglage, non verrouillée : soulevée, son aperçu est reconstruit à chaque mouvement
// (updateSmartPoint) et le dépôt est une seule SetShapeParametersCommand.
void SelectTool::beginSmartPoint()
{
    const core::Page& page = m_context.document.page();
    const std::vector<core::ObjectId>& selection = m_context.user.selection();
    if (selection.size() != 1) {
        return;
    }
    const std::size_t index = core::indexOf(page, selection.front());
    if (index == page.objects.size() || (page.objects[index].common.locked && !m_context.lockIgnored)) {
        return;
    }
    const auto* shape = std::get_if<core::Shape>(&core::body(page.objects[index]));
    if (shape == nullptr || m_smartPoint >= core::smartPoints(*shape).size()) {
        return;
    }
    m_frame = layoutSelection(page, selection, m_context.view.zoom(), {}, m_context.lockIgnored);
    m_smartParameters.clear();
    m_smartMoved = false;
    liftForGesture({selection.front()}, State::SmartPoint);
}

void SelectTool::updateSmartPoint(core::PointF point)
{
    if (!m_firstLifted) {
        return;
    }
    core::PageObject copy = *m_firstLifted;
    auto* shape = std::get_if<core::Shape>(&core::body(copy));
    if (shape == nullptr) {
        return;
    }
    m_smartParameters = core::dragSmartPoint(*shape, m_smartPoint, toObjectPoint(copy, point));
    shape->parameters = m_smartParameters;
    core::rebuildShape(*shape);
    m_smartMoved = true;
    SelectionView view = m_frame;
    view.handlesVisible = false; // F-528
    view.smartPoints = smartPointsOnPage(copy);
    std::vector<core::PageObject> preview;
    preview.push_back(std::move(copy));
    m_context.lift.show(std::move(preview));
    publish(view);
}

void SelectTool::endSmartPoint(bool cancelled)
{
    if (cancelled || !m_smartMoved || m_smartParameters.empty()) {
        m_context.document.drop(nullptr);
    } else {
        m_context.document.drop(std::make_unique<core::SetShapeParametersCommand>(
            m_context.document.pageIndex(), m_lifted.front(), m_smartParameters));
    }
    m_context.lift.clear();
    reset();
    showFrame();
}

// Un point de forme d'une forme seule, non verrouillée : soulevée, son aperçu est le résultat de
// MoveShapePointCommand sur une copie (L5-T9, aucune règle refaite ici) ; le dépôt est une seule commande.
void SelectTool::beginShapePoint()
{
    const core::Page& page = m_context.document.page();
    const std::vector<core::ObjectId>& selection = m_context.user.selection();
    if (selection.size() != 1) {
        return;
    }
    const std::size_t index = core::indexOf(page, selection.front());
    if (index == page.objects.size() || (page.objects[index].common.locked && !m_context.lockIgnored) ||
        m_shapePoint >= core::shapePoints(page.objects[index]).size()) {
        return; // D9 : un verrouillé ne bouge pas par ses points
    }
    m_frame = layoutSelection(page, selection, m_context.view.zoom(), {}, m_context.lockIgnored);
    m_shapePointTo.reset();
    liftForGesture({selection.front()}, State::ShapePoint);
}

void SelectTool::updateShapePoint(core::PointF point)
{
    if (!m_firstLifted) {
        return;
    }
    core::Document probe; // hors du document : aucun redessin du cache pendant le geste
    probe.page(0).objects.push_back(*m_firstLifted);
    core::Changes ignored;
    core::MoveShapePointCommand(0, m_firstLifted->common.id, m_shapePoint, point).redo(probe, ignored);
    core::PageObject preview = probe.page(0).objects.front();
    m_shapePointTo = point;
    SelectionView view = m_frame;
    view.handlesVisible = false;
    view.smartPoints.clear();
    view.shapePoints = core::shapePoints(preview);
    std::vector<core::PageObject> shown;
    shown.push_back(std::move(preview));
    m_context.lift.show(std::move(shown));
    publish(view);
}

void SelectTool::endShapePoint(bool cancelled)
{
    if (cancelled || !m_shapePointTo) {
        m_context.document.drop(nullptr);
    } else {
        // L5-T10 : le connecteur lié suit ; enveloppée, la commande ne fusionne plus, le geste reste une
        // étape parce qu'il n'en dépose qu'une, au relâché.
        const std::size_t page = m_context.document.pageIndex();
        m_context.document.drop(
            core::withFollowingConnectors(m_context.document.page(), page,
                                          std::make_unique<core::MoveShapePointCommand>(
                                              page, m_lifted.front(), m_shapePoint, *m_shapePointTo)));
    }
    m_context.lift.clear();
    reset();
    showFrame();
}

// m_lifted est posé avant le soulèvement.
void SelectTool::liftForGesture(std::vector<core::ObjectId> ids, State state)
{
    m_lifted = std::move(ids);
    std::vector<core::PageObject> copies = m_context.document.lift(m_lifted);
    m_firstLifted.reset();
    if (!copies.empty()) {
        m_firstLifted = copies.front();
    }
    m_context.lift.show(std::move(copies));
    m_state = state;
}

// Transformation du geste en cours, en coordonnées de page, appliquée après celle des objets.
core::Transform SelectTool::gestureTransform(core::PointF point) const
{
    if (m_state == State::Rotating) {
        const core::PointF center{(m_frame.corners[0].x + m_frame.corners[2].x) / 2,
                                  (m_frame.corners[0].y + m_frame.corners[2].y) / 2};
        double delta = toDegrees(std::atan2(point.y - center.y, point.x - center.x)) -
                       toDegrees(std::atan2(m_start.y - center.y, m_start.x - center.x));
        if ((m_modifiers & static_cast<int>(Qt::ShiftModifier)) != 0) {
            delta = std::round(delta / kSnapStepDegrees) * kSnapStepDegrees;
        }
        return core::rotationAbout(center, delta);
    }
    if (m_state == State::Resizing) {
        const std::vector<core::PointF>& c = m_frame.corners;
        const double width = std::hypot(c[1].x - c[0].x, c[1].y - c[0].y);
        const double height = std::hypot(c[3].x - c[0].x, c[3].y - c[0].y);
        if (!(width > 0) || !(height > 0)) {
            return core::Transform{};
        }
        // Axes du cadre (ceux de la page tant que l'objet n'a pas pivoté).
        const core::PointF u{(c[1].x - c[0].x) / width, (c[1].y - c[0].y) / width};
        const core::PointF v{(c[3].x - c[0].x) / height, (c[3].y - c[0].y) / height};
        const std::size_t grabbedIndex = static_cast<std::size_t>(m_handle);
        const core::PointF grabbed = m_frame.handles[grabbedIndex];
        const core::PointF anchor = m_frame.handles[(grabbedIndex + 4) % 8]; // le côté opposé reste fixe
        const core::PointF rel{grabbed.x + point.x - m_start.x - anchor.x,
                               grabbed.y + point.y - m_start.y - anchor.y};
        const int sh = horizontalSign(m_handle);
        const int sv = verticalSign(m_handle);
        double sx = sh == 0 ? 1.0 : std::max(sh * (rel.x * u.x + rel.y * u.y), kMinExtent) / width;
        double sy = sv == 0 ? 1.0 : std::max(sv * (rel.x * v.x + rel.y * v.y), kMinExtent) / height;
        if (m_handle == HandleKind::BottomRight) { // seule poignée qui garde les proportions
            sx = sy = std::max(sx, sy);
        }
        const core::Transform scale = core::scalingAbout(anchor, sx, sy);
        const double angle = toDegrees(std::atan2(u.y, u.x));
        if (std::abs(angle) < 1e-9) {
            return scale;
        }
        return core::compose(core::compose(core::rotationAbout(anchor, -angle), scale),
                             core::rotationAbout(anchor, angle));
    }
    return core::translation(point.x - m_start.x, point.y - m_start.y);
}

void SelectTool::updateDrag(core::PointF point)
{
    const core::Transform delta = gestureTransform(point);
    m_context.lift.setTransform(delta);
    if (m_state == State::Dragging) {
        const bool over = m_context.view.overTrash(point);
        if (over != m_trashHover) {
            m_trashHover = over;
            m_context.view.setTrashHover(over);
        }
    }
    SelectionView view = m_frame;
    view.handlesVisible = false; // F-528
    view.smartPoints.clear();
    if (m_dragSelection || m_state != State::Dragging) {
        for (core::PointF& corner : view.corners) {
            corner = delta.map(corner);
        }
    }
    publish(view);
}

void SelectTool::endDrag(core::PointF point, bool cancelled)
{
    core::Transform delta = gestureTransform(point);
    const std::size_t page = m_context.document.pageIndex();
    if (m_state == State::Dragging && !cancelled && m_firstLifted && !m_context.snapDisabled &&
        !m_context.view.overTrash(point)) {
        // Accrochage au dépôt seulement (F-622) : les objets sont soulevés, donc hors de la page ; on
        // accroche d'après la copie du premier, sur la grille de la page.
        core::Page probe;
        probe.grid = m_context.document.page().grid;
        probe.width = m_context.document.page().width;
        probe.height = m_context.document.page().height;
        probe.objects.push_back(*m_firstLifted);
        const core::PointF snapped = core::snapDelta(probe, {m_firstLifted->common.id}, {delta.dx, delta.dy});
        delta = core::translation(snapped.x, snapped.y);
    }
    const bool trash = m_state == State::Dragging && !cancelled && m_context.view.overTrash(point);
    if (trash) {
        std::vector<core::ObjectId> removed; // D9 : un objet à dragCopy reste, même sur la corbeille
        for (const core::ObjectId id : m_lifted) {
            if (!contains(m_copied, id)) {
                removed.push_back(id);
            }
        }
        std::vector<core::ObjectId> selection = m_context.user.selection();
        std::erase_if(selection, [&removed](core::ObjectId id) { return contains(removed, id); });
        m_context.document.drop(
            removed.empty() ? nullptr : std::make_unique<core::RemoveObjectsCommand>(page, removed));
        m_context.user.setSelection(std::move(selection));
    } else if (cancelled || delta.isIdentity()) {
        m_context.document.drop(nullptr);
    } else {
        // Même chemin qu'au lot 3 (drop : objets remis, une étape) ; D9 : un objet à dragCopy est remis sans
        // étape et sa copie ajoutée (PasteObjectsCommand), jamais transformé.
        m_context.document.drop(dragDropCommand(page, m_lifted, m_copied, std::move(m_copySources), delta));
    }
    m_context.lift.clear();
    if (m_trashHover) {
        m_trashHover = false;
        m_context.view.setTrashHover(false);
    }
    reset();
    showFrame();
}

void SelectTool::showBand(core::PointF point)
{
    SelectionView view = layoutSelection(m_context.document.page(), m_context.user.selection(),
                                         m_context.view.zoom(), {}, m_context.lockIgnored);
    view.rubberBand = core::normalizedRect(m_start, point);
    publish(view);
}

void SelectTool::release(const input::Sample& sample)
{
    const core::PointF point{sample.x, sample.y};
    switch (m_state) {
    case State::Idle:
        return;
    case State::Pressed:
        if (m_grab == Grab::Command) {
            const QString command = m_command;
            reset();
            m_context.commands.trigger(command, {});
            showFrame();
            return;
        }
        if (m_grab == Grab::Drag && m_pendingSelect != 0) {
            m_context.user.setSelection({m_pendingSelect});
        } else if (m_grab == Grab::Band && !m_additive) {
            m_context.user.setSelection({}); // clic dans le vide : désélection
        }
        break;
    case State::RubberBand: {
        std::vector<core::ObjectId> ids =
            core::objectsTouching(m_context.document.page(), core::normalizedRect(m_start, point));
        if (m_additive) {
            std::vector<core::ObjectId> merged = m_base;
            for (const core::ObjectId id : ids) {
                if (!contains(merged, id)) {
                    merged.push_back(id);
                }
            }
            ids = std::move(merged);
        }
        m_context.user.setSelection(std::move(ids));
        break;
    }
    case State::Dragging:
    case State::Resizing:
    case State::Rotating:
        endDrag(point, false);
        return;
    case State::SmartPoint:
        endSmartPoint(false);
        return;
    case State::ShapePoint:
        endShapePoint(false);
        return;
    }
    reset();
    showFrame();
}

void SelectTool::showFrame()
{
    SelectionView view =
        layoutSelection(m_context.document.page(), m_context.user.selection(), m_context.view.zoom(),
                        m_context.view.objectBarCommands(), m_context.lockIgnored);
    view.shapePoints = shownShapePoints(m_context.user.selection());
    if (!view.shapePoints.empty()) { // les carrés remplacent poignées et points intelligents (F-527)
        view.handlesVisible = false;
        view.smartPoints.clear();
    }
    publish(view);
}

void SelectTool::publish(const SelectionView& view)
{
    m_context.selection.show(view);
}

void SelectTool::reset() noexcept
{
    m_state = State::Idle;
    m_grab = Grab::None;
    m_pendingSelect = 0;
    m_dragSelection = false;
    m_additive = false;
    m_command.clear();
    m_smartParameters.clear();
    m_smartMoved = false;
    m_shapePoint = 0;
    m_shapePointTo.reset();
    m_copied.clear();
    m_copySources.clear();
    m_base.clear();
    m_grabbed.clear();
    m_lifted.clear();
    m_firstLifted.reset();
}

} // namespace actcraft::tools
