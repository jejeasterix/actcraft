#include <actcraft/core/alignment.h>
#include <actcraft/core/arrange.h>
#include <actcraft/core/edits.h>
#include <actcraft/core/objectactions.h>
#include <actcraft/core/objectedits.h>
#include <actcraft/core/propertyedits.h>

#include <QByteArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QString>

#include <algorithm>
#include <cmath>
#include <functional>
#include <optional>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>

// Actions d'objet de géométrie (L8-T7). Aucune commande neuve : angle, placer, dimensionner, étirer, refléter
// et retourner rendent un TransformObjectsCommand (delta en coordonnées de page, comme l'outil Sélection) ;
// ajuster et taille d'origine reprennent fitTransforms des commandes de menu `transform.*` (alignment.h,
// ui/src/objectediting.cpp) et TransformEachCommand.
namespace actcraft::core {
namespace {

constexpr double kEps = 1e-9;

struct Target
{
    const Page& page;
    const PageObject& object;
    RectF box; // emprise sur la page ; un groupe : celle de ses membres
    const ActionContext& context;
    QJsonObject parameters;
};

using Builder = std::function<std::unique_ptr<UndoCommand>(const Target&)>;

// Point de la boîte, en fractions de sa largeur et de sa hauteur (0 : gauche ou haut ; 1 : droite ou bas).
struct Anchor
{
    double x = 0;
    double y = 0;
};

const std::pair<const char*, Anchor> kAnchors[] = {
    {"top-left", {0, 0}},    {"top", {0.5, 0}},      {"top-right", {1, 0}},
    {"left", {0, 0.5}},      {"center", {0.5, 0.5}}, {"right", {1, 0.5}},
    {"bottom-left", {0, 1}}, {"bottom", {0.5, 1}},   {"bottom-right", {1, 1}},
};

PointF at(const RectF& r, Anchor a) noexcept
{
    return {r.x + a.x * r.width, r.y + a.y * r.height};
}

std::optional<double> number(const QJsonObject& parameters, const char* name)
{
    const QJsonValue v = parameters.value(QString::fromLatin1(name));
    if (!v.isDouble() || !std::isfinite(v.toDouble())) {
        return std::nullopt;
    }
    return v.toDouble();
}

// Objets dont la transformation change : la cible ; pour un groupe, ses membres, seuls porteurs d'une
// transformation appliquée (edits.h).
std::vector<ObjectId> transformedIds(const Page& page, const PageObject& object)
{
    if (!std::holds_alternative<Group>(body(object))) {
        return {object.common.id};
    }
    std::vector<ObjectId> ids;
    for (const ObjectId id : expandGroups(page, {object.common.id})) {
        const std::size_t i = indexOf(page, id);
        if (i < page.objects.size() && !std::holds_alternative<Group>(body(page.objects[i]))) {
            ids.push_back(id);
        }
    }
    return ids;
}

std::unique_ptr<UndoCommand> transformBy(const Target& t, const Transform& delta)
{
    std::vector<ObjectId> ids = transformedIds(t.page, t.object);
    if (ids.empty()) {
        return nullptr;
    }
    return std::make_unique<TransformObjectsCommand>(t.context.page, std::move(ids), delta);
}

std::unique_ptr<UndoCommand> transformEach(const Target& t, TransformList list)
{
    if (list.empty()) {
        return nullptr; // le chemin du menu n'a rien à faire (objet déjà à la taille, plus petit que la
                        // page…)
    }
    return std::make_unique<TransformEachCommand>(t.context.page, std::move(list));
}

// Pivot de rotation de la cible (D8-7), en coordonnées de page.
PointF pivotOf(const Target& t)
{
    const Restrictions& r = t.object.common.restrictions;
    switch (r.pivot) {
    case Mark::TopLeft:
        return at(t.box, {0, 0});
    case Mark::Top:
        return at(t.box, {0.5, 0});
    case Mark::TopRight:
        return at(t.box, {1, 0});
    case Mark::Right:
        return at(t.box, {1, 0.5});
    case Mark::BottomRight:
        return at(t.box, {1, 1});
    case Mark::Bottom:
        return at(t.box, {0.5, 1});
    case Mark::BottomLeft:
        return at(t.box, {0, 1});
    case Mark::Left:
        return at(t.box, {0, 0.5});
    case Mark::Point:
        return t.object.common.transform.map(r.pivotPoint);
    case Mark::OtherObject: {
        const std::size_t i = indexOf(t.page, r.pivotObject);
        if (r.pivotObject != 0 && i < t.page.objects.size() && hasExtent(t.page, t.page.objects[i])) {
            return boundsOnPage(t.page, t.page.objects[i]).center();
        }
        return t.box.center();
    }
    case Mark::Center:
    case Mark::FirstTextLine:
        break;
    }
    return t.box.center();
}

bool rotatable(const Target& t) noexcept
{
    return t.object.common.restrictions.rotatable;
}
bool resizable(const Target& t) noexcept
{
    return t.object.common.restrictions.resizable;
}
bool movable(const Target& t) noexcept
{
    return t.object.common.restrictions.movable;
}

std::unique_ptr<UndoCommand> turn(const Target& t, bool absolute)
{
    const std::optional<double> angle = number(t.parameters, "angle");
    if (!rotatable(t) || !angle) {
        return nullptr;
    }
    if (absolute && std::holds_alternative<Group>(body(t.object))) {
        return nullptr; // l'angle d'un groupe n'est pas défini (alignment.cpp)
    }
    // Angle de la propriété « Rotation » (propertyedits.h), sans le demi-tour d'un objet reflété.
    const double degrees = absolute ? *angle - objectAngle(t.object.common.transform) : *angle;
    return transformBy(t, rotationAbout(pivotOf(t), degrees));
}

// Repère propre de la cible : sa boîte droite (rotation retirée autour du centre) et son angle ; celui de la
// page pour un groupe. Dimensionner et étirer s'y font, comme les poignées de l'outil Sélection
// (selecttool.cpp, gestureTransform).
struct Frame
{
    RectF box;
    double degrees = 0;
    PointF center;
};

Frame frameOf(const Target& t)
{
    const double degrees =
        std::holds_alternative<Group>(body(t.object)) ? 0.0 : objectAngle(t.object.common.transform);
    const PointF c = t.box.center();
    if (degrees == 0.0) {
        return {t.box, 0.0, c};
    }
    PageObject upright = t.object;
    upright.common.transform = compose(upright.common.transform, rotationAbout(c, -degrees));
    return {boundsOnPage(upright), degrees, c};
}

std::unique_ptr<UndoCommand> scaleIn(const Target& t, const Frame& f, Anchor a, double sx, double sy)
{
    if (!resizable(t) || !(sx > 0) || !(sy > 0)) {
        return nullptr;
    }
    const Transform scale = scalingAbout(at(f.box, a), sx, sy);
    if (f.degrees == 0.0) {
        return transformBy(t, scale);
    }
    return transformBy(
        t, compose(compose(rotationAbout(f.center, -f.degrees), scale), rotationAbout(f.center, f.degrees)));
}

RectF unite(const RectF& a, const RectF& b) noexcept
{
    const double left = std::min(a.x, b.x);
    const double top = std::min(a.y, b.y);
    return {left, top, std::max(a.x + a.width, b.x + b.width) - left,
            std::max(a.y + a.height, b.y + b.height) - top};
}

// Page lue par fitTransforms, la cible et ses membres déverrouillés : H8-11, une action agit aussi sur un
// objet verrouillé (le verrou garde contre la main de l'utilisateur, pas contre les actions).
Page unlockedCopy(const Page& page, const PageObject& object)
{
    Page copy = page;
    std::vector<ObjectId> ids = expandGroups(page, {object.common.id});
    ids.push_back(object.common.id);
    for (const ObjectId id : ids) {
        const std::size_t i = indexOf(copy, id);
        if (i < copy.objects.size()) {
            copy.objects[i].common.locked = false;
        }
    }
    return copy;
}

std::unique_ptr<UndoCommand> fit(const Target& t, FitMode mode)
{
    if (!resizable(t)) {
        return nullptr;
    }
    const Page page = unlockedCopy(t.page, t.object);
    TransformList list = fitTransforms(page, {t.object.common.id}, mode);
    if (!movable(t) && !list.empty()) {
        // Déplacement bloqué : réduit autour de son centre, sans le décalage qui le ramène dans la page.
        std::optional<RectF> after;
        for (const auto& entry : list) {
            const std::size_t i = indexOf(page, entry.first);
            if (i >= page.objects.size()) {
                continue;
            }
            PageObject moved = page.objects[i];
            moved.common.transform = entry.second;
            if (hasExtent(moved)) {
                after = after ? unite(*after, boundsOnPage(moved)) : boundsOnPage(moved);
            }
        }
        if (after) {
            const PointF from = after->center();
            const PointF to = t.box.center();
            const Transform back = translation(to.x - from.x, to.y - from.y);
            for (auto& entry : list) {
                entry.second = compose(entry.second, back);
            }
        }
    }
    return transformEach(t, std::move(list));
}

// H8-10 : reflet en place autour du centre de la boîte, en une étape. Suffixe d'axe lu comme les commandes de
// menu (`transform.flip-x`, `transform.mirror-x` → Axis::X, haut / bas ; objectediting.cpp). `object.flip-*`
// fait le même reflet, sans écarter les objets verrouillés (H8-11) ; le menu `transform.mirror-*` ajoute,
// lui, une copie.
std::unique_ptr<UndoCommand> reflect(const Target& t, Axis axis)
{
    const bool upDown = axis == Axis::X;
    return transformBy(t, scalingAbout(t.box.center(), upDown ? 1.0 : -1.0, upDown ? -1.0 : 1.0));
}

std::unordered_map<std::string, Builder> makeTable()
{
    std::unordered_map<std::string, Builder> table;
    table["object.angle"] = [](const Target& t) { return turn(t, true); };
    table["object.angle-step"] = [](const Target& t) { return turn(t, false); };
    table["object.place-step"] = [](const Target& t) -> std::unique_ptr<UndoCommand> {
        const std::optional<double> dx = number(t.parameters, "dx");
        const std::optional<double> dy = number(t.parameters, "dy");
        if (!movable(t) || !dx || !dy) {
            return nullptr;
        }
        return transformBy(t, translation(*dx, *dy));
    };
    for (const auto& [name, anchor] : kAnchors) {
        const std::string suffix = name;
        const Anchor a = anchor;
        table["object.place-" + suffix] = [a](const Target& t) -> std::unique_ptr<UndoCommand> {
            if (!movable(t)) {
                return nullptr;
            }
            const PointF to = at({0, 0, t.context.pageWidth, t.context.pageHeight}, a);
            const PointF from = at(t.box, a);
            return transformBy(t, translation(to.x - from.x, to.y - from.y));
        };
        table["object.size-" + suffix] = [a](const Target& t) -> std::unique_ptr<UndoCommand> {
            const std::optional<double> width = number(t.parameters, "width");
            const Frame f = frameOf(t);
            if (!width || *width < 1 || f.box.width < kEps) {
                return nullptr;
            }
            const double k = *width / f.box.width;
            return scaleIn(t, f, a, k, k);
        };
        table["object.size-step-" + suffix] = [a](const Target& t) -> std::unique_ptr<UndoCommand> {
            const std::optional<double> factor = number(t.parameters, "factor");
            if (!factor) {
                return nullptr;
            }
            return scaleIn(t, frameOf(t), a, *factor, *factor);
        };
        table["object.stretch-" + suffix] = [a](const Target& t) -> std::unique_ptr<UndoCommand> {
            const std::optional<double> width = number(t.parameters, "width");
            const std::optional<double> height = number(t.parameters, "height");
            const Frame f = frameOf(t);
            if (!width || !height || *width < 1 || *height < 1 || f.box.width < kEps || f.box.height < kEps) {
                return nullptr;
            }
            return scaleIn(t, f, a, *width / f.box.width, *height / f.box.height);
        };
        table["object.stretch-step-" + suffix] = [a](const Target& t) -> std::unique_ptr<UndoCommand> {
            const std::optional<double> fx = number(t.parameters, "ratioX");
            const std::optional<double> fy = number(t.parameters, "ratioY");
            if (!fx || !fy) {
                return nullptr;
            }
            return scaleIn(t, frameOf(t), a, *fx, *fy);
        };
    }
    table["object.mirror-x"] = [](const Target& t) { return reflect(t, Axis::X); };
    table["object.mirror-y"] = [](const Target& t) { return reflect(t, Axis::Y); };
    table["object.flip-x"] = [](const Target& t) { return reflect(t, Axis::X); };
    table["object.flip-y"] = [](const Target& t) { return reflect(t, Axis::Y); };
    table["object.original-size"] = [](const Target& t) { return fit(t, FitMode::OriginalSize); };
    table["object.fit-width"] = [](const Target& t) { return fit(t, FitMode::Width); };
    table["object.fit-height"] = [](const Target& t) { return fit(t, FitMode::Height); };
    table["object.fit-page"] = [](const Target& t) { return fit(t, FitMode::Page); };
    return table;
}

const std::unordered_map<std::string, Builder>& geometryTable()
{
    static const std::unordered_map<std::string, Builder> table = makeTable();
    return table;
}

} // namespace

bool isGeometryAction(const std::string& id) noexcept
{
    const auto& table = geometryTable();
    return table.find(id) != table.end();
}

std::unique_ptr<UndoCommand> editActionCommand(const Document& document, const std::string& id,
                                               const std::string& parameters,
                                               const ActionContext& context); // objectactions_edit.cpp

std::unique_ptr<UndoCommand> objectActionCommand(const Document& document, const std::string& id,
                                                 const std::string& parameters, const ActionContext& context)
{
    if (isEditAction(id)) {
        return editActionCommand(document, id, parameters, context); // L8-T8
    }
    const auto& table = geometryTable();
    const auto builder = table.find(id);
    if (builder == table.end() || context.page >= document.pageCount()) {
        return nullptr;
    }
    const Page& page = document.page(context.page);
    const ObjectId targetId = context.target != 0 ? context.target : context.source;
    const std::size_t index = indexOf(page, targetId);
    if (targetId == 0 || index >= page.objects.size() || !hasExtent(page, page.objects[index])) {
        return nullptr;
    }
    QJsonObject values;
    if (!parameters.empty()) {
        const QJsonDocument json = QJsonDocument::fromJson(QByteArray::fromStdString(parameters));
        if (!json.isObject()) {
            return nullptr;
        }
        values = json.object();
    }
    const PageObject& object = page.objects[index];
    const Target target{page, object, boundsOnPage(page, object), context, values};
    return builder->second(target);
}

} // namespace actcraft::core
