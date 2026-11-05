import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import ActCraft.Ui

// Onglet Commandes (F-910) : à gauche les commandes disponibles sur la plateforme, par catégorie ; à droite
// une cible (palette, barre d'objet, menus) et son contenu. « Appliquer » écrit le seul layout.json du profil
// (LayoutEditor.apply), puis recharge les commandes ; quitter l'onglet (« Terminé » de l'éditeur, autre
// onglet) applique aussi ce qui ne l'est pas encore : un seul « Terminé » dans la fenêtre, rien de perdu en
// silence. Aucun accès disque en QML.
Item {
    id: root
    objectName: "commandsPage"
    property Commands commands: null
    property DocumentController documentController: null
    property Tools tools: null
    property PageView view: null
    property var profile: null
    property var preferences: null
    // Pas de taille implicite, comme le talon : avec 560 × 420, le StackLayout de ProfileEditor.qml passait à
    // 879 × 460 et le bas de FormatPage sortait de la fenêtre (tst_ui_profileeditor).

    LayoutEditor { id: layoutEditor; objectName: "layoutEditor"; commands: root.commands; profile: root.profile }

    // Popup de l'éditeur fermée ou autre onglet du StackLayout : la visibilité effective tombe.
    onVisibleChanged: if (!visible && layoutEditor.modified) layoutEditor.apply()

    RowLayout {
        anchors.fill: parent
        spacing: 8

        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Label { text: qsTr("Commandes disponibles"); font.bold: true }
            ListView {
                id: catalogList
                objectName: "catalogList"
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true
                model: layoutEditor.catalog
                delegate: Column {
                    id: catalogEntry
                    required property var modelData
                    required property int index
                    width: ListView.view.width
                    Label {
                        visible: catalogEntry.modelData.categoryStart
                        text: catalogEntry.modelData.categoryLabel ?? ""
                        font.italic: true
                    }
                    ItemDelegate {
                        width: parent.width
                        text: catalogEntry.modelData.label
                        icon.source: catalogEntry.modelData.icon
                        highlighted: catalogList.currentIndex === catalogEntry.index
                        onClicked: catalogList.currentIndex = catalogEntry.index
                        onDoubleClicked: layoutEditor.add(catalogEntry.modelData.id)
                    }
                }
            }
        }

        ColumnLayout {
            Layout.alignment: Qt.AlignVCenter
            Button {
                objectName: "addButton"
                text: qsTr("Ajouter")
                enabled: catalogList.currentIndex >= 0 && catalogList.currentIndex < catalogList.count
                onClicked: layoutEditor.add(layoutEditor.catalog[catalogList.currentIndex].id)
            }
            Button {
                objectName: "removeButton"
                text: qsTr("Retirer")
                enabled: layoutEditor.current >= 0
                onClicked: layoutEditor.removeAt(layoutEditor.current)
            }
            Button {
                objectName: "moveUpButton"
                text: qsTr("Monter")
                enabled: layoutEditor.current > 0
                onClicked: layoutEditor.moveUp(layoutEditor.current)
            }
            Button {
                objectName: "moveDownButton"
                text: qsTr("Descendre")
                enabled: layoutEditor.current >= 0 && layoutEditor.current < targetList.count - 1
                onClicked: layoutEditor.moveDown(layoutEditor.current)
            }
        }

        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            ComboBox {
                objectName: "targetCombo"
                Layout.fillWidth: true
                model: layoutEditor.targets
                textRole: "label"
                valueRole: "id"
                currentIndex: Math.max(0, indexOfValue(layoutEditor.target))
                onActivated: layoutEditor.target = currentValue
            }
            ListView {
                id: targetList
                objectName: "targetList"
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true
                model: layoutEditor.items
                // La sélection vit dans LayoutEditor.current : un nouveau modèle remet currentIndex à 0, le
                // signal current qui suit le rétablit, et l'entrée choisie reste montrée.
                currentIndex: layoutEditor.current
                onCurrentIndexChanged: if (currentIndex >= 0) positionViewAtIndex(currentIndex, ListView.Contain)
                delegate: ItemDelegate {
                    id: targetEntry
                    required property var modelData
                    required property int index
                    width: ListView.view.width
                    // Séparateur : un trait ; en-tête de groupe : son nom ; bloc fixe (nuancier, épaisseur) : son
                    // nom entre crochets, sans commande ; sous-menu : « ▸ ».
                    text: modelData.separator
                          ? (modelData.group ? (modelData.fixed ? "[ " + modelData.label + " ]"
                                                                : "— " + modelData.label + " —")
                                             : "——————")
                          : modelData.submenu ? modelData.label + " ▸" : modelData.label
                    icon.source: modelData.separator || modelData.submenu ? "" : modelData.icon
                    opacity: modelData.available ? 1 : 0.5 // absente de la plateforme : gardée, grisée
                    highlighted: layoutEditor.current === index
                    onClicked: layoutEditor.current = index
                }
            }
            Label {
                objectName: "applyError"
                Layout.fillWidth: true
                visible: text.length > 0
                text: layoutEditor.error
                color: "#b00020"
                wrapMode: Text.Wrap
            }
            RowLayout {
                Button {
                    objectName: "separatorButton"
                    text: qsTr("Séparateur")
                    enabled: layoutEditor.separatorAllowed
                    onClicked: layoutEditor.addSeparator()
                }
                Item { Layout.fillWidth: true }
                Button {
                    objectName: "applyButton"
                    text: qsTr("Appliquer")
                    enabled: layoutEditor.modified
                    onClicked: layoutEditor.apply()
                }
            }
        }
    }
}
