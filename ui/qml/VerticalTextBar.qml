import QtQuick
import QtQuick.Controls.Basic
import ActCraft.Ui

// Barre du texte de haut en bas (F-301 b, D9) : police, taille, gras, italique, souligné, sous la ligne
// supérieure, tant qu'un tel texte s'édite. Chaque bouton déclenche sa commande nommée (aucun ne prend le
// focus : le TextEdit garde la frappe et sa sélection). Couverture de la vue : le stylet posé sur la barre ne
// va pas à l'outil, qui validerait le texte.
TextBarBase {
    id: bar
    function closeLists() {
        fontCombo.popup.close()
        sizeCombo.popup.close()
    }
    onCloseRequested: bar.closeLists()

    visible: editing !== null && editing.active && editing.vertical

    Row {
        id: row
        anchors.fill: parent
        anchors.leftMargin: Theme.espace2
        spacing: Theme.espace1

        BarCombo {
            id: fontCombo
            objectName: "vbar-font"
            onPopupOpened: bar.listOpened()
            onPopupClosed: bar.listClosed()
            width: 168
            y: (row.height - height) / 2
            model: Qt.fontFamilies()
            displayText: bar.editing ? bar.editing.fontFamily : ""
            onActivated: index => {
                bar.commands.trigger(bar.fontId, model[index])
                bar.backToText()
            }
        }
        BarCombo {
            id: sizeCombo
            objectName: "vbar-size"
            onPopupOpened: bar.listOpened()
            onPopupClosed: bar.listClosed()
            width: 76
            y: (row.height - height) / 2
            model: bar.sizes
            displayText: bar.editing ? String(Math.round(bar.editing.fontSize)) : ""
            onActivated: index => {
                bar.commands.trigger(bar.sizeId, bar.sizes[index])
                bar.backToText()
            }
        }
        CommandButton { objectName: "vbar-bold"; commands: bar.commands; commandId: "text.bold"; checked: bar.editing !== null && bar.editing.bold; y: (row.height - height) / 2 }
        CommandButton { objectName: "vbar-italic"; commands: bar.commands; commandId: "text.italic"; checked: bar.editing !== null && bar.editing.italic; y: (row.height - height) / 2 }
        CommandButton { objectName: "vbar-underline"; commands: bar.commands; commandId: "text.underline"; checked: bar.editing !== null && bar.editing.underline; y: (row.height - height) / 2 }
    }
}
