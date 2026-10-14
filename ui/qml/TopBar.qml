import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import ActCraft.Ui

// Ligne supérieure (F-002) : menus à gauche ; indicateur « n / N », échelle (F-021) et plein écran à droite. Pas
// de barre d'état ailleurs (F-003).
Rectangle {
    id: bar
    objectName: "topBar"
    required property Commands commands
    required property DocumentController documentController
    required property PageView view
    signal popupOpened()
    signal popupClosed()

    implicitHeight: Theme.hauteurBarre
    color: Theme.barre

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: Theme.espace2
        anchors.rightMargin: Theme.espace2
        spacing: Theme.espace1

        Repeater {
            model: bar.commands.menus
            delegate: ToolButton {
                id: menuButton
                required property var modelData
                objectName: "menu-" + modelData.id
                text: modelData.label
                font.pixelSize: Theme.taillePolice
                Layout.fillHeight: true
                contentItem: Label {
                    text: menuButton.text
                    font: menuButton.font
                    color: Theme.texte
                    verticalAlignment: Text.AlignVCenter
                }
                background: Rectangle {
                    radius: Theme.rayon
                    color: menuButton.down || menu.opened ? Theme.surfaceActive
                         : menuButton.hovered ? Theme.surfaceSurvol : "transparent"
                }
                onClicked: menu.popup(menuButton, 0, menuButton.height)

                CommandMenu {
                    id: menu
                    commands: bar.commands
                    spec: menuButton.modelData
                    onOpened: bar.popupOpened()
                    onClosed: bar.popupClosed()
                }
            }
        }

        Item {
            Layout.fillWidth: true
        }

        Label {
            id: pageIndicator
            objectName: "pageIndicator"
            text: bar.documentController.pageLabel
            color: Theme.texte
            font.pixelSize: Theme.taillePolice
            Layout.rightMargin: Theme.espace3
        }

        CommandButton {
            commands: bar.commands
            commandId: "view.fullscreen"
        }
    }
}
