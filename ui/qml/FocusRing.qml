import QtQuick
import ActCraft.Ui

// Anneau de focus clavier que le style n'entoure pas lui-même (L11-T24) : il remplit son parent ; l'appelant lui
// donne la condition de visibilité (`visible: parent.visualFocus`), et au besoin la couleur ou le rayon.
Rectangle {
    anchors.fill: parent
    color: "transparent"
    border.width: 2
    border.color: Theme.surfaceActive
}
