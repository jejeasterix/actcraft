#pragma once

#include <actcraft/core/document.h>
#include <actcraft/core/model.h>
#include <actcraft/core/undo.h>

#include <cstddef>
#include <memory>
#include <string>

// Moteur des actions d'objet (lot 8, F-583) : une action du catalogue (`actions/data/object-actions.json`,
// L8-T6) devient une commande d'annulation sur sa cible. Le moteur ne touche pas au document : il rend la
// commande, que l'appelant exécute.
namespace actcraft::core {

struct ActionContext
{
    std::size_t page = 0;
    ObjectId source = 0; // objet cliqué, porteur de l'action
    ObjectId target = 0; // cible ; 0 : l'objet cliqué
    double pageWidth = 1024;
    double pageHeight = 768;
};

// Commande d'annulation qui exécute l'action d'objet `id` (catalogue, L8-T6) avec ses paramètres (objet JSON
// compact) ; nullptr si l'identifiant est inconnu, la cible absente, un paramètre manquant ou hors bornes, ou
// si un limiteur de la cible bloque l'action (rotation, dimension, déplacement).
// Une action permise rend une commande même si elle ne change rien (« fixer 30° » sur un objet déjà à 30° :
// angleSetsAbsoluteRotation) ; seuls ajuster et taille d'origine rendent nullptr quand fitTransforms n'a rien
// à faire. Une action agit aussi sur un objet verrouillé (H8-11) ; « fixer l'angle » d'un groupe rend nullptr
// (angle non défini).
// Édition (L8-T8) : la cible n'a pas besoin d'emprise. nullptr aussi pour un pas de translucidité déjà à sa
// borne, une valeur arithmétique sur un texte qui n'est pas un nombre, une action de texte sur un objet qui
// n'est pas un texte, un « nouveau texte » vide ; « fixer la translucidité » borne sa valeur à 0…255. Copier,
// couper et coller ne sont pas des actions du moteur (isEditAction faux, nullptr) : presse-papiers hors de core,
// l'exécuteur (L8-T9) déclenche edit.copy, edit.cut, edit.paste.
std::unique_ptr<UndoCommand> objectActionCommand(const Document& document, const std::string& id,
                                                 const std::string& parameters, const ActionContext& context);
bool isGeometryAction(const std::string& id) noexcept;
bool isEditAction(const std::string& id) noexcept; // L8-T8

} // namespace actcraft::core
