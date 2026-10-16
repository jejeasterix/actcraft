#pragma once

#include <actcraft/core/model.h>
#include <actcraft/core/undo.h>

#include <cstddef>
#include <optional>

namespace actcraft::core {

// Zone à redessiner en coordonnées de page ; std::nullopt = toute la page (fond, grille…). Un rectangle vide
// n'est PAS toute la page : c'est une zone vide.
using CacheArea = std::optional<RectF>;

// Cache du contenu fixe de la page affichée (DA11). Seule voie des outils et du document vers le cache : le
// lot 10 ne change que son implémentation (lot 2 : redessin complet, FixedPageItem + ShaderEffectSource de la
// vue). Chaque appel porte l'identifiant de l'objet ET sa zone, pour que le cache incrémental vise le nœud.
class PageCache
{
public:
    virtual ~PageCache() = default;
    // L'objet `id` est déjà dans la page affichée du document (son emprise s'y lit).
    virtual void insert(ObjectId id) = 0;
    // L'objet `id` n'est plus dans la page affichée : `area` est son emprise d'avant le retrait, qu'on ne
    // peut plus relire dans le document (vide : l'objet ne dessinait rien).
    virtual void remove(ObjectId id, const RectF& area) = 0;
    // L'objet `id` a changé dans `area` (anciennes et nouvelles emprises). `id` 0 et `area` std::nullopt : la
    // page entière a changé.
    virtual void invalidate(ObjectId id, const CacheArea& area) = 0;
};

// Envoie au cache, dans l'ordre, les changements de la page affichée `shownPage`, et rend l'indice de cette
// page après les changements. Un ajout ou un retrait de page placé avant elle la déplace : la suite du lot
// est filtrée sur le nouvel indice. Si elle est retirée, rend std::nullopt et n'envoie plus rien : le
// contrôleur rattache alors la page fixe (FixedPageItem::setSource) ; d'ici là, elle ne lit jamais hors des
// bornes du document. Un ObjectChanged à zone vide est lu comme « toute la page » (contrat de Change,
// undo.h).
std::optional<std::size_t> applyToCache(const Changes& changes, std::size_t shownPage, PageCache& cache);

} // namespace actcraft::core
