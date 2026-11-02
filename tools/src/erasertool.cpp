#include <actcraft/core/edits.h>
#include <actcraft/core/erasegeometry.h>
#include <actcraft/core/hittest.h>
#include <actcraft/core/objectedits.h>
#include <actcraft/core/replace.h>
#include <actcraft/tools/erasertool.h>
#include <actcraft/tools/userstate.h>

#include <algorithm>
#include <cmath>
#include <memory>
#include <utility>

namespace actcraft::tools {

namespace {

// Emprise de la ligne centrale (sans l'épaisseur). core::cutByDisk ne retire que ce qui est à moins de r du
// centre : un morceau dont cette emprise ne touche pas le carré [c − r, c + r] ne peut pas être coupé.
core::RectF centerlineBounds(const std::vector<core::StrokePoint>& points) noexcept
{
    if (points.empty())
        return {};
    double x0 = points.front().x;
    double x1 = x0;
    double y0 = points.front().y;
    double y1 = y0;
    for (const core::StrokePoint& p : points) {
        x0 = std::min(x0, p.x);
        x1 = std::max(x1, p.x);
        y0 = std::min(y0, p.y);
        y1 = std::max(y1, p.y);
    }
    return {x0, y0, x1 - x0, y1 - y0};
}

} // namespace

EraserTool::EraserTool(ToolContext& context) : m_context(context) {}

QString EraserTool::id() const
{
    return QStringLiteral("eraser");
}

double EraserTool::radius() const
{
    return m_context.user.settings(QStringLiteral("eraser")).width / 2;
}

std::size_t EraserTool::cutCalls() const noexcept
{
    return m_cuts;
}

bool EraserTool::isBusy() const noexcept
{
    return m_active;
}

// Seul le pointeur de l'appui mène le geste : un autre contact (appui, mouvement, relâché) est ignoré.
void EraserTool::onSample(const input::Sample& sample)
{
    const core::PointF point{sample.x, sample.y};
    switch (sample.phase) {
    case input::Phase::Press:
        if (m_active) {
            if (sample.pointer != m_pointer)
                return;
            cancel();
        }
        begin(sample);
        break;
    case input::Phase::Move:
    case input::Phase::Release:
        if (!m_active || sample.pointer != m_pointer)
            return;
        sweep(m_last, point);
        m_last = point;
        if (sample.phase == input::Phase::Release)
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
    m_pointer = sample.pointer;
    m_last = {sample.x, sample.y};
    sweep(m_last, m_last, true); // une touche sans mouvement coupe sous le disque
}

void EraserTool::reset()
{
    m_active = false;
    m_lifted.clear();
}

// Soulève chaque annotation touchée par le balayage (une seule fois par geste), puis recoupe les morceaux
// proches du balayage. La couche soulevée n'est remontrée que si un trait vient d'être soulevé ou qu'un
// morceau a changé : un mouvement loin de tout morceau ne coupe ni ne montre rien.
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
    bool changed = false;
    if (!toLift.empty()) {
        for (core::PageObject& copy : m_context.document.lift(toLift)) {
            auto* stroke = std::get_if<core::Stroke>(&core::body(copy));
            if (!stroke)
                continue;
            Lifted lifted;
            lifted.id = copy.common.id;
            std::vector<core::StrokePoint> points = core::strokePointsOnPage(copy);
            const core::RectF bounds = centerlineBounds(points);
            lifted.pieces.push_back({std::move(points), bounds});
            // Modèle des morceaux : leurs points sont en coordonnées de page, donc transformation identité et
            // épaisseur à l'échelle visible de l'original ; ses points ne sont plus recopiés par morceau.
            stroke->baseWidth *= core::lengthScale(copy.common.transform);
            std::vector<core::StrokePoint>().swap(stroke->points);
            copy.common.transform = core::Transform{};
            copy.common.id = 0; // tiré par la commande au dépôt
            lifted.original = std::move(copy);
            m_lifted.push_back(std::move(lifted));
            changed = true; // il a quitté la page : la couche doit le montrer
        }
    }
    if (m_lifted.empty())
        return;

    // Pas d'au plus r / 2 (comme core::cutBySweep). Le centre `from` est celui du segment précédent (ou du
    // geste, pour le premier) : il n'est recoupé que pour le premier segment. core::cutByDisk compte le
    // cercle dehors (L3-C6) : recouper un morceau avec le même disque ne raccourcirait pas le bout qu'il
    // vient de laisser sur ce cercle ; ce serait une coupe redondante, à l'arrondi près.
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

    // Chaque morceau proche reçoit les centres dans l'ordre ; ses restes prennent sa place dans le trait. Un
    // morceau loin du balayage n'est ni relu ni coupé.
    const auto nearSweep = [&](const Piece& piece) { return core::rectsTouch(piece.bounds, reach); };
    if (!centers.empty())
        for (Lifted& lifted : m_lifted) {
            if (std::none_of(lifted.pieces.begin(), lifted.pieces.end(), nearSweep))
                continue;
            std::vector<Piece> next;
            for (Piece& piece : lifted.pieces) {
                if (!nearSweep(piece)) {
                    next.push_back(std::move(piece));
                    continue;
                }
                std::vector<Piece> parts;
                parts.push_back(std::move(piece));
                for (const core::PointF& center : centers) {
                    const core::RectF square{center.x - r, center.y - r, 2 * r, 2 * r};
                    std::vector<Piece> after;
                    for (Piece& part : parts) {
                        if (!core::rectsTouch(part.bounds, square)) {
                            after.push_back(std::move(part));
                            continue;
                        }
                        ++m_cuts;
                        core::StrokeCut cut = core::cutByDisk(part.points, center, r);
                        if (!cut.touched) {
                            after.push_back(std::move(part));
                            continue;
                        }
                        lifted.touched = true;
                        changed = true;
                        for (auto& rest : cut.pieces) {
                            const core::RectF restBounds = centerlineBounds(rest);
                            after.push_back({std::move(rest), restBounds});
                        }
                    }
                    parts = std::move(after);
                }
                for (Piece& part : parts)
                    next.push_back(std::move(part));
            }
            lifted.pieces = std::move(next);
        }
    if (changed)
        showLifted();
}

// Morceaux d'un trait : copies du modèle (sans les points d'origine), chacune avec les points de son morceau.
std::vector<core::PageObject> EraserTool::pieceObjects(const Lifted& lifted) const
{
    std::vector<core::PageObject> out;
    out.reserve(lifted.pieces.size());
    for (const Piece& piece : lifted.pieces) {
        core::PageObject object = lifted.original;
        std::get<core::Stroke>(core::body(object)).points = piece.points;
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
