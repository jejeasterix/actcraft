#pragma once

#include <QString>

namespace actcraft::bench {

// Racines « Mes ressources » et ressources partagées d'essai, sous `<outputDir>` (`mes-ressources`,
// `ressources-partagees`), vidées puis posées par ResourceLibrary::setRootPathsForTests. Le mode test de
// QStandardPaths ne déplace pas DocumentsLocation : sans cet appel, un lancement viderait le vrai
// Documents/ActCraft/Ressources du poste. Seuls ces deux dossiers d'essai sont supprimés.
void useIsolatedResourceRoots(const QString& outputDir);

} // namespace actcraft::bench
