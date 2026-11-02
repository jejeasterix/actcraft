#pragma once

#include <actcraft/core/model.h>
#include <actcraft/core/undo.h>

#include <cstddef>

namespace actcraft::core {

// Briques d'édition d'une page ; Page::objects reste rangé par couche (F-510, F-512).

// Insère `object` au sommet de sa couche ; retourne son indice dans page.objects.
std::size_t insertAtTopOfLayer(Page& page, PageObject object);
// Indice de l'objet `id`, ou page.objects.size() s'il n'y est pas.
std::size_t indexOf(const Page& page, ObjectId id) noexcept;
// Profondeur (F-512, lecture seule) : rang dans sa couche, 0 = base ; page.objects.size() si absent.
std::size_t depthInLayer(const Page& page, ObjectId id) noexcept;
// Vrai si les objets sont rangés par couche croissante.
bool isLayerOrdered(const Page& page) noexcept;
// Vrai si l'objet a une emprise : sa géométrie donne au moins un point. Un groupe, un objet inconnu sans
// boîte (UnknownObject::hasBounds faux), un trait sans point n'en ont pas.
bool hasExtent(const PageObject& object) noexcept;
// Rectangle englobant en coordonnées de page, épaisseur comprise (transformation appliquée) ; 0 × 0 à
// l'origine pour un objet sans emprise (hasExtent faux), ce qui n'est pas une position.
RectF boundsOnPage(const PageObject& object) noexcept;

// Ajoute un objet au sommet de sa couche sur la page `page`. L'identifiant est alloué au premier redo() si
// l'objet n'en a pas, puis gardé. Si l'espace d'identifiants est épuisé (allocateId() rend 0), la commande ne
// fait rien : aucun objet inséré, aucun changement rendu, id() reste 0.
class AddObjectCommand final : public UndoCommand
{
public:
    AddObjectCommand(std::size_t page, PageObject object);
    void redo(Document& document, Changes& changes) override;
    void undo(Document& document, Changes& changes) override;
    ObjectId id() const noexcept; // 0 avant le premier redo(), ou si les identifiants sont épuisés

private:
    std::size_t m_page;
    PageObject m_object;
    bool m_inserted = false;
};

// Ajoute une page vierge `width` × `height` à l'indice `index` (≤ pageCount()).
class AddPageCommand final : public UndoCommand
{
public:
    AddPageCommand(std::size_t index, double width, double height);
    void redo(Document& document, Changes& changes) override;
    void undo(Document& document, Changes& changes) override;

private:
    std::size_t m_index;
    double m_width;
    double m_height;
};

} // namespace actcraft::core
