import QtQuick
import ActCraft.Ui

// Poignée d'allongement de la page (F-301 a) : barre arrondie et flèche, centrée au bas de la page, de taille
// constante à l'écran. Un toucher déclenche la commande « page.extend » sans passer par l'outil courant : la
// poignée est une couverture d'entrée de la vue (elle prend l'appui, la vue l'ignore).
Item {
    id: handle
    // Posés par PageArea.
    property PageView pageView: null
    property Commands commands: null
    // Identifiant lu depuis une propriété : l'AOT refuse l'argument littéral (voir TrashCan.qml).
    readonly property string extendId: "page.extend"

    visible: false
    width: 72
    height: 24
    // Le parent couvre la page, en coordonnées de page : la poignée en annule le zoom.
    scale: pageView ? 1 / pageView.zoom : 1
    x: parent ? parent.width / 2 - width / 2 : 0
    y: parent ? parent.height - height / 2 : 0

    // Vrai si le centre est à moins d'un pixel du bas de la page, en coordonnées de scène. mapToItem ne crée
    // aucune dépendance : zoom, défilement et géométrie sont lus explicitement.
    readonly property bool atPageBottom: {
        if (!pageView || !parent) {
            return false
        }
        void (pageView.zoom + pageView.pan.x + pageView.pan.y + x + y + width + height + scale)
        const centre = mapToItem(null, width / 2, height / 2)
        const bas = pageView.pageToScene(Qt.point(parent.width / 2, parent.height))
        return Math.abs(centre.x - bas.x) < 1 && Math.abs(centre.y - bas.y) < 1
    }

    Component.onCompleted: if (pageView) pageView.addInputCover(handle)
    Component.onDestruction: if (pageView) pageView.removeInputCover(handle)

    Rectangle {
        anchors.fill: parent
        radius: height / 2
        color: tap.pressed ? Theme.surfaceActive : Theme.surface
        border.width: 1
        border.color: Theme.contour

        // Flèche vers le bas : deux traits inclinés.
        Rectangle {
            width: 11
            height: 3
            radius: 1
            color: tap.pressed ? Theme.texteSurActive : Theme.texte
            rotation: 40
            x: parent.width / 2 - width + 3
            y: parent.height / 2 - height / 2
        }
        Rectangle {
            width: 11
            height: 3
            radius: 1
            color: tap.pressed ? Theme.texteSurActive : Theme.texte
            rotation: -40
            x: parent.width / 2 - 3
            y: parent.height / 2 - height / 2
        }
    }

    TapHandler {
        id: tap
        acceptedButtons: Qt.LeftButton
        onTapped: if (handle.commands) handle.commands.trigger(handle.extendId)
    }
}
