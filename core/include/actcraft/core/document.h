#pragma once

#include <actcraft/core/model.h>

#include <cstddef>
#include <cstdint>
#include <vector>

namespace actcraft::core {

// Squelette du document (DA2, décision 15 du cadrage livraison-complete) : une suite de pages, jamais vide.
// Couches, ressources et pile d'annulation arrivent au lot 2.
class Document
{
public:
    Document(); // une page vierge de 1024 × 768

    std::size_t pageCount() const noexcept;
    // Exige index < pageCount() (assert).
    const Page& page(std::size_t index) const;
    Page& page(std::size_t index);
    // Ajoute à la fin une page vierge de la taille de la dernière ; retourne son indice.
    std::size_t appendPage();

private:
    std::vector<Page> m_pages;
};

// « Page suivante » (F-125) : indice suivant ; sur la dernière page, ajoute d'abord une page vierge.
std::size_t nextPageIndex(Document& document, std::size_t current);
// « Page précédente » : indice précédent, borné à 0.
std::size_t previousPageIndex(std::size_t current) noexcept;

// Empreinte FNV-1a 64 bits : nombre de pages, puis fingerprint(page) de chaque page dans l'ordre.
std::uint64_t fingerprint(const Document& document) noexcept;

} // namespace actcraft::core
