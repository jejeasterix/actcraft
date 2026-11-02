import QtQuick
import QtQuick.Controls.Basic
import ActCraft.Ui

// Liste déroulante des barres de texte (le style de base est clair) : le TextFormatBar et le VerticalTextBar la
// partagent. `popupOpened`/`popupClosed` disent à la barre qu'une liste est ouverte (la vue ne reçoit rien alors).
ComboBox {
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
