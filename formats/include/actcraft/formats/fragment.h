#pragma once

#include <actcraft/core/document.h>
#include <actcraft/core/model.h>

#include <QIODevice>
#include <QString>

#include <optional>
#include <vector>

namespace actcraft::formats {

// Ressource de la bibliothèque (D6-14) : un .actcraft d'une seule page, qui ne porte que les objets choisis
// et les seules ressources qu'ils citent (formats::resourcesFor). Jamais le document d'origine élagué : sa
// table de ressources, son titre et ses entrées inconnues n'en sortent pas.
struct Fragment
{
    core::Page page;          // une page qui ne contient que les objets du fragment
    core::Document resources; // document porteur des seules ressources référencées (même type que le natif)
};

// Écrit un .actcraft d'une page avec les objets `objects` (fermés : closureForCopy) de la page `page` (indice
// à partir de 0) de `document`. Une action OnObject dont la cible n'est pas du fragment y vise 0. Faux,
// `error` rempli, si la page n'existe pas, si aucun objet n'est trouvé ou si l'écriture échoue.
bool writeFragment(const core::Document& document, int page, const std::vector<core::ObjectId>& objects,
                   QIODevice& out, QString& error);
// Refuse un fichier de plus d'une page, et un fichier dont le lecteur natif écarte une ressource (absente,
// illisible, d'empreinte fausse) ; un objet de type inconnu reste lu (L2-C8).
std::optional<Fragment> readFragment(QIODevice& in, QString& error);
// Objets prêts à insérer : identifiants neufs pris dans `target`, ressources manquantes copiées dans
// `target`.
std::vector<core::PageObject> prepareInsertion(const Fragment& fragment, core::Document& target);

} // namespace actcraft::formats
