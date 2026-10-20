#pragma once

#include <actcraft/core/document.h>
#include <actcraft/core/undo.h>

#include <cstdint>
#include <utility>
#include <vector>

namespace actcraft::core {

inline constexpr int kMergeTranslucency = 3; // famille de fusion de SetTranslucencyCommand

// Déplacer, redimensionner, faire pivoter, agrandir, réduire (F-210, F-522, F-523) : la transformation de
// chaque objet est composée avec `delta` (coordonnées de page, appliquée après la sienne). Changes : un
// ObjectChanged par objet, zone = emprise avant ∪ emprise après. Identifiants absents ignorés.
class TransformObjectsCommand final : public UndoCommand
{
public:
    TransformObjectsCommand(std::size_t page, std::vector<ObjectId> ids, Transform delta);
    void redo(Document& document, Changes& changes) override;
    void undo(Document& document, Changes& changes) override;

private:
    std::size_t m_page;
    std::vector<ObjectId> m_ids;
    Transform m_delta;
    std::vector<std::pair<ObjectId, Transform>> m_before;
};

// Supprimer, corbeille (F-050, F-541). Undo : objets remis à leurs rangs d'origine (même profondeur).
// Changes : ObjectRemoved (zone = emprise) ; à l'annulation, ObjectInserted.
class RemoveObjectsCommand final : public UndoCommand
{
public:
    RemoveObjectsCommand(std::size_t page, std::vector<ObjectId> ids);
    void redo(Document& document, Changes& changes) override;
    void undo(Document& document, Changes& changes) override;

private:
    std::size_t m_page;
    std::vector<ObjectId> m_ids;
    std::vector<std::pair<std::size_t, PageObject>> m_removed; // rang d'origine, objet ; rangs croissants
};

// Gomme (F-230) : core::ReplaceObjectsCommand (replace.h), un groupe par trait touché, morceaux au rang de
// l'original.

enum class ClearKind : std::uint8_t
{
    Annotations = 0, // isAnnotation
    Objects = 1,     // tout objet qui n'est pas une annotation (D8)
    Grid = 2,        // grille remise à Grid{}
    Background = 3,  // fond remis à Background{}
    Page = 4,        // tout
};
// Effacer ▸ (F-130). Changes : un PageChanged (page entière) si quelque chose a changé ; rien sinon.
class ClearPageCommand final : public UndoCommand
{
public:
    ClearPageCommand(std::size_t page, ClearKind kind);
    void redo(Document& document, Changes& changes) override;
    void undo(Document& document, Changes& changes) override;

private:
    std::size_t m_page;
    ClearKind m_kind;
    Page m_before;
};

// Rétablir la page (F-131) : la page devient `saved` (taille, fond, grille, objets), en une étape (D9).
// Changes : un PageChanged (page entière).
class ReplacePageCommand final : public UndoCommand
{
public:
    ReplacePageCommand(std::size_t page, Page saved);
    void redo(Document& document, Changes& changes) override;
    void undo(Document& document, Changes& changes) override;

private:
    std::size_t m_page;
    Page m_other;
};

// Translucidité (F-524) : 0 opaque … 255 invisible. Fusionne avec la précédente de même famille portant sur
// les mêmes objets (réglette glissée : une étape). Changes : ObjectChanged par objet.
class SetTranslucencyCommand final : public UndoCommand
{
public:
    SetTranslucencyCommand(std::size_t page, std::vector<ObjectId> ids, std::uint8_t value);
    void redo(Document& document, Changes& changes) override;
    void undo(Document& document, Changes& changes) override;
    int mergeId() const noexcept override;
    bool mergeWith(const UndoCommand& other) override;

private:
    std::size_t m_page;
    std::vector<ObjectId> m_ids;
    std::uint8_t m_value;
    std::vector<std::pair<ObjectId, std::uint8_t>> m_before;
};

// Centre de l'emprise réunie des objets (pivot de la rotation et des pas de 10 %) ; page sans ces objets :
// (0, 0).
PointF selectionCenter(const Page& page, const std::vector<ObjectId>& ids);
Transform scalingAbout(PointF center, double sx, double sy) noexcept;
Transform rotationAbout(PointF center, double degrees) noexcept;
// Échelle des longueurs de `t` : sqrt(|m11·m22 − m12·m21|). Exacte pour une échelle uniforme (avec rotation
// ou miroir), moyenne géométrique des deux échelles sinon. Sert à l'épaisseur visible d'un trait transformé.
double lengthScale(const Transform& t) noexcept;

} // namespace actcraft::core
