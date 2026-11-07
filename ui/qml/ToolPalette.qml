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
    required property Tools tools
    property DocumentController documentController: null
    property var profile: null
    property var preferences: null
    // Posé par PaletteFrame : palette ancrée en haut ou en bas, groupes côte à côte.
    property bool horizontal: false
    readonly property real contentHeight: column.implicitHeight + 2 * Theme.espace2
    // Groupes de la disposition, puis le groupe du profil (commandes épinglées, F-033) filtré par disponibilité.
    readonly property var groups: {
        const list = paletteRoot.commands.palette.slice()
        const pins = paletteRoot.profile ? paletteRoot.profile.pinned.filter(id => paletteRoot.commands.isAvailable(id)) : []
        if (pins.length > 0)
            list.push({ group: "profil", items: pins })
        return list
    }
    implicitWidth: Theme.largeurPalette
    color: Theme.palette

    Flickable {
        id: flick
        objectName: "paletteFlick"
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        // Hauteur réservée en bas pour la corbeille quand elle est affichée.
        anchors.bottomMargin: trash.visible ? trash.height : 0.0
        contentHeight: paletteRoot.horizontal ? height : paletteRoot.contentHeight
        contentWidth: paletteRoot.horizontal ? column.implicitWidth + 2 * Theme.espace2 : width
        flickableDirection: paletteRoot.horizontal ? Flickable.HorizontalFlick : Flickable.VerticalFlick
        clip: true
        boundsBehavior: Flickable.StopAtBounds
        // Poignée toujours visible quand le contenu déborde (AsNeeded la laisserait transparente hors survol, style
        // Basic) ; dans la marge droite de la palette, sans recouvrir de bouton.
        ScrollBar.vertical: ScrollBar {
            objectName: "paletteScrollBar"
            width: Theme.espace2
            policy: !paletteRoot.horizontal && flick.contentHeight > flick.height ? ScrollBar.AlwaysOn : ScrollBar.AlwaysOff
        }

        GridLayout {
            id: column
            objectName: "paletteRows"
            x: Theme.espace2
            y: Theme.espace2
            width: paletteRoot.horizontal ? implicitWidth : paletteRoot.width - 2 * Theme.espace2
            columns: paletteRoot.horizontal ? 64 : 1
            rowSpacing: Theme.espace2
            columnSpacing: Theme.espace3
            // Réduite : la première rangée seulement (F-032).
            readonly property int shownRows: paletteRoot.profile && paletteRoot.profile.paletteCollapsed
                                             ? Math.min(1, groupRepeater.count) : groupRepeater.count

            Repeater {
                id: groupRepeater
                model: paletteRoot.groups
                delegate: ColumnLayout {
                    id: groupItem
                    required property var modelData
                    required property int index
                    visible: groupItem.index < column.shownRows
                    Layout.fillWidth: !paletteRoot.horizontal
                    Layout.alignment: Qt.AlignTop
                    spacing: Theme.espace2

                    Loader {
                        Layout.fillWidth: true
                        sourceComponent: groupItem.modelData.group === "couleurs" ? swatchesComponent
                                       : groupItem.modelData.group === "epaisseur" ? widthComponent
                                       : buttonsComponent
                    }
                    Rectangle {
                        visible: !paletteRoot.horizontal
                        Layout.fillWidth: true
                        implicitHeight: 1
                        color: Theme.separateur
                    }

                    Component {
                        id: swatchesComponent
                        ColorSwatches {
                            commands: paletteRoot.commands
                            view: paletteRoot.view
                            profile: paletteRoot.profile
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
                            columns: paletteRoot.horizontal ? 64 : 2
                            spacing: Theme.espaceCible
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

    // Lot 6 : barre de raccourcis, troisième colonne (masquée jusqu'à L6-T9).
    ShortcutBar {
        objectName: "shortcutBar"
        visible: !(paletteRoot.profile && (paletteRoot.profile.shortcutsHidden || paletteRoot.profile.paletteCollapsed))
        commands: paletteRoot.commands
        documentController: paletteRoot.documentController
        tools: paletteRoot.tools
        view: paletteRoot.view
        profile: paletteRoot.profile
    }

    // Lot 3 : corbeille sous la palette (« Afficher la corbeille » la masque).
    TrashCan {
        id: trash
        // Commande qui affiche la corbeille (identifiant lu depuis une propriété : l'AOT refuse l'argument littéral).
        readonly property string commandId: "view.trash"
        objectName: "trashCan"
        tools: paletteRoot.tools
        commands: paletteRoot.commands
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        visible: paletteRoot.commands.revision >= 0 && paletteRoot.commands.isChecked(trash.commandId)
    }
}
