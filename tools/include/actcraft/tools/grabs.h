#pragma once

#include <actcraft/core/model.h>
#include <actcraft/input/routing.h>

#include <map>
#include <optional>

namespace actcraft::tools {

// Registre des objets tenus (F-303, H11-13) : un objet tenu par un utilisateur est inaccessible à l'autre
// jusqu'au relâchement. Un seul registre pour les deux utilisateurs (ui::ToolHost), posé sur leurs sélecteurs
// (SelectTool::setGrabs).
class Grabs
{
public:
    // Faux si `object` est tenu par un autre utilisateur ; vrai s'il était libre (il est alors tenu par
    // `user`) ou déjà tenu par `user`.
    bool hold(core::ObjectId object, input::UserId user);
    void release(input::UserId user); // tout ce que tient `user`, rien d'autre
    std::optional<input::UserId> holder(core::ObjectId object) const; // vide : libre

private:
    std::map<core::ObjectId, input::UserId> m_held;
};

} // namespace actcraft::tools
