import QtQuick
import QtQuick.Controls.Basic
import ActCraft.Ui

// Barre du texte de haut en bas (F-301 b, D9) : police, taille, gras, italique, souligné, sous la ligne
// supérieure, tant qu'un tel texte s'édite. Chaque bouton déclenche sa commande nommée (aucun ne prend le
// focus : le TextEdit garde la frappe et sa sélection). Couverture de la vue : le stylet posé sur la barre ne
// va pas à l'outil, qui validerait le texte.
Item {
    id: bar
    property Commands commands: null
    property TextEditing editing: null
    property PageView pageView: null
    // Listes ouvertes (police, taille) : la vue ne reçoit rien tant que l'une l'est.
    property int openLists: 0
    signal popupOpened()
    signal popupClosed()
    function listOpened() { bar.openLists += 1; bar.popupOpened() }
    function listClosed() { bar.openLists = Math.max(0, bar.openLists - 1); bar.popupClosed() }
    function closeLists() {
        fontCombo.popup.close()
        sizeCombo.popup.close()
    }
    onVisibleChanged: if (!visible) bar.closeLists()
    Component.onCompleted: if (pageView) pageView.addInputCover(bar)
    Component.onDestruction: if (pageView) pageView.removeInputCover(bar)

    // Identifiants lus depuis des propriétés : l'AOT refuse l'argument littéral (voir TrashCan.qml).
    readonly property string fontId: "text.font"
    readonly property string sizeId: "text.size"
    readonly property var sizes: [8, 10, 12, 14, 18, 24, 36, 48, 72]

    visible: editing !== null && editing.active && editing.vertical
    implicitHeight: Theme.tailleBouton + Theme.espace2
    height: implicitHeight

    function backToText() {
        if (editing)
            editing.focusRequested()
    }

    Rectangle {
        anchors.fill: parent
        color: Theme.palette
        MouseArea { anchors.fill: parent }
        Rectangle {
            anchors.bottom: parent.bottom
            width: parent.width
            height: 1
            color: Theme.separateur
        }
    }

    component BarCombo: ComboBox {
        id: combo
        signal popupOpened()
        signal popupClosed()
        focusPolicy: Qt.NoFocus
        implicitHeight: Theme.tailleBouton - Theme.espace2
        font.pixelSize: Theme.taillePolice
        contentItem: Text {
            leftPadding: Theme.espace2
            rightPadding: combo.indicator.width + Theme.espace1
            text: combo.displayText
            textFormat: Text.PlainText // un nom de famille lu dans le document n'est jamais du HTML
            font: combo.font
            color: Theme.texte
            elide: Text.ElideRight
            verticalAlignment: Text.AlignVCenter
        }
        indicator: Text {
            x: combo.width - width - Theme.espace2
            y: (combo.height - height) / 2
            text: "▾"
            color: Theme.texteDiscret
            font.pixelSize: Theme.taillePolice
        }
        background: Rectangle {
            radius: Theme.rayon
            color: combo.hovered || combo.popup.visible ? Theme.surfaceSurvol : Theme.surface
        }
        delegate: ItemDelegate {
            id: entry
            required property var modelData
            required property int index
            width: combo.width
            height: Theme.tailleBouton - Theme.espace2
            focusPolicy: Qt.NoFocus
            highlighted: combo.highlightedIndex === index
            contentItem: Text {
                text: entry.modelData
                textFormat: Text.PlainText
                font: combo.font
                color: entry.highlighted ? Theme.texteSurActive : Theme.texte
                elide: Text.ElideRight
                verticalAlignment: Text.AlignVCenter
            }
            background: Rectangle { color: entry.highlighted ? Theme.surfaceActive : "transparent" }
        }
        popup: Popup {
            onOpened: combo.popupOpened()
            onClosed: combo.popupClosed()
            y: combo.height
            width: combo.width
            height: Math.min(contentItem.implicitHeight, 8 * (Theme.tailleBouton - Theme.espace2))
            padding: 1
            contentItem: ListView {
                clip: true
                implicitHeight: contentHeight
                model: combo.popup.visible ? combo.delegateModel : null
                currentIndex: combo.highlightedIndex
            }
            background: Rectangle {
                color: Theme.palette
                border.color: Theme.contour
                radius: Theme.rayon
            }
        }
    }

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
