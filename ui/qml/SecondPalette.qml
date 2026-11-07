import QtQuick
import ActCraft.Ui

// L11-T9 (F-303, N2-A7, H11-12) : deuxième palette, celle du second utilisateur, au bord gauche ; montrée tant
// que le mode à deux utilisateurs est allumé. Contenu fixe, non personnalisable : stylo, surligneur, gomme,
// sélecteur ; rouge, bleu, vert, noir ; épaisseurs 2, 4, 8. Chaque bouton passe par
// Commands.trigger(id, argument, 1), le chemin des boutons du premier utilisateur avec l'utilisateur en plus :
// verrou de classe, diffusion protégée et refus des boîtes s'y appliquent ; aucune autre voie vers les outils.
Rectangle {
    id: palette
    objectName: "secondPalette"
    required property Commands commands
    readonly property int secondUser: 1
    readonly property var toolIds: ["pen", "highlighter", "eraser", "select"]
    readonly property var colorValues: ["#E53935", "#1E88E5", "#43A047", "#000000"]
    readonly property var widthValues: [2, 4, 8]
    implicitWidth: Theme.largeurPalette
    color: "#F4F4F4"

    component PaletteButton: Rectangle {
        id: button
        property string commandId
        property var argument: undefined
        // Un composant en ligne ne voit pas les id du document : commandes et utilisateur lui sont passés.
        property Commands commands: null
        property int user: 0
        // Même chemin qu'un tapé : appelée aussi par l'accessibilité et les tests.
        function press() {
            return button.commands ? button.commands.trigger(button.commandId, button.argument, button.user) : false
        }
        width: 44
        height: 44
        radius: 6
        color: "#FFFFFF"
        border.width: 1
        border.color: "#9E9E9E"
        Accessible.role: Accessible.Button
        Accessible.onPressAction: button.press()
        TapHandler { onTapped: button.press() }
    }

    Column {
        anchors.top: parent.top
        anchors.topMargin: 12
        anchors.horizontalCenter: parent.horizontalCenter
        spacing: 12

        Grid {
            columns: 2
            spacing: 4
            Repeater {
                model: palette.toolIds
                PaletteButton {
                    required property string modelData
                    objectName: "secondTool_" + modelData
                    commands: palette.commands
                    user: palette.secondUser
                    commandId: "tool." + modelData
                    Accessible.name: palette.commands.label(commandId)
                    Image {
                        anchors.centerIn: parent
                        width: 24
                        height: 24
                        sourceSize: Qt.size(24, 24)
                        source: palette.commands.iconUrl(parent.commandId)
                    }
                }
            }
        }

        Grid {
            columns: 2
            spacing: 4
            Repeater {
                model: palette.colorValues
                PaletteButton {
                    required property string modelData
                    required property int index
                    objectName: "secondColor_" + index
                    commands: palette.commands
                    user: palette.secondUser
                    commandId: "pen.color"
                    argument: modelData
                    color: modelData
                    Accessible.name: qsTr("Couleur %1").arg(modelData)
                }
            }
        }

        Column {
            spacing: 4
            Repeater {
                model: palette.widthValues
                PaletteButton {
                    required property int modelData
                    required property int index
                    objectName: "secondWidth_" + index
                    commands: palette.commands
                    user: palette.secondUser
                    commandId: "pen.width"
                    argument: modelData
                    width: 92
                    Accessible.name: qsTr("Épaisseur %1").arg(modelData)
                    Rectangle {
                        anchors.centerIn: parent
                        width: parent.width - 16
                        height: parent.modelData
                        radius: height / 2
                        color: "#212121"
                    }
                }
            }
        }
    }
}
