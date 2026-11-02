#pragma once
// En-tête interne de formats/ (non installé) : le code d'objet du format natif (nativejson.cpp), partagé par
// l'enregistrement et le presse-papiers d'objets (clipboard.cpp, L5-T3) pour qu'ils ne divergent jamais.
// Aucun champ d'objet n'est écrit ni lu ailleurs.
#include <actcraft/core/model.h>

#include <QJsonArray>
#include <QJsonObject>
#include <QStringList>

#include <vector>

namespace actcraft::formats::native {

// Un objet tel que document.json l'écrit : clés communes et propres à son type, clés inconnues fusionnées
// (unknownKeys) ; un objet d'un type inconnu, son JSON gardé avec l'identifiant courant.
QJsonObject objectJson(const core::PageObject& object);

// Objets d'un tableau JSON, lus comme ceux d'une page de document.json (mêmes gabarits de clés connues) : un
// type inconnu est gardé tel quel, les clés inconnues aussi ; un élément qui n'est pas un objet est sauté.
// Les valeurs mal formées prennent le défaut du modèle, avec un avertissement ajouté à `warnings`.
std::vector<core::PageObject> readObjects(const QJsonArray& list, QStringList& warnings);

} // namespace actcraft::formats::native
