import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import ActCraft.Ui

// Menu construit depuis la disposition (actions/data/layout.json) : une entrée par commande disponible ; libellé,
// raccourci et coche lus dans le registre ; un clic déclenche la commande par son nom. Les entrées sont créées par
// un Instantiator (jamais par createObject sans parent : elles survivaient au menu et lisaient un registre disparu).
Menu {
    id: menu
    required property Commands commands
    property var spec: ({ id: "", label: "", items: [] })
    title: spec.label

    Instantiator {
        model: menu.spec.items || []
        delegate: MenuItem {
            id: entry
            required property var modelData
            readonly property bool isSeparator: modelData.separator === true
            readonly property string commandId: isSeparator ? "" : modelData.id
            objectName: isSeparator ? "menuSeparator" : "menuItem-" + commandId
            text: isSeparator ? "" : menu.commands.label(commandId)
            enabled: !isSeparator && menu.commands.revision >= 0 && menu.commands.isEnabled(commandId)
            checkable: !isSeparator && modelData.checkable === true
            checked: !isSeparator && menu.commands.revision >= 0 && menu.commands.isChecked(commandId)
            implicitHeight: isSeparator ? 9 : Theme.tailleBouton
            // Le clic bascule la coche localement : on rend la main au registre.
            onToggled: checked = Qt.binding(function () {
                return !entry.isSeparator && menu.commands.revision >= 0 && menu.commands.isChecked(entry.commandId)
            })
            onTriggered: if (!isSeparator) menu.commands.trigger(commandId)
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
                        text: entry.isSeparator ? "" : menu.commands.shortcut(entry.commandId)
                        font.pixelSize: Theme.taillePolice
                        color: entry.enabled ? Theme.texteDiscret : Theme.texteDesactive
                    }
                }
            }
        }
        onObjectAdded: (index, object) => menu.insertItem(index, object)
        onObjectRemoved: (index, object) => menu.removeItem(object)
    }
}