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

    // Tout ce que la barre contient fait la hauteur de la barre, celle d'une cible tactile (Theme.hauteurBarre =
    // Theme.tailleBouton) : le bouton plein écran et le zoom n'en débordent plus (L1-C5).
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
                readonly property bool onAccent: menuButton.down || menu.opened
                objectName: "menu-" + modelData.id
                text: modelData.label
                font.pixelSize: Theme.taillePolice
                Layout.fillHeight: true
                contentItem: Label {
                    text: menuButton.text
                    font: menuButton.font
                    color: menuButton.onAccent ? Theme.texteSurActive : Theme.texte
                    verticalAlignment: Text.AlignVCenter
                }
                background: Rectangle {
                    radius: Theme.rayon
                    color: menuButton.onAccent ? Theme.surfaceActive
                         : menuButton.hovered ? Theme.surfaceSurvol : "transparent"
                    border.width: menuButton.visualFocus ? 2 : 0 // focus clavier (Tab)
                    border.color: Theme.surfaceActive
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

        ComboBox {
            id: zoomBox
            objectName: "zoomBox"
            Layout.preferredWidth: 112
            Layout.fillHeight: true // couleurs : palette de la fenêtre (Main.qml), tirée de Theme
            // Ouverte ou appuyée : accent, texte et flèche sombres, comme un menu de la barre ouvert (L1-C5). Le
            // style Basic peint alors le fond en `mid` et la flèche en `dark` ; la liste hérite de cette palette.
            palette.mid: Theme.surfaceActive
            palette.buttonText: zoomBox.down ? Theme.texteSurActive : Theme.texte
            palette.dark: zoomBox.down ? Theme.texteSurActive : Theme.contour
            delegate: ItemDelegate {
                id: zoomEntry
                required property var modelData
                required property int index
                width: ListView.view.width
                implicitHeight: Theme.tailleBouton
                text: modelData
                font.pixelSize: Theme.taillePolice
                font.weight: zoomBox.currentIndex === index ? Font.DemiBold : Font.Normal
                highlighted: zoomBox.highlightedIndex === index
                hoverEnabled: zoomBox.hoverEnabled
                contentItem: Label {
                    text: zoomEntry.text
                    font: zoomEntry.font
                    color: Theme.texte
                    verticalAlignment: Text.AlignVCenter
                }
                background: ItemHighlight {
                    control: zoomEntry
                }
            }
            readonly property var percents: [0, 25, 50, 100, 150, 200, 400, 800]
            model: [qsTr("Ajuster"), "25 %", "50 %", "100 %", "150 %", "200 %", "400 %", "800 %"]
            displayText: Math.round(bar.view.zoom * 100) + " %"
            onActivated: index => {
                if (index === 0)
                    bar.commands.trigger("view.fit")
                else
                    bar.commands.trigger("view.zoom", zoomBox.percents[index])
            }
            Connections {
                target: zoomBox.popup
                function onOpened() { bar.popupOpened() }
                function onClosed() { bar.popupClosed() }
            }
        }

        CommandButton {
            commands: bar.commands
            commandId: "view.fullscreen"
        }
    }
}
