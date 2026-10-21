#include <actcraft/core/edits.h>
#include <actcraft/core/gridsnap.h>
#include <actcraft/core/hittest.h>
#include <actcraft/core/objectedits.h>
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
bool movable(const core::PageObject& object)
{
    return !object.common.locked && object.common.restrictions.movable;
}

// Dimensionnable : la règle des poignées (layoutSelection, beginTransform ; P28, P30), pas celle du glisser.
bool resizable(const core::PageObject& object)
{
    return !object.common.locked && object.common.restrictions.resizable;
}

// Pivotable : la même règle pour le bouton de rotation (beginTransform ; P28).
bool rotatable(const core::PageObject& object)
{
    return !object.common.locked && object.common.restrictions.rotatable;
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
        if (index != page.objects.size() && core::isSelectable(page.objects[index])) {
            out.push_back(id);
        }
    }
    return out;
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
    }
}

bool SelectTool::onKey(int key, int modifiers)
{
    m_modifiers = modifiers;
    if (key == Qt::Key_Escape) {
        if (m_state == State::Idle) {
            return false;
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
    if (!selection.empty()) { // barre et poignées passent avant tout, même avec Maj ou Ctrl
        const SelectionView grabFrame =
            layoutSelection(page, selection, zoom, m_context.view.objectBarCommands());
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
        if (grabbed.kind == SelectionHitKind::Handle) {
            m_grab = Grab::Handle;
            m_handle = grabbed.handle;
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
    const SelectionView frame = layoutSelection(page, selection, zoom, {});
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
        if (index != page.objects.size() && (force || movable(page.objects[index]))) {
            ids.push_back(id);
        }
    }
    if (ids.empty()) {
        return;
    }
    m_frame = layoutSelection(page, m_context.user.selection(), m_context.view.zoom(), {});
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
        const bool allowed = kind == State::Resizing ? resizable(object) : rotatable(object); // P28
        if (allowed) {
            ids.push_back(id);
        }
    }
    m_frame = layoutSelection(page, m_context.user.selection(), m_context.view.zoom(), {});
    if (ids.empty() || m_frame.corners.size() != 4 ||
        (kind == State::Resizing && m_frame.handles.size() != 8)) {
        return;
    }
    liftForGesture(std::move(ids), kind);
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
    if (m_state == State::Dragging && !cancelled && m_firstLifted && !m_context.view.overTrash(point)) {
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
        std::vector<core::ObjectId> selection = m_context.user.selection();
        std::erase_if(selection, [this](core::ObjectId id) { return contains(m_lifted, id); });
        m_context.document.drop(std::make_unique<core::RemoveObjectsCommand>(page, m_lifted));
        m_context.user.setSelection(std::move(selection));
    } else if (cancelled || delta.isIdentity()) {
        m_context.document.drop(nullptr);
    } else {
        m_context.document.drop(std::make_unique<core::TransformObjectsCommand>(page, m_lifted, delta));
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
    SelectionView view =
        layoutSelection(m_context.document.page(), m_context.user.selection(), m_context.view.zoom(), {});
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
    }
    reset();
    showFrame();
}

void SelectTool::showFrame()
{
    publish(layoutSelection(m_context.document.page(), m_context.user.selection(), m_context.view.zoom(),
                            m_context.view.objectBarCommands()));
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
    m_base.clear();
    m_grabbed.clear();
    m_lifted.clear();
    m_firstLifted.reset();
}

} // namespace actcraft::tools
