import QtQuick
import ActCraft.Ui

// Pouce d'une barre de défilement (L1-C5) : sombre cerclé de clair, lisible sur la page blanche (15,57:1) comme sur
// le fond bleu (bord à 4,68:1), au repos comme appuyé. Le pouce suit le doigt : l'appui n'a pas besoin d'une autre
// couleur, qui tombait sous 3:1 sur la page.
Rectangle {
    implicitWidth: 6
    implicitHeight: 6
    radius: 3
    color: Theme.barre
    border.width: 1
    border.color: Theme.texte
}
