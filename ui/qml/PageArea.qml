import QtQuick
import QtQuick.Controls.Basic
import ActCraft.Ui

// Zone de la page (F-020, F-022) : vue zoomable, cache du contenu fixe, encre ; barres de défilement quand la
// page dépasse ; indicateur de largeur du stylo sous le pointeur (F-138). Noms d'objet lus par le banc : inchangés.
Item {
    id: area
    objectName: "pageArea"
    property alias view: view
    property alias fixedPage: fixedPageItem
    property bool cacheEnabled: true
    property bool inputBlocked: false
    // Document ouvert : le stylo y verse ses traits par la pile (L2-T15).
    property DocumentController documentController: null
    clip: true

    PageView {
        id: view
        objectName: "pageView"
        anchors.fill: parent
        cacheEnabled: area.cacheEnabled
        inputBlocked: area.inputBlocked
        content: pageContent
        fixedPage: fixedPageItem
        ink: inkLayer
        documentController: area.documentController
        // Branché dès la création de la vue (un bloc Connections ne l'est qu'en fin de composant).
        onCacheRefreshRequested: cache.scheduleUpdate()

        // Page au format d'un paperboard (1024 × 768), transformée par la vue.
        Item {
            id: pageContent
            objectName: "page"
            width: 1024
            height: 768

            FixedPage {
                id: fixedPageItem
                objectName: "fixedPage"
                anchors.fill: parent
            }

            // Cache du contenu fixe : redessiné seulement sur demande de la vue.
            ShaderEffectSource {
                id: cache
                objectName: "pageCache"
                anchors.fill: parent
                visible: view.cacheEnabled
                sourceItem: view.cacheEnabled ? fixedPageItem : null
                hideSource: view.cacheEnabled
                live: false
                textureSize: view.cacheTextureSize
                onScheduledUpdateCompleted: view.notifyCacheUpdated()
            }

            InkLayer {
                id: inkLayer
                objectName: "ink"
                anchors.fill: parent
            }
        }
    }

    // Indicateur de largeur (F-138) : cercle de la taille et de la couleur du trait sous le pointeur.
    HoverHandler {
        id: hover
    }
    Rectangle {
        objectName: "widthIndicator"
        visible: hover.hovered && !area.inputBlocked
        width: Math.max(4, view.inkWidth * view.zoom)
        height: width
        radius: width / 2
        x: hover.point.position.x - width / 2
        y: hover.point.position.y - height / 2
        color: "transparent"
        border.width: 1
        border.color: view.inkColor
        // Filet extérieur « contour » : le cercle reste visible quand l'encre a la couleur de la page (L1-C5).
        Rectangle {
            anchors.fill: parent
            anchors.margins: -1
            radius: width / 2
            color: "transparent"
            border.width: 1
            border.color: Theme.contour
        }
    }

    // Pouce des barres de défilement : ScrollThumb (le gris translucide du style Basic tombait sous 3:1 sur la page
    // comme sur le fond, L1-C5).
    ScrollBar {
        id: horizontalBar
        objectName: "horizontalScrollBar"
        contentItem: ScrollThumb {}
        orientation: Qt.Horizontal
        anchors.left: parent.left
        anchors.right: verticalBar.left
        anchors.bottom: parent.bottom
        policy: ScrollBar.AlwaysOn
        visible: size < 1
        size: view.scrollSizeX
        position: view.scrollPositionX
        onPositionChanged: if (pressed) view.scrollTo(position, view.scrollPositionY)
        onPressedChanged: if (!pressed) position = Qt.binding(() => view.scrollPositionX)
    }

    ScrollBar {
        id: verticalBar
        objectName: "verticalScrollBar"
        contentItem: ScrollThumb {}
        orientation: Qt.Vertical
        anchors.top: parent.top
        anchors.right: parent.right
        anchors.bottom: horizontalBar.top
        policy: ScrollBar.AlwaysOn
        visible: size < 1
        size: view.scrollSizeY
        position: view.scrollPositionY
        onPositionChanged: if (pressed) view.scrollTo(view.scrollPositionX, position)
        onPressedChanged: if (!pressed) position = Qt.binding(() => view.scrollPositionY)
    }
}
