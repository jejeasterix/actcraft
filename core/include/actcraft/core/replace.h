#pragma once

#include <actcraft/core/model.h>
#include <actcraft/core/undo.h>

#include <cstddef>
#include <utility>
#include <vector>

namespace actcraft::core {

// Un groupe : objets retirés et objets qui les remplacent (gomme : un trait → ses morceaux ; reconnaissance :
// des traits → une forme ou un texte).
struct ObjectReplacement
{
    std::vector<ObjectId> removed;
    std::vector<PageObject> added;
};

// Remplacement annulable en une étape (socle des lots 3 et 7, seule commande de remplacement).
// Placement, groupe après groupe sur la page courante : soit r le rang (dans page.objects, avant le retrait
// du groupe) du plus haut de ses objets retirés présents, et k le nombre de ses retirés présents situés sous
// lui ; les ajouts du groupe sont insérés, dans l'ordre, à partir du rang r - k (exemple : a, s1, b, s2, c
// avec {s1, s2} → {f} donne a, b, f, c) quand ils sont de la même couche que ce plus haut retiré ; sinon
// (autre couche, ou aucun retrait présent) ils vont au sommet de leur couche (insertAtTopOfLayer).
// Identifiants des ajouts tirés au premier redo() (allocateId) puis gardés ; l'identifiant porté par un ajout
// est ignoré. Identifiant retiré absent de la page : ignoré. Changes au redo, groupe par groupe dans l'ordre
// : un ObjectRemoved par objet retiré présent (zone : son emprise, boundsOnPage), puis un ObjectInserted par
// ajout (zone : son emprise). À l'annulation, l'inverse exact : groupes en ordre inverse ; pour chacun, un
// ObjectRemoved par ajout (ordre inverse), puis un ObjectInserted par objet retiré, remis à son rang
// d'origine (rangs croissants). Le cache (DA11) traite une insertion hors fin de page par un redessin
// immédiat (règle du lot 10) : rien de plus à faire ici.
class ReplaceObjectsCommand final : public UndoCommand
{
public:
    ReplaceObjectsCommand(std::size_t page, std::vector<ObjectReplacement> replacements);
    ReplaceObjectsCommand(std::size_t page, std::vector<ObjectId> removed,
                          std::vector<PageObject> added); // un groupe
    void redo(Document& document, Changes& changes) override;
    void undo(Document& document, Changes& changes) override;
    const std::vector<ObjectId>&
    addedIds() const noexcept; // groupes puis ajouts, dans l'ordre ; vide avant redo

private:
    struct Applied
    {
        std::vector<std::pair<std::size_t, PageObject>> removed; // rang d'origine, objet ; rangs croissants
        std::vector<ObjectId> added;
    };
    std::size_t m_page;
    std::vector<ObjectReplacement> m_groups;
    std::vector<Applied> m_applied;
    std::vector<ObjectId> m_addedIds;
};

// Réglages de présentation de la page (F-591 à F-593, F-640, F-284), en une étape. Changes : un seul
// PageSettingsChanged (page `page`), que applyToCache ignore (aucun redessin, I3).
class SetPagePresentationCommand final : public UndoCommand
{
public:
    SetPagePresentationCommand(std::size_t page, PagePresentation value);
    void redo(Document& document, Changes& changes) override;
    void undo(Document& document, Changes& changes) override;

private:
    std::size_t m_page;
    PagePresentation m_value;
    PagePresentation m_previous;
};

} // namespace actcraft::core
