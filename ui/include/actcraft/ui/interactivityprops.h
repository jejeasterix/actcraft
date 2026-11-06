#pragma once

#include <actcraft/core/model.h>
#include <actcraft/core/undo.h>
#include <actcraft/ui/propertymodel.h>

#include <QString>
#include <QStringList>
#include <QVariant>

#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <variant>
#include <vector>

// Sections de propriétés de l'interactivité (L8-T17, F-577 à F-579, F-582) : « Actions », « Conteneur »,
// « Rotation », « Limiteurs », « Multimédia », ajoutées par PropertyModel après celles du lot 6. Aucun champ
// de l'interactivité ne change un pixel du cache (« Multimédia » : rien à l'écran au sous-lot 8a) : toute
// édition rend un core::Change::Kind::PageSettingsChanged par objet modifié, jamais ObjectChanged, comme le
// nom et les mots-clés (propertyedits.cpp, changeFor) et les actions (SetObjectActionsCommand).
namespace actcraft::ui {

// Rôle « enabled » de PropertyModel : faux quand l'éditeur est grisé (chemin d'un objet qui ne suit pas de
// chemin, mots d'un conteneur qui n'accepte pas des mots…) ; une ligne grisée refuse setValue. Les lignes du
// lot 6 ne sont jamais grisées.
inline constexpr int kEnabledRole = PropertyModel::MaxRole + 1;

// Sections du modèle dans l'ordre de leurs lignes (clés de SectionRole, chacune une fois).
QStringList sections(const PropertyModel& model);

// Champ de l'interactivité porté par une ligne.
enum class InteractivityField : std::uint8_t
{
    Actions,         // liste en lecture ; on la modifie dans le navigateur d'actions (L8-T11)
    Accepts,         // ContainerRule::accepts
    ContainedObject, // ContainerRule::object
    // « Contenir des mots » : les mots-clés du lot 6 du conteneur (ObjectCommon::keywords), jamais la copie
    // ContainerRule::keywords. Écrite par le chemin des mots-clés du lot 6 (SetObjectPropertiesCommand,
    // core::ObjectProperty::Keywords), pas par SetInteractivityCommand.
    ContainWords,
    Fit,
    Tolerance,
    BounceBack,
    Chime,
    Turning,
    TurnStep,
    Pivot,
    PivotObject,
    Travel,
    Track,
    BlocksOthers,
    Magnet,
    MagnetMark,
    PlayOnOpen, // champs de core::Media : objet média seulement
    Loop,
    ControlsHidden,
    AspectLocked,
    ClearBackground,
    WrapText,
};

enum class InteractivityEditor : std::uint8_t
{
    Actions,  // « actions » : liste en lecture
    Choice,   // « choice » : indice = valeur sous-jacente de l'énumération
    Bool,     // « bool »
    Number,   // « number », borné par min et max
    Object,   // « object » : identifiant d'un autre objet de la page, 0 pour aucun
    Keywords, // « keywords » : comme la ligne des mots-clés du lot 6
};

struct InteractivityRow
{
    const char* section; // actions, container, rotation, restrictors, multimedia
    const char* key;
    const char* label; // libellé à nous, traduit par PropertyModel::tr
    InteractivityEditor editor;
    InteractivityField field;
    double min; // number seulement ; NaN sinon
    double max;
};

// Lignes dans l'ordre d'affichage, section par section.
std::span<const InteractivityRow> interactivityRows();
// Libellé d'une section de l'interactivité ; vide pour une autre clé.
QString interactivitySectionLabel(const QString& section);
QString interactivityEditorKey(InteractivityEditor editor);
// Libellés d'un champ « choice » ; vide pour un autre champ.
QStringList interactivityChoices(InteractivityField field);
// « Multimédia » : un objet média seulement ; les autres sections : tout objet.
bool appliesTo(const InteractivityRow& row, const core::PageObject& object);
// Valeur montrée, lue sur la forme normalisée des restrictions (core::normalize : les booléens du lot 2
// priment, un objet bloqué au lot 2 se lit « non »).
QVariant readInteractivity(const core::PageObject& object, InteractivityField field);
// Faux si l'éditeur du champ est grisé pour cet objet.
bool isEnabled(const core::PageObject& object, InteractivityField field);

using InteractivityValue = std::variant<bool, int, double, core::ObjectId>;

// Une étape pour tous les objets (L8-T17). Les modes passent par core::setTravel, core::setTurning puis
// core::normalize : choisir « verticalement » rend `movable` vrai, « non » le rend faux. Identifiants absents
// de la page, objets déjà à cette valeur, champ média sur un objet qui n'en est pas un : ignorés. Un
// PageSettingsChanged par objet modifié, que applyToCache ignore (L0-S3).
class SetInteractivityCommand final : public core::UndoCommand
{
public:
    SetInteractivityCommand(std::size_t page, std::vector<core::ObjectId> ids, InteractivityField field,
                            InteractivityValue value);
    void redo(core::Document& document, core::Changes& changes) override;
    void undo(core::Document& document, core::Changes& changes) override;

    struct MediaOptions
    {
        bool playOnOpen = false;
        bool loop = false;
        bool controlsHidden = false;
        bool aspectLocked = false;
        bool clearBackground = false;
        bool wrapText = false;
        bool operator==(const MediaOptions&) const = default;
    };

private:
    struct Before
    {
        core::ObjectId id = 0;
        core::Restrictions restrictions;
        core::ContainerRule container;
        MediaOptions media;
    };

    std::size_t m_page;
    std::vector<core::ObjectId> m_ids;
    InteractivityField m_field;
    InteractivityValue m_value;
    std::vector<Before> m_before; // objets modifiés au dernier redo ; undo ne rétablit qu'eux
};

// Commande d'une saisie sur `ids` de `page` (indice `pageIndex`) ; nullptr si refusée : lecture seule
// (Actions), ContainWords (chemin du lot 6), valeur illisible, choix inconnu, nombre hors de [min, max]
// (refusé, jamais écrêté), objet visé absent de la page ou parmi `ids`, chemin qui n'est ni un trait ni une
// ligne.
std::unique_ptr<core::UndoCommand> interactivityCommand(const core::Page& page, std::size_t pageIndex,
                                                        std::vector<core::ObjectId> ids,
                                                        const InteractivityRow& row, const QVariant& value);

} // namespace actcraft::ui
