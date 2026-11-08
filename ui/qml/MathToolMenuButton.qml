import QtQuick

// Bouton « ≡ » d'un outil de maths : ouvre son menu. Position posée par l'outil.
Rectangle {
    id: button
    property QtObject menu: null
    width: 18
    height: 18
    radius: 4
    color: "#f4f7fa"
    border.color: "#6d7c8b"
    Text { anchors.centerIn: parent; text: "≡"; font.pixelSize: 12; color: "#1f2a35" }
    TapHandler { onTapped: if (button.menu) button.menu.popup() }
}
