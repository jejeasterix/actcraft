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
std::unique_ptr<UndoCommand> objectActionCommand(const Document& document, const std::string& id,
                                                 const std::string& parameters, const ActionContext& context);
bool isGeometryAction(const std::string& id) noexcept;
bool isEditAction(const std::string& id) noexcept; // L8-T8

} // namespace actcraft::core
