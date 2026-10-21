import QtQuick
import ActCraft.Ui

// Racine des fonctions du lot 7 : chaque fonction pose ici son contrôleur, sa surcouche et ses gestionnaires de
// commandes (commands.setHandler). Une surcouche qui prend l'entrée se déclare couverture :
// Component.onCompleted: pageArea.view.addInputCover(surcouche)
// Racine QtObject et non Item : la propriété `layer` figée par le socle est une propriété FINAL de Item (grouped
// property de rendu), qu'une classe dérivée ne peut pas redéfinir (« Cannot override FINAL property »).
// Règle pour le lot 7 : une surcouche ou un panneau est un Item déclaré dans `data` (ou ailleurs) avec
// `parent: presentation.layer` (couche plein cadre) ou `parent: presentation.pageOverlays` (coordonnées de page) ;
// jamais comme enfant visuel direct de cette racine, qui n'est pas un Item.
QtObject {
    id: presentation
    default property list<QtObject> data
    property Commands commands: null
    property DocumentController documentController: null
    property Tools tools: null
    property Item pageArea: null
    property Item pageOverlays: null
    property Item layer: null
}
