import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import ActCraft.Ui

// Palette d'outils principale, ancrée à droite (F-120, F-121) : groupes lus dans la disposition ; « couleurs » et
// « epaisseur » sont dessinés ici, les autres groupes sont des boutons de commande sur deux colonnes.
Rectangle {
    id: paletteRoot
    objectName: "toolPalette"
    required property Commands commands
    required property PageView view
    implicitWidth: Theme.largeurPalette
    color: Theme.palette

    Flickable {
        anchors.fill: parent
        contentHeight: column.implicitHeight + 2 * Theme.espace2
        clip: true
        boundsBehavior: Flickable.StopAtBounds

        ColumnLayout {
            id: column
            x: Theme.espace2
            y: Theme.espace2
            width: paletteRoot.width - 2 * Theme.espace2
            spacing: Theme.espace2

            Repeater {
                model: paletteRoot.commands.palette
                delegate: ColumnLayout {
                    id: groupItem
                    required property var modelData
                    Layout.fillWidth: true
                    spacing: Theme.espace2

                    Loader {
                        Layout.fillWidth: true
                        sourceComponent: groupItem.modelData.group === "couleurs" ? swatchesComponent
                                       : groupItem.modelData.group === "epaisseur" ? widthComponent
                                       : buttonsComponent
                    }
                    Rectangle {
                        Layout.fillWidth: true
                        implicitHeight: 1
                        color: Theme.separateur
                    }

                    Component {
                        id: swatchesComponent
                        ColorSwatches {
                            commands: paletteRoot.commands
                            view: paletteRoot.view
                        }
                    }
                    Component {
                        id: widthComponent
                        WidthPicker {
                            commands: paletteRoot.commands
                            view: paletteRoot.view
                        }
                    }
                    Component {
                        id: buttonsComponent
                        Grid {
                            columns: 2
                            spacing: Theme.espace1
                            Repeater {
                                model: groupItem.modelData.items
                                delegate: CommandButton {
                                    required property var modelData
                                    commands: paletteRoot.commands
                                    commandId: modelData.id
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}
