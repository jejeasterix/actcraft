import QtQuick
import ActCraft.Ui

// Ossature des deux barres de texte (TextFormatBar, VerticalTextBar) : commandes, texte édité, vue (couverture
// d'entrée : le stylet posé sur la barre ne va pas à l'outil, qui validerait le texte), compte des listes
// ouvertes (la vue ne reçoit rien tant que l'une l'est), fond qui arrête l'appui, ligne de séparation. Une barre
// fermée ferme ses listes : elle répond à `closeRequested`.
Item {
    id: base
    property Commands commands: null
    property TextEditing editing: null
    property PageView pageView: null
    property int openLists: 0
    signal popupOpened()
    signal popupClosed()
    signal closeRequested()
    function listOpened() { base.openLists += 1; base.popupOpened() }
    function listClosed() { base.openLists = Math.max(0, base.openLists - 1); base.popupClosed() }
    onVisibleChanged: if (!visible) base.closeRequested()
    Component.onCompleted: if (pageView) pageView.addInputCover(base)
    Component.onDestruction: if (pageView) pageView.removeInputCover(base)

    // Identifiants lus depuis des propriétés : l'AOT refuse l'argument littéral (voir TrashCan.qml).
    readonly property string fontId: "text.font"
    readonly property string sizeId: "text.size"
    readonly property var sizes: [8, 10, 12, 14, 18, 24, 36, 48, 72]

    implicitHeight: Theme.tailleBouton + Theme.espace2
    height: implicitHeight

    // Rend la frappe au texte après un choix dans une liste ou une fenêtre (le focus est passé au bouton).
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
}
