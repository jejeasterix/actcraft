#include <actcraft/core/edits.h>
#include <actcraft/core/hittest.h>
#include <actcraft/core/objectedits.h>
#include <actcraft/tools/selectiongeometry.h>
#include <actcraft/tools/selecttool.h>
#include <actcraft/tools/userstate.h>

#include <QtCore/qnamespace.h>

#include <algorithm>
#include <cmath>
#include <memory>
#include <optional>
#include <utility>

namespace actcraft::tools {

namespace {

// Glissement : au-delà de 2 px d'écran (D6, P27) ; un clic ne soulève rien.
constexpr double kDragThresholdScreen = 2.0;
constexpr int kExtendModifiers = Qt::ShiftModifier | Qt::ControlModifier;

bool contains(const std::vector<core::ObjectId>& ids, core::ObjectId id)
{
    return std::find(ids.begin(), ids.end(), id) != ids.end();
}

// Verrouillé ou non déplaçable : sélectionné, jamais déplacé (F-210, P28).
bool movable(const core::PageObject& object)
{
    return !object.common.locked && object.common.restrictions.movable;
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
    const core::Page& page = m_context.document.page();
    const std::vector<core::ObjectId> ids = present(page, m_context.user.selection());
    if (ids.empty()) {
        return;
    }
    const core::PointF center = core::selectionCenter(page, ids);
    m_context.document.execute(std::make_unique<core::TransformObjectsCommand>(
        m_context.document.pageIndex(), ids, core::scalingAbout(center, kEnlargeStep, kEnlargeStep)));
    showFrame();
}

void SelectTool::reduce()
{
    const core::Page& page = m_context.document.page();
    const std::vector<core::ObjectId> ids = present(page, m_context.user.selection());
    if (ids.empty()) {
        return;
    }
    const core::PointF center = core::selectionCenter(page, ids);
    const double factor = 1.0 / kEnlargeStep;
    m_context.document.execute(std::make_unique<core::TransformObjectsCommand>(
        m_context.document.pageIndex(), ids, core::scalingAbout(center, factor, factor)));
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
    const std::vector<core::ObjectId> selection = m_context.user.selection();
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
        if (m_grab == Grab::Band) {
            m_state = State::RubberBand;
        } else if (m_grab == Grab::Drag) {
            beginDrag();
            if (m_state != State::Dragging) {
                m_grab = Grab::None; // rien de déplaçable : le geste ne fait plus rien
                return;
            }
        } else {
            return;
        }
    }
    if (m_state == State::Dragging) {
        updateDrag(point);
    } else if (m_state == State::RubberBand) {
        showBand(point);
    }
}

void SelectTool::beginDrag()
{
    const core::Page& page = m_context.document.page();
    std::vector<core::ObjectId> ids;
    for (const core::ObjectId id : m_grabbed) {
        const std::size_t index = core::indexOf(page, id);
        if (index != page.objects.size() && movable(page.objects[index])) {
            ids.push_back(id);
        }
    }
    if (ids.empty()) {
        return;
    }
    m_frame = layoutSelection(page, m_context.user.selection(), m_context.view.zoom(), {});
    m_lifted = ids;
    std::vector<core::PageObject> copies = m_context.document.lift(ids);
    m_context.lift.show(std::move(copies));
    m_state = State::Dragging;
}

void SelectTool::updateDrag(core::PointF point)
{
    const double dx = point.x - m_start.x;
    const double dy = point.y - m_start.y;
    m_context.lift.setTransform(core::translation(dx, dy));
    const bool over = m_context.view.overTrash(point);
    if (over != m_trashHover) {
        m_trashHover = over;
        m_context.view.setTrashHover(over);
    }
    SelectionView view = m_frame;
    view.handlesVisible = false; // F-528
    if (m_dragSelection) {
        for (core::PointF& corner : view.corners) {
            corner.x += dx;
            corner.y += dy;
        }
    }
    publish(view);
}

void SelectTool::endDrag(core::PointF point, bool cancelled)
{
    const double dx = point.x - m_start.x;
    const double dy = point.y - m_start.y;
    const std::size_t page = m_context.document.pageIndex();
    const bool trash = !cancelled && m_context.view.overTrash(point);
    if (trash) {
        std::vector<core::ObjectId> selection = m_context.user.selection();
        selection.erase(std::remove_if(selection.begin(), selection.end(),
                                       [this](core::ObjectId id) { return contains(m_lifted, id); }),
                        selection.end());
        m_context.document.drop(std::make_unique<core::RemoveObjectsCommand>(page, m_lifted));
        m_context.user.setSelection(std::move(selection));
    } else if (cancelled || (dx == 0 && dy == 0)) {
        m_context.document.drop(nullptr);
    } else {
        m_context.document.drop(
            std::make_unique<core::TransformObjectsCommand>(page, m_lifted, core::translation(dx, dy)));
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
    m_base.clear();
    m_grabbed.clear();
    m_lifted.clear();
}

} // namespace actcraft::tools
