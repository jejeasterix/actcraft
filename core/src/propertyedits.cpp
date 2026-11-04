#include <actcraft/core/document.h>
#include <actcraft/core/edits.h>
#include <actcraft/core/hittest.h>
#include <actcraft/core/objectedits.h>
#include <actcraft/core/propertyedits.h>

#include <cmath>
#include <numbers>
#include <type_traits>
#include <utility>

namespace actcraft::core {

namespace {

bool isGeometry(ObjectProperty property) noexcept
{
    switch (property) {
    case ObjectProperty::Left:
    case ObjectProperty::Top:
    case ObjectProperty::Width:
    case ObjectProperty::Height:
    case ObjectProperty::Angle:
    case ObjectProperty::Mirrored:
        return true;
    default:
        return false;
    }
}

bool isMirrored(const Transform& t) noexcept
{
    return t.m11 * t.m22 - t.m12 * t.m21 < 0;
}

// 0 … 360. En miroir, la transformation est le retournement (scalingAbout(-1, 1)) suivi d'une rotation :
// l'angle est celui de la rotation, sans le demi-tour que le retournement ajoute à atan2(m12, m11).
double angleOf(const Transform& t) noexcept
{
    const double radians = isMirrored(t) ? std::atan2(-t.m12, -t.m11) : std::atan2(t.m12, t.m11);
    double degrees = radians * 180.0 / std::numbers::pi;
    if (degrees < 0) {
        degrees += 360.0;
    }
    return degrees >= 360.0 || degrees == 0.0 ? 0.0 : degrees; // −0 compris
}

bool sameTransform(const Transform& s, const Transform& t) noexcept
{
    return s.m11 == t.m11 && s.m12 == t.m12 && s.m21 == t.m21 && s.m22 == t.m22 && s.dx == t.dx &&
           s.dy == t.dy;
}

// Contour et remplissage, selon le type : une forme lue (Path) dessine avec `pen` et `brush`, les autres
// formes avec leurs champs de couleur ; un trait n'a qu'une couleur et une épaisseur. nullptr : ne s'applique
// pas.
template <typename Object>
std::conditional_t<std::is_const_v<Object>, const Outline, Outline>* penOf(Object& object) noexcept
{
    auto& variant = body(object);
    if (auto* shape = std::get_if<Shape>(&variant); shape != nullptr && shape->kind == ShapeKind::Path) {
        return &shape->pen;
    }
    if (auto* line = std::get_if<Line>(&variant)) {
        return &line->pen;
    }
    if (auto* connector = std::get_if<Connector>(&variant)) {
        return &connector->pen;
    }
    return nullptr;
}

template <typename T, typename Field> bool assign(Field& field, const PropertyValue& value)
{
    const T* wanted = std::get_if<T>(&value);
    if (wanted == nullptr || field == *wanted) {
        return false;
    }
    field = *wanted;
    return true;
}

bool positive(double v) noexcept
{
    return std::isfinite(v) && v > 0;
}

// Pose la valeur sur l'objet ; vrai si l'objet a changé. Propriétés déléguées et valeurs d'un autre type :
// rien.
bool write(PageObject& object, ObjectProperty property, const PropertyValue& value)
{
    ObjectCommon& common = object.common;
    Shape* shape = std::get_if<Shape>(&body(object));
    const bool plainShape = shape != nullptr && shape->kind != ShapeKind::Path;
    Stroke* stroke = std::get_if<Stroke>(&body(object));
    Outline* pen = penOf(object);
    switch (property) {
    case ObjectProperty::Name:
        return assign<std::string>(common.name, value);
    case ObjectProperty::Keywords:
        return assign<std::vector<std::string>>(common.keywords, value);
    case ObjectProperty::Label:
        return assign<std::string>(common.label, value);
    case ObjectProperty::LabelStyle:
        return assign<LabelStyle>(common.labelStyle, value);
    case ObjectProperty::Layer:
    case ObjectProperty::Translucency:
    case ObjectProperty::Visible:
    case ObjectProperty::Locked:
        return false; // amendement 1, point 15 : SetLayerCommand, SetTranslucencyCommand,
                      // SetObjectFlagCommand
    case ObjectProperty::OutlineStyle:
        return pen != nullptr && assign<PenStyle>(pen->style, value);
    case ObjectProperty::OutlineColor:
        if (plainShape) {
            return assign<std::uint32_t>(shape->outline, value);
        }
        if (stroke != nullptr) {
            return assign<std::uint32_t>(stroke->color, value);
        }
        return pen != nullptr && assign<std::uint32_t>(pen->color, value);
    case ObjectProperty::OutlineWidth:
        if (plainShape) {
            return assign<double>(shape->outlineWidth, value);
        }
        if (stroke != nullptr) {
            return assign<double>(stroke->baseWidth, value);
        }
        return pen != nullptr && assign<double>(pen->width, value);
    case ObjectProperty::FillStyle:
        return shape != nullptr && !plainShape && assign<FillKind>(shape->brush.kind, value);
    case ObjectProperty::FillColor:
        if (plainShape) {
            return assign<std::uint32_t>(shape->fill, value);
        }
        return shape != nullptr && assign<std::uint32_t>(shape->brush.color, value);
    case ObjectProperty::Left:
    case ObjectProperty::Top:
    case ObjectProperty::Width:
    case ObjectProperty::Height:
    case ObjectProperty::Angle:
    case ObjectProperty::Mirrored:
        break;
    }

    // Position, taille, angle, miroir : par la transformation, d'après la boîte transformée (boundsOnPage),
    // comme TransformObjectsCommand (delta en coordonnées de page, appliqué après la transformation de
    // l'objet).
    if (!hasExtent(object)) {
        return false; // groupe, objet sans géométrie
    }
    const RectF box = boundsOnPage(object);
    Transform delta;
    if (property == ObjectProperty::Mirrored) {
        const bool* mirrored = std::get_if<bool>(&value);
        if (mirrored == nullptr || *mirrored == isMirrored(common.transform)) {
            return false;
        }
        delta = scalingAbout(box.center(), -1, 1);
    } else {
        const double* number = std::get_if<double>(&value);
        if (number == nullptr || !std::isfinite(*number)) {
            return false;
        }
        const double v = *number;
        switch (property) {
        case ObjectProperty::Left:
            delta = translation(v - box.x, 0);
            break;
        case ObjectProperty::Top:
            delta = translation(0, v - box.y);
            break;
        case ObjectProperty::Width:
            if (!positive(v) || !positive(box.width)) {
                return false;
            }
            delta = scalingAbout({box.x, box.y}, v / box.width, 1);
            break;
        case ObjectProperty::Height:
            if (!positive(v) || !positive(box.height)) {
                return false;
            }
            delta = scalingAbout({box.x, box.y}, 1, v / box.height);
            break;
        default: // Angle
            delta = rotationAbout(box.center(), v - angleOf(common.transform));
            break;
        }
    }
    if (delta.isIdentity()) {
        return false;
    }
    common.transform = compose(common.transform, delta);
    return true;
}

// Saisie refusée : épaisseur non finie ou négative, nulle pour un trait. Annuler n'y passe pas : il remet la
// valeur lue sur l'objet.
bool refused(const PageObject& object, ObjectProperty property, const PropertyValue& value)
{
    const double* width = std::get_if<double>(&value);
    if (property != ObjectProperty::OutlineWidth || width == nullptr) {
        return false;
    }
    return !std::isfinite(*width) || *width < 0 ||
           (*width == 0 && std::holds_alternative<Stroke>(body(object)));
}

// Ce qu'Annuler rétablira : la valeur de la propriété, ou la transformation pour la géométrie ; rien si la
// propriété ne s'applique pas à l'objet.
using Before = std::variant<PropertyValue, Transform>;

std::optional<Before> valueBefore(const PageObject& object, ObjectProperty property)
{
    if (isGeometry(property)) {
        return hasExtent(object) ? std::optional<Before>(object.common.transform) : std::nullopt;
    }
    std::optional<PropertyValue> value = readProperty(object, property);
    return value ? std::optional<Before>(std::move(*value)) : std::nullopt;
}

// Vrai si l'objet a changé.
bool restore(PageObject& object, ObjectProperty property, const Before& before)
{
    if (const auto* transform = std::get_if<Transform>(&before)) {
        if (sameTransform(object.common.transform, *transform)) {
            return false;
        }
        object.common.transform = *transform;
        return true;
    }
    return write(object, property, std::get<PropertyValue>(before));
}

Change changeFor(ObjectProperty property, std::size_t page, ObjectId id, const RectF& before,
                 const RectF& after)
{
    if (!propertyTouchesTheCache(property)) {
        return Change{Change::Kind::PageSettingsChanged, page, id, RectF{}};
    }
    return Change{Change::Kind::ObjectChanged, page, id, unitedRect(before, after)};
}

bool validPage(const Document& document, int page) noexcept
{
    return page >= 0 && static_cast<std::size_t>(page) < document.pageCount();
}

bool pageTouchesTheCache(PageProperty property) noexcept
{
    return property == PageProperty::Width || property == PageProperty::Height;
}

bool writePage(Page& page, PageProperty property, const PropertyValue& value)
{
    switch (property) {
    case PageProperty::Name:
        return assign<std::string>(page.name, value);
    case PageProperty::Description:
        return assign<std::string>(page.description, value);
    case PageProperty::Keywords:
        return assign<std::vector<std::string>>(page.keywords, value);
    case PageProperty::Width:
    case PageProperty::Height: {
        const double* size = std::get_if<double>(&value);
        if (size == nullptr || !positive(*size)) {
            return false;
        }
        return assign<double>(property == PageProperty::Width ? page.width : page.height, value);
    }
    }
    return false;
}

Change pageChangeFor(PageProperty property, std::size_t page)
{
    const Change::Kind kind =
        pageTouchesTheCache(property) ? Change::Kind::PageChanged : Change::Kind::PageSettingsChanged;
    return Change{kind, page, 0, RectF{}};
}

} // namespace

bool propertyTouchesTheCache(ObjectProperty property) noexcept
{
    switch (property) {
    case ObjectProperty::Name:
    case ObjectProperty::Keywords:
    case ObjectProperty::Label:
    case ObjectProperty::LabelStyle:
    case ObjectProperty::Locked:
        return false;
    default:
        return true;
    }
}

bool isDelegated(ObjectProperty property) noexcept
{
    return property == ObjectProperty::Layer || property == ObjectProperty::Visible ||
           property == ObjectProperty::Locked || property == ObjectProperty::Translucency;
}

SetObjectPropertiesCommand::SetObjectPropertiesCommand(int page, std::vector<ObjectId> objects,
                                                       ObjectProperty property, PropertyValue value)
    : m_page(page), m_objects(std::move(objects)), m_property(property), m_value(std::move(value))
{}

// Garde la valeur d'avant de chaque objet auquel la propriété s'applique, modifié ou non : une étape
// fusionnée dont le premier pas laissait un objet tel quel doit encore le rendre à son état d'avant le
// glissé.
void SetObjectPropertiesCommand::redo(Document& document, Changes& changes)
{
    m_old.clear();
    m_partlyRefused = false;
    if (isDelegated(m_property) || !validPage(document, m_page)) {
        return;
    }
    const auto pageIndex = static_cast<std::size_t>(m_page);
    Page& page = document.page(pageIndex);
    for (const ObjectId id : m_objects) {
        const std::size_t rank = indexOf(page, id);
        if (rank == page.objects.size()) {
            continue;
        }
        PageObject& object = page.objects[rank];
        std::optional<Before> before = valueBefore(object, m_property);
        if (!before) {
            continue;
        }
        m_old.emplace_back(id, std::move(*before));
        const RectF area = boundsOnPage(page, object);
        if (refused(object, m_property, m_value)) {
            m_partlyRefused = true;
        } else if (write(object, m_property, m_value)) {
            changes.push_back(changeFor(m_property, pageIndex, id, area, boundsOnPage(page, object)));
        }
    }
}

void SetObjectPropertiesCommand::undo(Document& document, Changes& changes)
{
    if (!validPage(document, m_page)) {
        return;
    }
    const auto pageIndex = static_cast<std::size_t>(m_page);
    Page& page = document.page(pageIndex);
    for (auto it = m_old.rbegin(); it != m_old.rend(); ++it) {
        const std::size_t rank = indexOf(page, it->first);
        if (rank == page.objects.size()) {
            continue;
        }
        PageObject& object = page.objects[rank];
        const RectF area = boundsOnPage(page, object);
        if (restore(object, m_property, it->second)) {
            changes.push_back(changeFor(m_property, pageIndex, it->first, area, boundsOnPage(page, object)));
        }
    }
}

int SetObjectPropertiesCommand::mergeId() const noexcept
{
    return kMergeObjectProperties;
}

// Réglette glissée : la commande garde son état d'avant et prend la valeur de la suivante (comme
// SetTranslucencyCommand).
bool SetObjectPropertiesCommand::mergeWith(const UndoCommand& other)
{
    const auto* next = dynamic_cast<const SetObjectPropertiesCommand*>(&other);
    if (next == nullptr || next->m_page != m_page || next->m_property != m_property ||
        next->m_objects != m_objects) {
        return false;
    }
    // Un pas refusé en partie (forme et trait, épaisseur 0) reste une étape à part : fusionné, il laisserait
    // une valeur refusée dans l'étape, et Refaire après Annuler ne rejouerait pas le glissé.
    if (next->m_partlyRefused) {
        return false;
    }
    m_value = next->m_value;
    return true;
}

SetPagePropertiesCommand::SetPagePropertiesCommand(int page, PageProperty property, PropertyValue value)
    : m_page(page), m_property(property), m_value(std::move(value))
{}

void SetPagePropertiesCommand::redo(Document& document, Changes& changes)
{
    if (!validPage(document, m_page)) {
        return;
    }
    const auto pageIndex = static_cast<std::size_t>(m_page);
    Page& page = document.page(pageIndex);
    m_old = readPageProperty(page, m_property);
    if (writePage(page, m_property, m_value)) {
        changes.push_back(pageChangeFor(m_property, pageIndex));
    }
}

void SetPagePropertiesCommand::undo(Document& document, Changes& changes)
{
    if (!validPage(document, m_page)) {
        return;
    }
    const auto pageIndex = static_cast<std::size_t>(m_page);
    if (writePage(document.page(pageIndex), m_property, m_old)) {
        changes.push_back(pageChangeFor(m_property, pageIndex));
    }
}

int SetPagePropertiesCommand::mergeId() const noexcept
{
    return kMergePageProperties;
}

bool SetPagePropertiesCommand::mergeWith(const UndoCommand& other)
{
    const auto* next = dynamic_cast<const SetPagePropertiesCommand*>(&other);
    if (next == nullptr || next->m_page != m_page || next->m_property != m_property) {
        return false;
    }
    m_value = next->m_value;
    return true;
}

std::optional<PropertyValue> readProperty(const PageObject& object, ObjectProperty property)
{
    const ObjectCommon& common = object.common;
    const Shape* shape = std::get_if<Shape>(&body(object));
    const bool plainShape = shape != nullptr && shape->kind != ShapeKind::Path;
    const Stroke* stroke = std::get_if<Stroke>(&body(object));
    const Outline* pen = penOf(object);
    switch (property) {
    case ObjectProperty::Name:
        return common.name;
    case ObjectProperty::Keywords:
        return common.keywords;
    case ObjectProperty::Label:
        return common.label;
    case ObjectProperty::LabelStyle:
        return common.labelStyle;
    case ObjectProperty::Layer:
        return common.layer;
    case ObjectProperty::Translucency:
        return static_cast<double>(common.translucency);
    case ObjectProperty::Visible:
        return common.visible;
    case ObjectProperty::Locked:
        return common.locked;
    case ObjectProperty::OutlineStyle:
        if (pen != nullptr) {
            return pen->style;
        }
        return std::nullopt;
    case ObjectProperty::OutlineColor:
        if (plainShape) {
            return shape->outline;
        }
        if (stroke != nullptr) {
            return stroke->color;
        }
        if (pen != nullptr) {
            return pen->color;
        }
        return std::nullopt;
    case ObjectProperty::OutlineWidth:
        if (plainShape) {
            return shape->outlineWidth;
        }
        if (stroke != nullptr) {
            return stroke->baseWidth;
        }
        if (pen != nullptr) {
            return pen->width;
        }
        return std::nullopt;
    case ObjectProperty::FillStyle:
        if (shape != nullptr && !plainShape) {
            return shape->brush.kind;
        }
        return std::nullopt;
    case ObjectProperty::FillColor:
        if (plainShape) {
            return shape->fill;
        }
        if (shape != nullptr) {
            return shape->brush.color;
        }
        return std::nullopt;
    case ObjectProperty::Left:
    case ObjectProperty::Top:
    case ObjectProperty::Width:
    case ObjectProperty::Height:
    case ObjectProperty::Angle:
    case ObjectProperty::Mirrored:
        break;
    }
    if (!hasExtent(object)) {
        return std::nullopt;
    }
    const RectF box = boundsOnPage(object);
    switch (property) {
    case ObjectProperty::Left:
        return box.x;
    case ObjectProperty::Top:
        return box.y;
    case ObjectProperty::Width:
        return box.width;
    case ObjectProperty::Height:
        return box.height;
    case ObjectProperty::Angle:
        return angleOf(common.transform);
    default:
        return isMirrored(common.transform);
    }
}

PropertyValue readPageProperty(const Page& page, PageProperty property)
{
    switch (property) {
    case PageProperty::Name:
        return page.name;
    case PageProperty::Description:
        return page.description;
    case PageProperty::Keywords:
        return page.keywords;
    case PageProperty::Width:
        return page.width;
    case PageProperty::Height:
        return page.height;
    }
    return std::string();
}

} // namespace actcraft::core
