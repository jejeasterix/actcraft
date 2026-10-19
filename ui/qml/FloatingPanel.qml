import QtQuick
import QtQuick.Controls.Basic
import ActCraft.Ui

// Panneau flottant commun du lot 7 (dés, calculatrice, horloge, boîtes) : titre, fermeture, déplaçable par sa
// barre (DragHandler), borné à son parent (la couche plein cadre de la fenêtre), jetons de Theme. Couverture de
// la page (décision 5) : tant qu'il est visible et que `pageArea` est posé, ni stylet, ni doigt, ni souris ne
// tracent sous lui ; fermé ou caché, il ne couvre plus.
Rectangle {
    id: panel
    property string title
    property Item pageArea: null
    default property alias content: body.data
    signal closed()

    function close() {
        panel.visible = false
        panel.closed()
    }

    color: Theme.palette
    border.color: Theme.contour
    radius: Theme.rayon

    // Vue où le panneau est déclaré couverture (nulle sinon) : un seul enregistrement à la fois, retiré quand
    // le panneau se cache ou change de zone de page.
    QtObject {
        id: cover
        property Item view: null
    }

    function syncCover() {
        const view = panel.pageArea ? panel.pageArea.view : null
        const wanted = panel.visible ? view : null
        if (cover.view === wanted)
            return
        if (cover.view)
            cover.view.removeInputCover(panel)
        cover.view = wanted
        if (wanted)
            wanted.addInputCover(panel)
    }

    Component.onCompleted: syncCover()
    onVisibleChanged: syncCover()
    onPageAreaChanged: syncCover()

    Rectangle {
        id: titleBar
        objectName: "floatingPanelTitleBar"
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        height: Theme.tailleBouton
        color: Theme.barre
        radius: Theme.rayon

        Label {
            anchors.left: parent.left
            anchors.right: closeButton.left
            anchors.verticalCenter: parent.verticalCenter
            anchors.leftMargin: Theme.espace3
            text: panel.title
            textFormat: Text.PlainText
            elide: Text.ElideRight
            color: Theme.texte
            font.pixelSize: Theme.taillePoliceTitre
        }

        Button {
            id: closeButton
            objectName: "floatingPanelClose"
            anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter
            width: Theme.tailleBouton
            height: Theme.tailleBouton
            flat: true
            text: "×"
            font.pixelSize: Theme.taillePoliceTitre
            Accessible.name: qsTr("Fermer")
            onClicked: panel.close()
        }

        DragHandler {
            objectName: "floatingPanelDrag"
            target: panel
            xAxis.minimum: 0
            xAxis.maximum: panel.parent ? Math.max(0, panel.parent.width - panel.width) : 0
            yAxis.minimum: 0
            yAxis.maximum: panel.parent ? Math.max(0, panel.parent.height - panel.height) : 0
        }
    }

    Item {
        id: body
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: titleBar.bottom
        anchors.bottom: parent.bottom
        anchors.margins: Theme.espace2
    }
}
