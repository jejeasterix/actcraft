#pragma once

#include <actcraft/core/model.h>
#include <actcraft/core/undo.h>

#include <cstddef>
#include <utility>
#include <vector>

namespace actcraft::core {

// Mêmes actions, champ par champ et dans le même ordre (Action n'a pas d'opérateur ==).
bool sameActions(const std::vector<Action>& a, const std::vector<Action>& b) noexcept;

// Remplace la liste des actions de chaque objet (liste vide : retire tout) ; une étape pour tous les objets
// (L8-T11, F-850, F-851). Identifiants absents de la page, objets qui portent déjà la liste : ignorés. Une
// action ne change aucun pixel : un PageSettingsChanged par objet modifié, que applyToCache ignore (L0-S3),
// comme le nom et les mots-clés (propertyedits.h, D6-6). Jamais ObjectChanged : le cache redessinerait
// l'objet pour rien.
class SetObjectActionsCommand final : public UndoCommand
{
public:
    SetObjectActionsCommand(std::size_t page, std::vector<ObjectId> ids, std::vector<Action> actions);
    void redo(Document& document, Changes& changes) override;
    void undo(Document& document, Changes& changes) override;

private:
    std::size_t m_page;
    std::vector<ObjectId> m_ids;
    std::vector<Action> m_actions;
    // Par objet trouvé au dernier redo, sa liste d'avant ; undo ne rétablit qu'elle.
    std::vector<std::pair<ObjectId, std::vector<Action>>> m_before;
};

} // namespace actcraft::core
