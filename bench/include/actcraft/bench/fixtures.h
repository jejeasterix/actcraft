#pragma once

#include <QString>
#include <QStringList>

namespace actcraft::bench {

// Documents de scénario (DA8), écrits avant d'être ouverts : aucun binaire commité. « trois-pages » (S5) :
// page 1 vide, page 2 rectangle bleu et texte « Bonjour », page 3 image rouge et blanche. Faux et `error` si
// le nom est inconnu ou l'écriture impossible.
bool buildFixture(const QString& name, const QString& path, QString& error);
// Noms connus de buildFixture (le joueur refuse les autres dès l'analyse du scénario).
QStringList fixtureNames();

} // namespace actcraft::bench
