#pragma once

#include <actcraft/core/model.h>
#include <actcraft/core/undo.h>

#include <cstddef>

namespace actcraft::core {

// Cache du contenu fixe de la page affichée (DA11). Seule voie des outils et du document vers le cache : le
// lot 10 ne change que son implémentation (lot 2 : redessin complet, FixedPageItem + ShaderEffectSource de la
// vue).
class PageCache
{
public:
    virtual ~PageCache() = default;
    // L'objet `id` est déjà dans la page affichée du document.
    virtual void insert(ObjectId id) = 0;
    // L'objet `id` n'est plus dans la page affichée.
    virtual void remove(ObjectId id) = 0;
    // Zone à redessiner en coordonnées de page ; rectangle vide = toute la page.
    virtual void invalidate(const RectF& pageRect) = 0;
};

// Envoie au cache les changements de la page `shownPage`, dans l'ordre. Les ajouts et retraits de pages ne le
// concernent pas : le contrôleur change alors de page affichée.
void applyToCache(const Changes& changes, std::size_t shownPage, PageCache& cache);

} // namespace actcraft::core
