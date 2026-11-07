// Talon L11 : rempli par L11-T8.
#pragma once

#include <actcraft/core/model.h>
#include <actcraft/input/routing.h>

#include <map>
#include <optional>

namespace actcraft::tools {

// Registre des objets tenus : un objet tenu par un utilisateur est inaccessible à l'autre.
class Grabs
{
public:
    bool hold(core::ObjectId object, input::UserId user);
    void release(input::UserId user);
    std::optional<input::UserId> holder(core::ObjectId object) const;

private:
    std::map<core::ObjectId, input::UserId> m_held;
};

} // namespace actcraft::tools
