#include <actcraft/core/alignment.h>
#include <actcraft/core/arrange.h>
#include <actcraft/core/edits.h>
#include <actcraft/core/objectactions.h>
#include <actcraft/core/objectedits.h>

#include <QByteArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QString>
#include <QtMath>

#include <cmath>
#include <functional>
#include <optional>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>

// Actions d'objet de géométrie (L8-T7). Aucune commande neuve : angle, placer, dimensionner, étirer et
// refléter rendent un TransformObjectsCommand (delta en coordonnées de page, comme l'outil Sélection) ;
// retourner, ajuster, taille d'origine reprennent le chemin des commandes de menu
// `transform.*` (alignment.h, ui/src/objectediting.cpp), pour faire la même chose qu'elles.
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

// Degrés, sens horaire à l'écran (convention de rotation(), model.h).
double angleOf(const Transform& t) noexcept
{
    return qRadiansToDegrees(std::atan2(t.m12, t.m11));
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
    const double degrees = absolute ? *angle - angleOf(t.object.common.transform) : *angle;
    return transformBy(t, rotationAbout(pivotOf(t), degrees));
}

std::unique_ptr<UndoCommand> scaleAbout(const Target& t, Anchor a, double sx, double sy)
{
    if (!resizable(t) || !(sx > 0) || !(sy > 0)) {
        return nullptr;
    }
    return transformBy(t, scalingAbout(at(t.box, a), sx, sy));
}

std::unique_ptr<UndoCommand> fit(const Target& t, FitMode mode)
{
    if (!resizable(t)) {
        return nullptr;
    }
    return transformEach(t, fitTransforms(t.page, {t.object.common.id}, mode));
}

// Reflet en place autour du centre de la boîte, en une étape (plan, `mirrorIsOneStep`) : x renverse la
// largeur, y la hauteur. La commande de menu `transform.mirror-*` ajoute, elle, une copie miroir
// (mirrorCopies) : écart consigné, non tranché ici.
std::unique_ptr<UndoCommand> mirror(const Target& t, bool horizontal)
{
    return transformBy(t, scalingAbout(t.box.center(), horizontal ? -1.0 : 1.0, horizontal ? 1.0 : -1.0));
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
            if (!width || *width < 1 || t.box.width < kEps) {
                return nullptr;
            }
            const double f = *width / t.box.width;
            return scaleAbout(t, a, f, f);
        };
        table["object.size-step-" + suffix] = [a](const Target& t) -> std::unique_ptr<UndoCommand> {
            const std::optional<double> factor = number(t.parameters, "factor");
            if (!factor) {
                return nullptr;
            }
            return scaleAbout(t, a, *factor, *factor);
        };
        table["object.stretch-" + suffix] = [a](const Target& t) -> std::unique_ptr<UndoCommand> {
            const std::optional<double> width = number(t.parameters, "width");
            const std::optional<double> height = number(t.parameters, "height");
            if (!width || !height || *width < 1 || *height < 1 || t.box.width < kEps || t.box.height < kEps) {
                return nullptr;
            }
            return scaleAbout(t, a, *width / t.box.width, *height / t.box.height);
        };
        table["object.stretch-step-" + suffix] = [a](const Target& t) -> std::unique_ptr<UndoCommand> {
            const std::optional<double> fx = number(t.parameters, "factorX");
            const std::optional<double> fy = number(t.parameters, "factorY");
            if (!fx || !fy) {
                return nullptr;
            }
            return scaleAbout(t, a, *fx, *fy);
        };
    }
    table["object.mirror-x"] = [](const Target& t) { return mirror(t, true); };
    table["object.mirror-y"] = [](const Target& t) { return mirror(t, false); };
    // Suffixe d'axe lu comme les commandes de menu (`transform.flip-x` → Axis::X, objectediting.cpp).
    table["object.flip-x"] = [](const Target& t) {
        return transformEach(t, flipTransforms(t.page, {t.object.common.id}, Axis::X));
    };
    table["object.flip-y"] = [](const Target& t) {
        return transformEach(t, flipTransforms(t.page, {t.object.common.id}, Axis::Y));
    };
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

std::unique_ptr<UndoCommand> objectActionCommand(const Document& document, const std::string& id,
                                                 const std::string& parameters, const ActionContext& context)
{
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
