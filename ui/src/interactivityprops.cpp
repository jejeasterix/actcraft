// L8-T17 : sections de propriétés de l'interactivité (F-577 à F-579, F-582).
#include <actcraft/core/document.h>
#include <actcraft/core/interactivity.h>
#include <actcraft/ui/interactivityprops.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <optional>
#include <string>
#include <utility>
#include <variant>

namespace actcraft::ui {

namespace {

constexpr double kNoBound = std::numeric_limits<double>::quiet_NaN();

constexpr InteractivityRow row(const char* section, const char* key, const char* label,
                               InteractivityEditor editor, InteractivityField field, double min = kNoBound,
                               double max = kNoBound)
{
    return {section, key, label, editor, field, min, max};
}

using E = InteractivityEditor;
using F = InteractivityField;

// Ordre d'affichage : les cinq sections dans l'ordre de la spécification, chacune d'un seul tenant.
const std::array kRows = {
    row("actions", "actions", "Actions", E::Actions, F::Actions),
    row("container", "accepts", "Peut contenir", E::Choice, F::Accepts),
    row("container", "contained-object", "Objet accepté", E::Object, F::ContainedObject),
    row("container", "contain-words", "Mots acceptés", E::Keywords, F::ContainWords),
    row("container", "fit", "Contenu quand", E::Choice, F::Fit),
    row("container", "tolerance", "Écart des centres", E::Number, F::Tolerance, 0, 1000),
    row("container", "bounce-back", "Revenir si non contenu", E::Bool, F::BounceBack),
    row("container", "chime", "Son de confirmation", E::Bool, F::Chime),
    row("rotation", "turning", "Peut pivoter", E::Choice, F::Turning),
    row("rotation", "turn-step", "Pas de rotation", E::Number, F::TurnStep, 0, 360),
    row("rotation", "pivot", "Centre de rotation", E::Choice, F::Pivot),
    row("rotation", "pivot-object", "Objet pivot", E::Object, F::PivotObject),
    row("restrictors", "travel", "Peut se déplacer", E::Choice, F::Travel),
    row("restrictors", "track", "Chemin suivi", E::Object, F::Track),
    row("restrictors", "blocks-others", "Arrête les autres objets", E::Bool, F::BlocksOthers),
    row("restrictors", "magnet", "Se poser sur un repère", E::Bool, F::Magnet),
    row("restrictors", "magnet-mark", "Repère de pose", E::Choice, F::MagnetMark),
    row("multimedia", "play-on-open", "Lire à l'ouverture de la page", E::Bool, F::PlayOnOpen),
    row("multimedia", "loop", "Lire en boucle", E::Bool, F::Loop),
    row("multimedia", "controls-hidden", "Masquer les commandes", E::Bool, F::ControlsHidden),
    row("multimedia", "aspect-locked", "Garder les proportions", E::Bool, F::AspectLocked),
    row("multimedia", "clear-background", "Fond transparent", E::Bool, F::ClearBackground),
    row("multimedia", "wrap-text", "Habiller le texte", E::Bool, F::WrapText),
};

bool isMediaField(InteractivityField field) noexcept
{
    switch (field) {
    case F::PlayOnOpen:
    case F::Loop:
    case F::ControlsHidden:
    case F::AspectLocked:
    case F::ClearBackground:
    case F::WrapText:
        return true;
    default:
        return false;
    }
}

const core::Media* mediaOf(const core::PageObject& object) noexcept
{
    return std::get_if<core::Media>(&static_cast<const core::ObjectVariant&>(object));
}

core::Media* mediaOf(core::PageObject& object) noexcept
{
    return std::get_if<core::Media>(&static_cast<core::ObjectVariant&>(object));
}

SetInteractivityCommand::MediaOptions optionsOf(const core::PageObject& object) noexcept
{
    const core::Media* media = mediaOf(object);
    if (media == nullptr) {
        return {};
    }
    return {media->playOnOpen,      media->loop,    media->controlsHidden, media->aspectLocked,
            media->clearBackground, media->wrapText};
}

void setOptions(core::PageObject& object, const SetInteractivityCommand::MediaOptions& options) noexcept
{
    core::Media* media = mediaOf(object);
    if (media == nullptr) {
        return;
    }
    media->playOnOpen = options.playOnOpen;
    media->loop = options.loop;
    media->controlsHidden = options.controlsHidden;
    media->aspectLocked = options.aspectLocked;
    media->clearBackground = options.clearBackground;
    media->wrapText = options.wrapText;
}

bool samePoint(const core::PointF& a, const core::PointF& b) noexcept
{
    return a.x == b.x && a.y == b.y;
}

// Champ par champ : Restrictions et ContainerRule n'ont pas d'opérateur ==.
bool sameRestrictions(const core::Restrictions& a, const core::Restrictions& b) noexcept
{
    return a.movable == b.movable && a.resizable == b.resizable && a.rotatable == b.rotatable &&
           a.travel == b.travel && a.track == b.track && a.turning == b.turning && a.turnStep == b.turnStep &&
           a.pivot == b.pivot && samePoint(a.pivotPoint, b.pivotPoint) && a.pivotObject == b.pivotObject &&
           a.blocksOthers == b.blocksOthers && a.magnet == b.magnet && a.magnetMark == b.magnetMark &&
           samePoint(a.magnetPoint, b.magnetPoint);
}

bool sameContainer(const core::ContainerRule& a, const core::ContainerRule& b) noexcept
{
    return a.accepts == b.accepts && a.object == b.object && a.keywords == b.keywords && a.fit == b.fit &&
           a.tolerance == b.tolerance && a.bounceBack == b.bounceBack && a.chime == b.chime &&
           a.chimeSound == b.chimeSound;
}

core::PageObject* find(core::Page& page, core::ObjectId id) noexcept
{
    for (core::PageObject& object : page.objects) {
        if (object.common.id == id) {
            return &object;
        }
    }
    return nullptr;
}

const core::PageObject* find(const core::Page& page, core::ObjectId id) noexcept
{
    for (const core::PageObject& object : page.objects) {
        if (object.common.id == id) {
            return &object;
        }
    }
    return nullptr;
}

// Écrit la valeur ; les modes passent par la forme normalisée (les booléens du lot 2 suivent le mode).
void writeField(core::PageObject& object, InteractivityField field, const InteractivityValue& value)
{
    core::Restrictions& r = object.common.restrictions;
    core::ContainerRule& c = object.common.container;
    const auto choice = [&value] { return std::get<int>(value); };
    const auto flag = [&value] { return std::get<bool>(value); };
    const auto number = [&value] { return std::get<double>(value); };
    const auto target = [&value] { return std::get<core::ObjectId>(value); };
    core::Media* media = mediaOf(object);
    switch (field) {
    case F::Accepts:
        c.accepts = static_cast<core::Accepts>(choice());
        break;
    case F::ContainedObject:
        c.object = target();
        break;
    case F::Fit:
        c.fit = static_cast<core::FitRule>(choice());
        break;
    case F::Tolerance:
        c.tolerance = number();
        break;
    case F::BounceBack:
        c.bounceBack = flag();
        break;
    case F::Chime:
        c.chime = flag();
        break;
    case F::Turning:
        core::setTurning(r, static_cast<core::Turning>(choice()));
        core::normalize(r);
        break;
    case F::TurnStep:
        r.turnStep = number();
        break;
    case F::Pivot:
        r.pivot = static_cast<core::Mark>(choice());
        break;
    case F::PivotObject:
        r.pivotObject = target();
        break;
    case F::Travel:
        core::setTravel(r, static_cast<core::Travel>(choice()));
        core::normalize(r);
        break;
    case F::Track:
        r.track = target();
        break;
    case F::BlocksOthers:
        r.blocksOthers = flag();
        break;
    case F::Magnet:
        r.magnet = flag();
        break;
    case F::MagnetMark:
        r.magnetMark = static_cast<core::Mark>(choice());
        break;
    case F::PlayOnOpen:
    case F::Loop:
    case F::ControlsHidden:
    case F::AspectLocked:
    case F::ClearBackground:
    case F::WrapText:
        if (media == nullptr) {
            break;
        }
        switch (field) {
        case F::PlayOnOpen:
            media->playOnOpen = flag();
            break;
        case F::Loop:
            media->loop = flag();
            break;
        case F::ControlsHidden:
            media->controlsHidden = flag();
            break;
        case F::AspectLocked:
            media->aspectLocked = flag();
            break;
        case F::ClearBackground:
            media->clearBackground = flag();
            break;
        default:
            media->wrapText = flag();
            break;
        }
        break;
    case F::Actions:
    case F::ContainWords:
        break;
    }
}

QStringList markLabels()
{
    return {PropertyModel::tr("Centre"),           PropertyModel::tr("Coin en haut à gauche"),
            PropertyModel::tr("Milieu du haut"),   PropertyModel::tr("Coin en haut à droite"),
            PropertyModel::tr("Milieu de droite"), PropertyModel::tr("Coin en bas à droite"),
            PropertyModel::tr("Milieu du bas"),    PropertyModel::tr("Coin en bas à gauche"),
            PropertyModel::tr("Milieu de gauche"), PropertyModel::tr("Première ligne du texte"),
            PropertyModel::tr("Point choisi"),     PropertyModel::tr("Autre objet")};
}

QStringList toStringList(const std::vector<std::string>& values)
{
    QStringList list;
    list.reserve(static_cast<qsizetype>(values.size()));
    for (const std::string& value : values) {
        list.append(QString::fromStdString(value));
    }
    return list;
}

} // namespace

QStringList sections(const PropertyModel& model)
{
    QStringList keys;
    for (int i = 0; i < model.rowCount(); ++i) {
        const QString key = model.data(model.index(i), PropertyModel::SectionRole).toString();
        if (!keys.contains(key)) {
            keys.append(key);
        }
    }
    return keys;
}

std::span<const InteractivityRow> interactivityRows()
{
    return kRows;
}

QString interactivitySectionLabel(const QString& section)
{
    if (section == QLatin1String("actions")) {
        return PropertyModel::tr("Actions");
    }
    if (section == QLatin1String("container")) {
        return PropertyModel::tr("Conteneur");
    }
    if (section == QLatin1String("rotation")) {
        return PropertyModel::tr("Rotation");
    }
    if (section == QLatin1String("restrictors")) {
        return PropertyModel::tr("Limiteurs");
    }
    if (section == QLatin1String("multimedia")) {
        return PropertyModel::tr("Multimédia");
    }
    return {};
}

QString interactivityEditorKey(InteractivityEditor editor)
{
    switch (editor) {
    case E::Actions:
        return QStringLiteral("actions");
    case E::Choice:
        return QStringLiteral("choice");
    case E::Bool:
        return QStringLiteral("bool");
    case E::Number:
        return QStringLiteral("number");
    case E::Object:
        return QStringLiteral("object");
    case E::Keywords:
        return QStringLiteral("keywords");
    }
    return {};
}

QStringList interactivityChoices(InteractivityField field)
{
    switch (field) {
    case F::Accepts: // indice = core::Accepts
        return {PropertyModel::tr("Rien"), PropertyModel::tr("N'importe quel objet"),
                PropertyModel::tr("Un objet précis"), PropertyModel::tr("Des objets aux mots-clés acceptés")};
    case F::Fit: // core::FitRule
        return {PropertyModel::tr("Les centres se rejoignent"),
                PropertyModel::tr("Il est entièrement dedans")};
    case F::Turning: // core::Turning
        return {PropertyModel::tr("Librement"), PropertyModel::tr("Dans le sens des aiguilles d'une montre"),
                PropertyModel::tr("Dans le sens inverse"), PropertyModel::tr("Non")};
    case F::Travel: // core::Travel
        return {PropertyModel::tr("Librement"), PropertyModel::tr("Verticalement"),
                PropertyModel::tr("Horizontalement"), PropertyModel::tr("Le long d'un chemin"),
                PropertyModel::tr("Non")};
    case F::Pivot: // core::Mark
    case F::MagnetMark:
        return markLabels();
    default:
        return {};
    }
}

bool appliesTo(const InteractivityRow& row, const core::PageObject& object)
{
    return !isMediaField(row.field) || mediaOf(object) != nullptr;
}

QVariant readInteractivity(const core::PageObject& object, InteractivityField field)
{
    core::Restrictions r = object.common.restrictions;
    core::normalize(r); // forme normalisée : ce que lisent les outils
    const core::ContainerRule& c = object.common.container;
    const SetInteractivityCommand::MediaOptions media = optionsOf(object);
    const auto id = [](core::ObjectId value) { return QVariant(static_cast<qulonglong>(value)); };
    switch (field) {
    case F::Actions: {
        QStringList list;
        for (const core::Action& action : object.common.actions) {
            list.append(action.catalogId.empty() ? PropertyModel::tr("Action non prise en charge")
                                                 : QString::fromStdString(action.catalogId));
        }
        return list;
    }
    case F::Accepts:
        return static_cast<int>(c.accepts);
    case F::ContainedObject:
        return id(c.object);
    case F::ContainWords:
        return toStringList(object.common.keywords); // lot 6, jamais c.keywords
    case F::Fit:
        return static_cast<int>(c.fit);
    case F::Tolerance:
        return c.tolerance;
    case F::BounceBack:
        return c.bounceBack;
    case F::Chime:
        return c.chime;
    case F::Turning:
        return static_cast<int>(r.turning);
    case F::TurnStep:
        return r.turnStep;
    case F::Pivot:
        return static_cast<int>(r.pivot);
    case F::PivotObject:
        return id(r.pivotObject);
    case F::Travel:
        return static_cast<int>(r.travel);
    case F::Track:
        return id(r.track);
    case F::BlocksOthers:
        return r.blocksOthers;
    case F::Magnet:
        return r.magnet;
    case F::MagnetMark:
        return static_cast<int>(r.magnetMark);
    case F::PlayOnOpen:
        return media.playOnOpen;
    case F::Loop:
        return media.loop;
    case F::ControlsHidden:
        return media.controlsHidden;
    case F::AspectLocked:
        return media.aspectLocked;
    case F::ClearBackground:
        return media.clearBackground;
    case F::WrapText:
        return media.wrapText;
    }
    return {};
}

bool isEnabled(const core::PageObject& object, InteractivityField field)
{
    core::Restrictions r = object.common.restrictions;
    core::normalize(r);
    const core::ContainerRule& c = object.common.container;
    switch (field) {
    case F::ContainedObject:
        return c.accepts == core::Accepts::OneObject;
    case F::ContainWords:
        return c.accepts == core::Accepts::Keywords;
    case F::Fit:
    case F::BounceBack:
    case F::Chime:
        return c.accepts != core::Accepts::Nothing;
    case F::Tolerance:
        return c.accepts != core::Accepts::Nothing && c.fit == core::FitRule::CentresMatch;
    case F::TurnStep:
    case F::Pivot:
        return r.turning != core::Turning::Fixed;
    case F::PivotObject:
        return r.turning != core::Turning::Fixed && r.pivot == core::Mark::OtherObject;
    case F::Track:
        return r.travel == core::Travel::AlongTrack;
    case F::MagnetMark:
        return r.magnet;
    default:
        return true;
    }
}

SetInteractivityCommand::SetInteractivityCommand(std::size_t page, std::vector<core::ObjectId> ids,
                                                 InteractivityField field, InteractivityValue value)
    : m_page(page), m_ids(std::move(ids)), m_field(field), m_value(value)
{}

void SetInteractivityCommand::redo(core::Document& document, core::Changes& changes)
{
    m_before.clear();
    core::Page& page = document.page(m_page);
    for (const core::ObjectId id : m_ids) {
        core::PageObject* object = find(page, id);
        if (object == nullptr || (isMediaField(m_field) && mediaOf(*object) == nullptr)) {
            continue;
        }
        Before before{id, object->common.restrictions, object->common.container, optionsOf(*object)};
        writeField(*object, m_field, m_value);
        if (sameRestrictions(before.restrictions, object->common.restrictions) &&
            sameContainer(before.container, object->common.container) && before.media == optionsOf(*object)) {
            continue; // déjà à cette valeur (ou identifiant répété)
        }
        m_before.push_back(std::move(before));
        changes.push_back(core::Change{core::Change::Kind::PageSettingsChanged, m_page, id, core::RectF{}});
    }
}

void SetInteractivityCommand::undo(core::Document& document, core::Changes& changes)
{
    core::Page& page = document.page(m_page);
    for (auto it = m_before.rbegin(); it != m_before.rend(); ++it) {
        core::PageObject* object = find(page, it->id);
        if (object == nullptr) {
            continue;
        }
        object->common.restrictions = it->restrictions;
        object->common.container = it->container;
        setOptions(*object, it->media);
        changes.push_back(
            core::Change{core::Change::Kind::PageSettingsChanged, m_page, it->id, core::RectF{}});
    }
}

std::unique_ptr<core::UndoCommand> interactivityCommand(const core::Page& page, std::size_t pageIndex,
                                                        std::vector<core::ObjectId> ids,
                                                        const InteractivityRow& row, const QVariant& value)
{
    if (!value.isValid() || ids.empty()) {
        return nullptr;
    }
    std::optional<InteractivityValue> converted;
    switch (row.editor) {
    case E::Actions:
    case E::Keywords:
        return nullptr; // lecture seule ; mots acceptés : chemin des mots-clés du lot 6
    case E::Bool:
        if (value.canConvert<bool>()) {
            converted = value.toBool();
        }
        break;
    case E::Choice: {
        bool ok = false;
        const int index = value.toInt(&ok);
        if (ok && index >= 0 && index < interactivityChoices(row.field).size()) {
            converted = index;
        }
        break;
    }
    case E::Number: {
        bool ok = false;
        const double number = value.toDouble(&ok);
        if (ok && std::isfinite(number) && (std::isnan(row.min) || number >= row.min) &&
            (std::isnan(row.max) || number <= row.max)) {
            converted = number;
        }
        break;
    }
    case E::Object: {
        bool ok = false;
        const auto id = static_cast<core::ObjectId>(value.toULongLong(&ok));
        if (!ok) {
            break;
        }
        if (id == 0) {
            converted = id; // aucun objet
            break;
        }
        const core::PageObject* target = find(page, id);
        const bool self = std::find(ids.begin(), ids.end(), id) != ids.end();
        const bool track = row.field != F::Track ||
                           (target != nullptr && (core::kindOf(*target) == core::ObjectKind::Stroke ||
                                                  core::kindOf(*target) == core::ObjectKind::Line));
        if (target != nullptr && !self && track) {
            converted = id;
        }
        break;
    }
    }
    if (!converted) {
        return nullptr;
    }
    return std::make_unique<SetInteractivityCommand>(pageIndex, std::move(ids), row.field, *converted);
}

} // namespace actcraft::ui
