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
    // Lot 3 : outils de l'utilisateur ; null pour une annotation de bureau (lot 7), où la zone dessine sans eux.
    property Tools tools: null
    property Commands commands: null
    // Emplacements des lots 3 et 7 (ordre des couches figé par le socle).
    property alias liftLayer: liftLayerItem
    property alias pageOverlays: pageOverlaysItem
    property alias presentationSlot: presentationSlotItem
    property alias selectionOverlay: selectionOverlayItem
    // Lot 4 : poignée d'extension (commande view.page-extension) et grille du dessus.
    property alias pageExtensionHandle: pageExtensionHandleItem
    property alias gridOverlay: gridOverlayItem
    // Nombre de demandes de menu contextuel reçues de la vue (espion des tests).
    property int contextMenuRequestCount: 0
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
        pendingLayer: pendingLayerItem
        documentController: area.documentController
        tools: area.tools
        // Clic droit non couvert : la vue le signale (aucun TapHandler de clic droit ici).
        onContextMenuRequested: viewPos => {
            area.contextMenuRequestCount += 1
            pageContextMenu.popupAt(viewPos.x, viewPos.y)
        }
        // Branché dès la création de la vue (un bloc Connections ne l'est qu'en fin de composant).
        onCacheRefreshRequested: cache.scheduleUpdate()

        // Page à la taille de la page affichée (F-602), transformée par la vue.
        Item {
            id: pageContent
            objectName: "page"
            width: area.documentController ? area.documentController.pageSize.width : 1024
            height: area.documentController ? area.documentController.pageSize.height : 768

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
                // = kCacheMsaaSamples en RHI, 1 en rendu logiciel (PageView.cacheSamples) ; relu par
                // bench/scenarios/anticrenelage.json (garde de liaison, pas une preuve du MSAA : celle-ci est
                // checkEdge, à l'écran). L10-T5 : texture du cache lissée en RHI ; H10-9 : son plafond suit cet
                // échantillonnage.
                samples: view.cacheSamples
                onScheduledUpdateCompleted: view.notifyCacheUpdated()
            }

            // Lot 10 : objets ajoutés depuis le dernier redessin du cache, dessinés au-dessus de lui. Sœur du
            // cache, jamais enfant de la page fixe : hideSource la cacherait avec elle. Rognée à la page comme le
            // cache (sourceRect vide : le rectangle de la page) : un trait qui sort sur la marge n'y apparaît
            // pas pendant l'attente pour disparaître au versement.
            PendingLayer {
                id: pendingLayerItem
                objectName: "pendingLayer"
                anchors.fill: parent
                clip: true
                fixedPage: fixedPageItem
            }

            // Lot 3 : objets soulevés pendant un geste, au-dessus de la couche d'attente, sous l'encre.
            LiftLayer {
                id: liftLayerItem
                objectName: "liftLayer"
                anchors.fill: parent
                fixedPage: fixedPageItem
            }

            // Encre en cours : non rognée (règle du lot 2, inchangée) ; elle déborde sur la marge pendant le
            // tracé, le trait figé est rogné à la page (cache et couche d'attente).
            InkLayer {
                id: inkLayer
                objectName: "ink"
                anchors.fill: parent
            }

            // Lot 4 : grille du dessus (F-620), au-dessus de l'encre en cours (observation de l'original, L4-O2 :
            // elle couvre tout ce qui est posé, le trait en cours compris, H4-22), hors de la scène que le cache
            // photographie. Sans `z:` : l'ordre des enfants fait l'empilement.
            GridOverlay {
                id: gridOverlayItem
                objectName: "gridOverlay"
                anchors.fill: parent
                zoom: view.zoom
                fixedPage: fixedPageItem
            }
            // PageSettingsChanged n'atteint pas la page fixe : la grille est relue sur le changement de la page
            // affichée.
            Connections {
                target: area.documentController
                function onPageContentChanged(page) {
                    if (area.documentController && page === area.documentController.currentIndex)
                        gridOverlayItem.refresh()
                }
            }

            // Lot 7 : surcouches en coordonnées de page (outils de maths, origine XY, spot, rideau de la page), au
            // dessus de l'encre et de la couche d'attente ; jamais dans la scène que le cache photographie. Une
            // surcouche qui prend l'entrée se déclare couverture (view.addInputCover).
            Item {
                id: pageOverlaysItem
                objectName: "pageOverlays"
                anchors.fill: parent

                // Lot 4 : zones et poignée d'extension de la page (talons).
                PageZonesOverlay {
                    objectName: "pageZones"
                    anchors.fill: parent
                }
                PageExtensionHandle {
                    id: pageExtensionHandleItem
                    objectName: "pageExtensionHandle"
                }
            }
        }
    }

    // Lot 3 : sélection (cadre, poignées, barre d'objet) ; sœur de la vue, jamais dessous. Ses gestionnaires d'appui
    // vont sur les poignées et la barre seulement, pas sur sa racine (le stylet hors du cadre doit être versé).
    SelectionOverlay {
        id: selectionOverlayItem
        objectName: "selectionOverlay"
        anchors.fill: parent
        tools: area.tools
        view: view
        visible: area.tools !== null
    }
    // Lot 4 : éditeur de texte (talon), à côté de la sélection.
    TextEditor {
        objectName: "textEditor"
    }
    ToolCursor {
        id: toolCursor
        objectName: "toolCursor"
        anchors.fill: parent
        tools: area.tools
    }
    // Lot 7 : surcouches en coordonnées de la zone (au-dessus de la sélection, sous les barres de défilement).
    Item {
        id: presentationSlotItem
        objectName: "presentationSlot"
        anchors.fill: parent
    }

    // Indicateur de largeur (F-138) : cercle de la taille et de la couleur du trait sous le pointeur, pour les
    // outils d'encre seulement. Avec la sélection, la flèche suffit (ToolCursor) : le cercle de l'encre gardée
    // restait sous le pointeur, au point d'appui sur un objet verrouillé (L3-C15). Sans Tools : le stylo.
    HoverHandler {
        id: hover
        cursorShape: toolCursor.cursorShape
    }
    Rectangle {
        objectName: "widthIndicator"
        visible: hover.hovered && !area.inputBlocked && !(area.tools && area.tools.currentTool === "select")
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

    PageContextMenu {
        id: pageContextMenu
        objectName: "pageContextMenu"
        commands: area.commands
    }
}
