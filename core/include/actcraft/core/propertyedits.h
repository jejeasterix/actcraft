#pragma once

#include <actcraft/core/model.h>
#include <actcraft/core/undo.h>

#include <cstdint>
#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace actcraft::core {

inline constexpr int kMergeObjectProperties = 5; // famille de fusion de SetObjectPropertiesCommand
inline constexpr int kMergePageProperties = 6;   // famille de fusion de SetPagePropertiesCommand

// Propriétés modifiables par le navigateur de propriétés (F-570 à F-576, F-581).
enum class ObjectProperty : std::uint8_t
{
    Name,
    Keywords,
    Label,
    LabelStyle,
    Layer,
    Translucency,
    Visible,
    Locked,
    OutlineStyle,
    OutlineColor,
    OutlineWidth,
    FillStyle,
    FillColor,
    Left,
    Top,
    Width,
    Height,
    Angle,
    Mirrored,
};

// Faux pour le nom, les mots-clés, la légende et son style : leur Change est PageSettingsChanged, qui ne
// touche aucun pixel du cache (D6-6, A12).
bool propertyTouchesTheCache(ObjectProperty property) noexcept;

// Amendement 1, point 15 : couche, visibilité, verrou et translucidité se lisent ici mais s'écrivent par
// SetLayerCommand, SetObjectFlagCommand (Hidden, Locked) et SetTranslucencyCommand. Une
// SetObjectPropertiesCommand sur l'une d'elles ne change rien (Changes vide).
bool isDelegated(ObjectProperty property) noexcept;

// Types : texte (nom, légende, page), liste de textes (mots-clés), double (épaisseur, translucidité 0 … 255,
// position, taille, angle), bool (visible, verrouillé, miroir), std::uint32_t (couleur Argb), énumérations.
using PropertyValue = std::variant<std::string, std::vector<std::string>, double, bool, std::uint32_t, Layer,
                                   PenStyle, FillKind, LabelStyle>;

// Une même valeur posée sur un ou plusieurs objets d'une page : une étape d'annulation, chaque ancienne
// valeur gardée par objet. Fusionne avec la suivante de même page, mêmes objets et même propriété (réglette
// glissée) ; UndoStack::closeMerge() clôt l'étape (D10). Identifiants absents et objets auxquels la propriété
// ne s'applique pas : ignorés. Changes : ObjectChanged par objet modifié (zone = emprise avant ∪ après) si la
// propriété touche le cache, PageSettingsChanged par objet modifié sinon.
class SetObjectPropertiesCommand final : public UndoCommand
{
public:
    // `page` : index de page ; `objects` : identifiants ; la valeur est la même pour tous.
    SetObjectPropertiesCommand(int page, std::vector<ObjectId> objects, ObjectProperty property,
                               PropertyValue value);
    void redo(Document& document, Changes& changes) override;
    void undo(Document& document, Changes& changes) override;
    int mergeId() const noexcept override;             // kMergeObjectProperties
    bool mergeWith(const UndoCommand& other) override; // même page, mêmes objets, même propriété

private:
    int m_page;
    std::vector<ObjectId> m_objects;
    ObjectProperty m_property;
    PropertyValue m_value;
    bool m_partlyRefused = false; // le dernier redo a refusé la valeur pour au moins un objet
    // Par objet auquel la propriété s'applique, sa valeur d'avant, ou sa transformation pour position,
    // taille, angle et miroir (comme TransformObjectsCommand) ; rempli à chaque redo. Annuler ne rétablit
    // qu'elle, jamais l'objet entier : un verrou posé hors pile depuis (SetObjectFlagCommand) resterait.
    std::vector<std::pair<ObjectId, std::variant<PropertyValue, Transform>>> m_old;
};

enum class PageProperty : std::uint8_t
{
    Name,
    Description,
    Keywords,
    Width,
    Height,
};

// Une propriété de la page : nom, description, mots-clés (PageSettingsChanged, sans cache) ; largeur, hauteur
// (PageChanged). Fusionne avec la suivante de même page et même propriété.
class SetPagePropertiesCommand final : public UndoCommand
{
public:
    SetPagePropertiesCommand(int page, PageProperty property, PropertyValue value);
    void redo(Document& document, Changes& changes) override;
    void undo(Document& document, Changes& changes) override;
    int mergeId() const noexcept override; // kMergePageProperties
    bool mergeWith(const UndoCommand& other) override;

private:
    int m_page;
    PageProperty m_property;
    PropertyValue m_value;
    PropertyValue m_old; // valeur d'avant, relue à chaque redo
};

// Valeur courante, pour le modèle du navigateur (L6-T12) ; std::nullopt si la propriété ne s'applique pas.
std::optional<PropertyValue> readProperty(const PageObject& object, ObjectProperty property);
// Valeur courante d'une propriété de la page.
PropertyValue readPageProperty(const Page& page, PageProperty property);

} // namespace actcraft::core
