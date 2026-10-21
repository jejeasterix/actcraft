#pragma once

#include <actcraft/core/model.h>
#include <actcraft/core/undo.h>

#include <vector>

namespace actcraft::core {

// Soulever (DA11, lot 3) : objets retirés de la page affichée le temps d'un geste, hors pile.
struct LiftedObject
{
    std::size_t index = 0; // rang d'origine dans page.objects
    PageObject object;
};

// Retire les objets `ids` présents (inconnus ignorés) ; rend leur liste dans l'ordre de la page (rangs
// croissants).
std::vector<LiftedObject> takeObjects(Page& page, const std::vector<ObjectId>& ids);
// Remet les objets à leurs rangs d'origine : la page redevient celle d'avant takeObjects.
void restoreObjects(Page& page, const std::vector<LiftedObject>& lifted);
// Pour le cache, au soulèvement : un ObjectRemoved par objet, zone = son emprise (boundsOnPage).
Changes liftChanges(std::size_t page, const std::vector<LiftedObject>& lifted);
// Pour le cache, au dépôt, `command` étant les changements de la commande exécutée sur la page restaurée
// (vide : geste annulé) ; le cache n'a plus aucun objet soulevé :
// - chaque objet soulevé encore présent dans `pageAfter` : un ObjectInserted (zone = emprise dans
// `pageAfter`) ;
// - aucun ObjectRemoved ni ObjectChanged d'un objet soulevé ;
// - tout autre changement de `command` passe tel quel, dans son ordre (morceaux de gomme : ObjectInserted).
Changes changesAfterLift(std::size_t page, const std::vector<LiftedObject>& lifted, const Changes& command,
                         const Page& pageAfter);

} // namespace actcraft::core
