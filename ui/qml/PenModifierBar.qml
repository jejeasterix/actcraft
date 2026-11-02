import QtQuick
import ActCraft.Ui

// Coquille (L5-T2) : barre des modifications du stylo, à côté de la palette (visible avec view.pen-modifiers).
// Le corps vient de L5-T17.
Item {
    id: root
    visible: false

    required property Commands commands
    required property Tools tools
}
