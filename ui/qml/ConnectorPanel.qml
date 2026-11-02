import QtQuick
import ActCraft.Ui

// Coquille (L5-T2) : panneau du connecteur (styles, extrémités), visible avec l'outil connecteur. Le corps vient
// de L5-T17.
Item {
    id: root
    visible: false

    required property Commands commands
    required property Tools tools
}
