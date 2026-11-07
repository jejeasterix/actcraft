#pragma once
#include <actcraft/core/model.h>

#include <vector>

// Règles de conteneur (D8-6, F-577, F-580) : logique pure, sans modification du document. Un objet n'est
// jamais contenu par un conteneur devant lequel il n'est pas : l'ordre de superposition compte autant que la
// géométrie, et la géométrie est celle du dépôt (la page reçue a déjà les objets à leur position relâchée).
namespace actcraft::core {
enum class DropOutcome
{
    Free,
    Contained,
    BounceBack
};
struct DropResult
{
    DropOutcome outcome = DropOutcome::Free;
    ObjectId container = 0; // Contained : le conteneur gagnant ; BounceBack : le conteneur qui renvoie
    bool chime = false;     // son de confirmation à jouer (L8-T25)
};
// Vrai si `candidate` est devant `container` : couche plus haute, ou même couche et plus haut dans l'ordre.
// Jugé sur les objets dessinés : un groupe, rangé au sommet de sa couche, ne l'est pas à ce rang ; ses
// membres (groupes imbriqués compris) le sont au leur. Faux si l'un des deux n'est pas sur la page ou ne
// dessine rien.
bool isInFront(const Page& page, ObjectId candidate, ObjectId container);
// Règle d'acceptation seule (Accepts) : rien, tout, un objet désigné, ou un mot-clé en commun
// (ObjectCommon::keywords, comparaison exacte).
bool accepts(const PageObject& container, const PageObject& candidate);
// Verdict au relâché ; `moved` est déjà à sa position de dépôt dans `page`. Le conteneur visible le plus haut
// dans la superposition qui accepte et contient l'emporte (imbrication). Refusé par tous et l'un d'eux sous
// le point de dépôt a `bounceBack` : BounceBack. Plusieurs objets déplacés : chacun doit être devant le
// conteneur et accepté par lui ; la règle FitRule porte sur l'union de leurs emprises, le point de dépôt est
// son centre. Un objet déplacé n'est jamais conteneur des autres.
DropResult resolveDrop(const Page& page, const std::vector<ObjectId>& moved);
// Objets dont `container` est le conteneur gagnant (resolveDrop de l'objet seul), dans l'ordre de la page :
// les contenus directs, sans ceux d'un conteneur imbriqué qui l'emporte.
std::vector<ObjectId> containedIn(const Page& page, ObjectId container);
} // namespace actcraft::core
