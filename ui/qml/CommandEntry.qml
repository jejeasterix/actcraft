import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import ActCraft.Ui

// Une entrée de menu : commande (libellé, raccourci et coche lus dans le registre) ou séparateur. `payload` est
// l'élément de la disposition sérialisé en JSON (voir CommandMenu.qml).
MenuItem {
    id: entry
    required property var commands
    required property string payload
    readonly property var entryData: JSON.parse(payload)
    readonly property bool isSeparator: entryData.separator === true
    readonly property string commandId: isSeparator ? "" : entryData.id
    objectName: isSeparator ? "menuSeparator" : "menuItem-" + commandId
    text: isSeparator ? "" : commands.label(commandId)
    enabled: !isSeparator && commands.revision >= 0 && commands.isEnabled(commandId)
    checkable: !isSeparator && entryData.checkable === true
    checked: !isSeparator && commands.revision >= 0 && commands.isChecked(commandId)
    implicitHeight: isSeparator ? 9 : Theme.tailleBouton
    // Le clic bascule la coche localement : on rend la main au registre.
    onToggled: checked = Qt.binding(function () {
        return !entry.isSeparator && commands.revision >= 0 && commands.isChecked(entry.commandId)
    })
    onTriggered: if (!isSeparator) commands.trigger(commandId)
    background: ItemHighlight {
        control: entry
    }
    contentItem: Item {
        implicitHeight: entry.isSeparator ? 1 : label.implicitHeight
        Rectangle {
            visible: entry.isSeparator
            anchors.verticalCenter: parent.verticalCenter
            width: parent.width
            height: 1
            color: Theme.separateur
        }
        // La colonne de la coche (indicateur du style, dessiné à gauche) est réservée sur chaque entrée : la
        // coche ne recouvre plus le libellé et les libellés d'un même menu restent alignés (L1-C5).
        RowLayout {
            visible: !entry.isSeparator
            anchors.fill: parent
            anchors.leftMargin: Theme.tailleIcone + Theme.espace1
            spacing: Theme.espace3
            Label {
                id: label
                text: entry.text
                Layout.fillWidth: true
                font.pixelSize: Theme.taillePolice
                color: entry.enabled ? Theme.texte : Theme.texteDesactive
            }
            Label {
                text: entry.isSeparator ? "" : commands.shortcut(entry.commandId)
                font.pixelSize: Theme.taillePolice
                color: entry.enabled ? Theme.texteDiscret : Theme.texteDesactive
            }
        }
    }
}
