import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import ActCraft.Ui

// Onglet Commandes (F-910) : à gauche les commandes disponibles sur la plateforme, par catégorie ; à droite
// une cible (palette, barre d'objet, menus) et son contenu. « Terminé » écrit le seul layout.json du profil
// (LayoutEditor.apply), puis recharge les commandes ; aucun accès disque en QML.
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

    LayoutEditor { id: layoutEditor; commands: root.commands; profile: root.profile }

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
                        text: catalogEntry.modelData.category
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
                enabled: targetList.currentIndex >= 0 && targetList.currentIndex < targetList.count
                onClicked: {
                    const i = targetList.currentIndex
                    if (layoutEditor.removeAt(i))
                        targetList.currentIndex = Math.min(i, targetList.count - 1)
                }
            }
            Button {
                objectName: "moveUpButton"
                text: qsTr("Monter")
                enabled: targetList.currentIndex > 0
                onClicked: {
                    const i = targetList.currentIndex
                    if (layoutEditor.moveUp(i))
                        targetList.currentIndex = i - 1
                }
            }
            Button {
                objectName: "moveDownButton"
                text: qsTr("Descendre")
                enabled: targetList.currentIndex >= 0 && targetList.currentIndex < targetList.count - 1
                onClicked: {
                    const i = targetList.currentIndex
                    if (layoutEditor.moveDown(i))
                        targetList.currentIndex = i + 1
                }
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
                delegate: ItemDelegate {
                    id: targetEntry
                    required property var modelData
                    required property int index
                    width: ListView.view.width
                    // Séparateur : un trait ; en-tête de groupe de la palette : son nom ; sous-menu : « ▸ ».
                    text: modelData.separator
                          ? (modelData.group ? "— " + modelData.group + " —" : "——————")
                          : modelData.submenu ? modelData.label + " ▸" : modelData.label
                    icon.source: modelData.separator || modelData.submenu ? "" : modelData.icon
                    opacity: modelData.available ? 1 : 0.5 // absente de la plateforme : gardée, grisée
                    highlighted: targetList.currentIndex === index
                    onClicked: targetList.currentIndex = index
                }
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
                    text: qsTr("Terminé")
                    enabled: layoutEditor.modified
                    onClicked: layoutEditor.apply()
                }
            }
        }
    }
}
