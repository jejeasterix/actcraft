#include <actcraft/core/edits.h>
#include <actcraft/core/erasegeometry.h>
#include <actcraft/core/hittest.h>
#include <actcraft/core/replace.h>
#include <actcraft/tools/erasertool.h>
#include <actcraft/tools/userstate.h>

#include <algorithm>
#include <cmath>
#include <memory>
#include <utility>

namespace actcraft::tools {

EraserTool::EraserTool(ToolContext& context) : m_context(context) {}

QString EraserTool::id() const
{
    return QStringLiteral("eraser");
}

double EraserTool::radius() const
{
    return m_context.user.settings(QStringLiteral("eraser")).width / 2;
}

bool EraserTool::isBusy() const noexcept
{
    return m_active;
}

void EraserTool::onSample(const input::Sample& sample)
{
    const core::PointF point{sample.x, sample.y};
    switch (sample.phase) {
    case input::Phase::Press:
        if (m_active)
            cancel();
        begin(sample);
        break;
    case input::Phase::Move:
        if (!m_active)
            return;
        sweep(m_last, point);
        m_last = point;
        break;
    case input::Phase::Release:
        if (!m_active)
            return;
        sweep(m_last, point);
        m_last = point;
        finish();
        break;
    }
}

void EraserTool::cancel()
{
    if (!m_active)
        return;
    m_context.document.drop(nullptr);
    m_context.lift.clear();
    reset();
}

void EraserTool::begin(const input::Sample& sample)
{
    m_active = true;
    m_lifted.clear();
    m_last = {sample.x, sample.y};
    sweep(m_last, m_last, true); // une touche sans mouvement coupe sous le disque
}

void EraserTool::reset()
{
    m_active = false;
    m_lifted.clear();
}

// Soulève chaque annotation touchée par le balayage (une seule fois par geste), puis recoupe les morceaux
// courants de tous les traits soulevés.
void EraserTool::sweep(core::PointF from, core::PointF to, bool first)
{
    const double r = radius();
    const core::RectF box = core::normalizedRect(from, to);
    const core::RectF reach{box.x - r, box.y - r, box.width + 2 * r, box.height + 2 * r};

    std::vector<core::ObjectId> toLift;
    for (const core::PageObject& object : m_context.document.page().objects) {
        // Ni forme, ni image, ni texte (isAnnotation) ; un objet masqué ou verrouillé est sauté (D4).
        if (!core::isAnnotation(object) || !core::isSelectable(object) || object.common.locked)
            continue;
        if (core::rectsTouch(core::boundsOnPage(object), reach))
            toLift.push_back(object.common.id);
    }
    if (!toLift.empty()) {
        for (core::PageObject& copy : m_context.document.lift(toLift)) {
            Lifted lifted;
            lifted.id = copy.common.id;
            lifted.pieces.push_back(core::strokePointsOnPage(copy));
            lifted.original = std::move(copy);
            m_lifted.push_back(std::move(lifted));
        }
    }
    if (m_lifted.empty())
        return;

    // Pas d'au plus r / 2 (comme core::cutBySweep). Le centre `from` est celui du segment précédent (ou du
    // geste, pour le premier) : il n'est recoupé que pour le premier segment. core::cutByDisk compte un
    // point sur le cercle comme intérieur : recouper un morceau avec le même disque effacerait le bout qu'il
    // vient de laisser sur ce cercle (constaté, à signaler à L3-T3).
    const double distance = std::hypot(to.x - from.x, to.y - from.y);
    const int steps = r > 0 ? std::max(1, static_cast<int>(std::ceil(distance / (r / 2)))) : 1;
    std::vector<core::PointF> centers;
    if (first)
        centers.push_back(from);
    if (distance > 0) {
        for (int step = 1; step <= steps; ++step) {
            const double t = static_cast<double>(step) / steps;
            centers.push_back({from.x + t * (to.x - from.x), from.y + t * (to.y - from.y)});
        }
    }
    for (Lifted& lifted : m_lifted) {
        for (const core::PointF& center : centers) {
            std::vector<std::vector<core::StrokePoint>> next;
            for (auto& piece : lifted.pieces) {
                core::StrokeCut cut = core::cutByDisk(piece, center, r);
                if (!cut.touched) {
                    next.push_back(std::move(piece));
                    continue;
                }
                lifted.touched = true;
                for (auto& part : cut.pieces)
                    next.push_back(std::move(part));
            }
            lifted.pieces = std::move(next);
        }
    }
    showLifted();
}

// Morceaux d'un trait : objets traits de la couche du dessus, commun de l'original, transformation identité.
std::vector<core::PageObject> EraserTool::pieceObjects(const Lifted& lifted) const
{
    std::vector<core::PageObject> out;
    out.reserve(lifted.pieces.size());
    for (const auto& piece : lifted.pieces) {
        core::PageObject object = lifted.original;
        std::get<core::Stroke>(core::body(object)).points = piece;
        object.common.transform = core::Transform{};
        object.common.id = 0; // tiré par la commande au dépôt
        out.push_back(std::move(object));
    }
    return out;
}

void EraserTool::showLifted()
{
    std::vector<core::PageObject> shown;
    for (const Lifted& lifted : m_lifted) {
        for (core::PageObject& object : pieceObjects(lifted))
            shown.push_back(std::move(object));
    }
    m_context.lift.show(std::move(shown));
}

// Relâché : une seule commande, un pas d'annulation. Un trait soulevé mais non coupé n'y figure pas : le
// dépôt le remet tel quel.
void EraserTool::finish()
{
    if (m_lifted.empty()) {
        reset();
        return;
    }
    std::vector<core::ObjectReplacement> groups;
    for (const Lifted& lifted : m_lifted) {
        if (!lifted.touched)
            continue;
        groups.push_back(core::ObjectReplacement{{lifted.id}, pieceObjects(lifted)});
    }
    std::unique_ptr<core::UndoCommand> command;
    if (!groups.empty()) {
        command =
            std::make_unique<core::ReplaceObjectsCommand>(m_context.document.pageIndex(), std::move(groups));
    }
    m_context.document.drop(std::move(command));
    m_context.lift.clear();
    reset();
}

} // namespace actcraft::tools
