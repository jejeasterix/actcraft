import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import ActCraft.Ui

// Fenêtre principale (décision 4 du cadrage livraison-complete ; F-001 à F-003) : ligne supérieure, page au
// centre, palette à droite, pas de barre d'état. Tout bouton, menu ou raccourci déclenche une commande nommée
// (DA1) ; les gestionnaires des commandes du lot 1 sont posés ici.
Window {
    id: root
    width: 1280
    height: 800
    visible: true
    // Rendu effectif dans le titre (décision 19 du prototype) : aucun texte dans la scène.
    // Titre : `*` devant un document modifié, puis le titre du paperboard (L2-T18).
    title: qsTr("%1%2 — ActCraft — rendu : %3").arg(appDocument.modified ? "*" : "").arg(appDocument.title)
               .arg(root.apiName(GraphicsInfo.api))
    color: Theme.fond

    // Palette des contrôles laissés au style Basic (menus, zoom, dialogue, info-bulles, barres de défilement) :
    // tirée de Theme, sinon ils restent clairs dans une interface sombre (critique impeccable, L1-C5). Les contrastes
    // de ces rôles sont ceux des paires vérifiées par tst_ui_theme.
    palette.window: Theme.palette
    palette.windowText: Theme.texte
    palette.base: Theme.barre
    palette.alternateBase: Theme.surface
    palette.text: Theme.texte
    palette.button: Theme.surface
    palette.buttonText: Theme.texte
    palette.brightText: Theme.texte
    palette.light: Theme.surfaceSurvol
    palette.midlight: Theme.surfaceSurvol
    palette.mid: Theme.surfaceSurvol
    palette.dark: Theme.contour
    palette.highlight: Theme.surfaceActive
    palette.highlightedText: Theme.texteSurActive
    palette.link: Theme.surfaceActive
    palette.linkVisited: Theme.surfaceActive
    palette.toolTipBase: Theme.barre
    palette.toolTipText: Theme.texte
    palette.placeholderText: Theme.texteDiscret
    palette.disabled.windowText: Theme.texteDesactive
    palette.disabled.text: Theme.texteDesactive
    palette.disabled.buttonText: Theme.texteDesactive

    // Fixées avant le chargement par l'application, le banc ou les tests (app/startup.cpp).
    property bool cacheEnabled: true
    property bool generatedPage: false
    // État de l'interface, lu par les tests et par actcraft-captures. Le plein écran est l'état réel de la
    // fenêtre : il suit aussi une sortie par le système, et la coche de « Plein écran » le suit.
    readonly property bool fullScreen: root.visibility === Window.FullScreen
    property int openPopups: 0

    onFullScreenChanged: appCommands.setChecked("view.fullscreen", root.fullScreen)

    Commands {
        id: appCommands
        objectName: "commands"
    }

    DocumentController {
        id: appDocument
        objectName: "documentController"
        fixedPage: pageArea.fixedPage
        // Page suivante pendant un trait (PgDown, télécommande, doigt sur le bouton) : le trait se termine et
        // reste sur sa page. finishStroke abandonne d'abord le geste de l'outil (M2) : sans cela, le relâché
        // synthétisé terminerait un glissement de la sélection par un déplacement.
        onPageAboutToChange: pageArea.view.finishStroke()
    }

    // Lot 4 : réglages de la page, miniatures, pages et édition de texte (types C++ de L4-T1).
    PageSettings {
        id: appPageSettings
        objectName: "appPageSettings"
        documentController: appDocument
    }
    PageThumbnails {
        id: appThumbnails
        objectName: "appThumbnails"
        documentController: appDocument
        view: pageArea.view
    }
    Pages {
        id: appPages
        objectName: "appPages"
        documentController: appDocument
    }
    TextEditing {
        id: appText
        objectName: "appText"
        documentController: appDocument
        view: pageArea.view
    }

    // Outils de l'utilisateur (lot 3) : branchés à la vue, à la couche soulevée et aux commandes.
    Tools {
        id: appTools
        objectName: "appTools"
        commands: appCommands
        documentController: appDocument
        view: pageArea.view
        liftLayer: pageArea.liftLayer
    }
    Connections {
        target: appDocument
        function onEditAboutToApply() { pageArea.view.finishStroke() }
        function onUndoChanged() { root.syncEditCommands() }
        function onDocumentChanged() { root.syncEditCommands() }
        function onCanRevertPageChanged() { root.syncEditCommands() }
    }
    // Coches des commandes de grille et de poignée : elles suivent l'état (commande, annulation, changement de
    // page). « Cacher la grille » n'est cochée que pour une grille qui existe et que l'on a cachée.
    Connections {
        target: appPageSettings
        function onChanged() { root.syncGridCommands() }
    }
    Connections {
        target: pageArea.pageExtensionHandle
        function onVisibleChanged() {
            appCommands.setChecked("view.page-extension", pageArea.pageExtensionHandle.visible)
        }
    }
    function syncGridCommands() {
        const levels = appPageSettings.grid.levels
        appCommands.setChecked("grid.hide", !appPageSettings.gridVisible && !!levels && levels.length > 0)
        appCommands.setChecked("grid.snap", appPageSettings.gridSnap)
    }
    function syncEditCommands() {
        appCommands.setEnabled("edit.undo", appDocument.canUndo)
        appCommands.setEnabled("edit.redo", appDocument.canRedo)
        appCommands.setEnabled("page.revert", appDocument.canRevertPage)
    }

    // Pilotage sans souris (actcraft-captures, tests) : même chemin que les boutons.
    function runCommand(id, argument) {
        return appCommands.trigger(id, argument)
    }

    function setFullScreen(on) {
        if (on)
            root.showFullScreen()
        else
            root.showNormal()
    }

    // Argument de text.edit-at : « x;y;v » (coordonnées de page, v = 0 ou 1) ; absent ou illisible : null,
    // c'est-à-dire valider l'édition.
    function editAtArgument(arg) {
        if (arg === undefined || arg === null || arg === "")
            return null
        const parts = String(arg).split(";")
        if (parts.length < 2)
            return null
        const x = Number(parts[0])
        const y = Number(parts[1])
        if (!isFinite(x) || !isFinite(y))
            return null
        return { x: x, y: y, vertical: parts.length > 2 && Number(parts[2]) === 1 }
    }

    function popupOpened() {
        root.openPopups += 1
    }

    function popupClosed() {
        root.openPopups = Math.max(0, root.openPopups - 1)
    }

    function menuSpec(id) {
        const menus = appCommands.menus
        for (let i = 0; i < menus.length; ++i) {
            if (menus[i].id === id)
                return menus[i]
        }
        return { id: id, label: "", items: [] }
    }

    TopBar {
        id: topBar
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        commands: appCommands
        documentController: appDocument
        view: pageArea.view
        onPopupOpened: root.popupOpened()
        onPopupClosed: root.popupClosed()
    }

    RowLayout {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: topBar.bottom
        anchors.bottom: parent.bottom
        spacing: 0

        // Lot 4 : panneau des navigateurs (ancre réglée par L4-T19), à gauche de la zone de page.
        BrowserPanel {
            id: browserPanel
            objectName: "browserPanel"
        }

        PageArea {
            id: pageArea
            Layout.fillWidth: true
            Layout.fillHeight: true
            cacheEnabled: root.cacheEnabled
            inputBlocked: root.openPopups > 0
            documentController: appDocument
            tools: appTools
            commands: appCommands
        }

        // Lot 4 : palette des formes, à droite, avant la palette d'outils.
        ShapePalette {
            id: shapePalette
            objectName: "shapePalette"
            tools: appTools
            commands: appCommands
        }

        ToolPalette {
            id: toolPalette
            Layout.fillHeight: true
            Layout.preferredWidth: Theme.largeurPalette
            commands: appCommands
            view: pageArea.view
            tools: appTools
        }
    }

    // Lot 4 : barres de texte sous la ligne supérieure, boîtes de la grille et du fond (talons).
    TextFormatBar {
        id: textFormatBar
        objectName: "textFormatBar"
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: topBar.bottom
    }
    VerticalTextBar {
        id: verticalTextBar
        objectName: "verticalTextBar"
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: topBar.bottom
    }
    GridDesigner {
        id: gridDesigner
        objectName: "gridDesigner"
        settings: appPageSettings
    }
    BackgroundDialog {
        id: backgroundDialog
        objectName: "backgroundDialog"
    }

    // « Menu principal » (F-122) : les mêmes menus que la ligne supérieure.
    Menu {
        id: mainMenu
        objectName: "mainMenu"
        onOpened: root.popupOpened()
        onClosed: root.popupClosed()
        // Entrées vers les sous-menus : même surlignage que les entrées de commande (L1-C5).
        delegate: MenuItem {
            id: mainEntry
            implicitHeight: Theme.tailleBouton
            font.pixelSize: Theme.taillePolice
            background: ItemHighlight {
                control: mainEntry
            }
        }

        Instantiator {
            model: appCommands.menus
            delegate: CommandMenu {
                required property var modelData
                commands: appCommands
                spec: modelData
            }
            onObjectAdded: (index, object) => mainMenu.insertMenu(index, object)
            onObjectRemoved: (index, object) => mainMenu.removeMenu(object)
        }
    }
    // « Outils » (F-128) : le menu Outils en menu surgissant.
    CommandMenu {
        id: toolsMenu
        objectName: "toolsMenu"
        commands: appCommands
        spec: root.menuSpec("tools")
        onOpened: root.popupOpened()
        onClosed: root.popupClosed()
    }

    AboutDialog {
        id: aboutDialog
        onOpened: root.popupOpened()
        onClosed: root.popupClosed()
    }

    FileActions {
        id: fileActions
        document: appDocument
        onPopupOpened: root.popupOpened()
        onPopupClosed: root.popupClosed()
    }

    // Fermeture de la fenêtre : même demande que « Quitter ».
    onClosing: close => {
        if (appDocument.modified && !fileActions.quitting) {
            close.accepted = false
            fileActions.quit()
        }
    }

    // Avertissements de lecture et erreurs : en haut de la page, sans bloquer, refermables.
    MessageBanner {
        id: messageBanner
        parent: pageArea
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.margins: Theme.espace2
        z: 10
    }
    Connections {
        target: appDocument
        function onLoadFinished(ok, message) {
            if (!ok)
                messageBanner.show(qsTr("Ouverture impossible : %1").arg(message))
            else if (appDocument.warnings.length === 1)
                messageBanner.show(qsTr("1 avertissement de lecture : %1").arg(appDocument.warnings[0]))
            else if (appDocument.warnings.length > 1)
                messageBanner.show(qsTr("%1 avertissements de lecture : %2").arg(appDocument.warnings.length)
                                   .arg(appDocument.warnings.slice(0, 3).join(" ; ")))
            else
                messageBanner.dismiss()
        }
        function onSaveFinished(ok, message) {
            if (!ok)
                messageBanner.show(qsTr("Enregistrement impossible : %1").arg(message))
        }
    }

    // Lot 7 : surcouches plein cadre et panneaux flottants (rideau, spot, message défilant, dés, horloge…).
    Item {
        id: presentationLayer
        objectName: "presentationLayer"
        anchors.fill: parent
        z: 50
    }
    Presentation {
        objectName: "presentation"
        commands: appCommands
        documentController: appDocument
        tools: appTools
        pageArea: pageArea
        pageOverlays: pageArea.pageOverlays
        layer: presentationLayer
    }

    // Raccourcis de la disposition (commande, ou remplacement du profil).
    Instantiator {
        model: appCommands.shortcuts
        delegate: Shortcut {
            required property var modelData
            sequence: modelData.sequence
            context: Qt.ApplicationShortcut
            onActivated: appCommands.trigger(modelData.id)
        }
    }

    Component.onCompleted: {
        appCommands.setHandler("file.quit", function () { fileActions.quit() })
        appCommands.setHandler("file.new", function () { fileActions.newDocument(1024, 768) })
        appCommands.setHandler("file.new-screen", function () { fileActions.newDocument(root.screen.width, root.screen.height) })
        appCommands.setHandler("file.new-1024x768", function () { fileActions.newDocument(1024, 768) })
        appCommands.setHandler("file.new-1152x864", function () { fileActions.newDocument(1152, 864) })
        appCommands.setHandler("file.new-1280x1024", function () { fileActions.newDocument(1280, 1024) })
        appCommands.setHandler("file.new-custom", function () { fileActions.askSize() })
        appCommands.setHandler("file.open", function (url) { fileActions.open(url) })
        appCommands.setHandler("file.save", function () { fileActions.save() })
        appCommands.setHandler("file.save-as", function (url) { fileActions.saveAs(url) })
        appCommands.setHandler("file.close", function () { fileActions.close() })
        appCommands.setHandler("view.fullscreen", function () { root.setFullScreen(!root.fullScreen) })
        appCommands.setHandler("view.fit", function () { pageArea.view.fitPage() })
        appCommands.setHandler("view.zoom", function (percent) { pageArea.view.zoom = percent / 100 })
        appCommands.setHandler("page.next", function () { appDocument.nextPage() })
        appCommands.setHandler("page.previous", function () { appDocument.pageBack() })
        appCommands.setHandler("tool.pen", function () { appTools.activate("pen") })
        // La vue n'a plus de couleur propre : appTools la tient à jour (inkColor, inkWidth).
        appCommands.setHandler("pen.color", function (color) { appTools.setColor(color) })
        appCommands.setHandler("pen.width", function (width) { appTools.setWidth(width) })
        // Échap (M9) : abandon du geste en cours ; au repos, outil Sélectionner. Jamais livré à l'outil.
        appCommands.setHandler("tool.select", function () {
            appTools.cancelGesture()
            appTools.activate("select")
        })
        appCommands.setHandler("tool.highlighter", function () { appTools.activate("highlighter") })
        appCommands.setHandler("tool.eraser", function () { appTools.activate("eraser") })
        appCommands.setHandler("edit.undo", function () { appDocument.undo() })
        appCommands.setHandler("edit.redo", function () { appDocument.redo() })
        appCommands.setHandler("edit.delete", function () { appTools.deleteSelection() })
        appCommands.setHandler("edit.select-all", function () { appTools.selectAll() })
        appCommands.setHandler("clear.annotations", function () { appDocument.clearPage(0) })
        appCommands.setHandler("clear.objects", function () { appDocument.clearPage(1) })
        appCommands.setHandler("clear.grid", function () { appDocument.clearPage(2) })
        appCommands.setHandler("clear.background", function () { appDocument.clearPage(3) })
        appCommands.setHandler("clear.page", function () { appDocument.clearPage(4) })
        appCommands.setHandler("page.revert", function () { appDocument.revertPage() })
        appCommands.setHandler("view.trash", function () { appCommands.setChecked("view.trash", !appCommands.isChecked("view.trash")) })
        appCommands.setHandler("object.translucency", function (value) {
            if (value === undefined)
                pageArea.selectionOverlay.openTranslucency()
            else
                appTools.setSelectionTranslucency(value)
        })
        appCommands.setHandler("object.enlarge", function () { appTools.enlargeSelection() })
        appCommands.setHandler("object.reduce", function () { appTools.reduceSelection() })
        appCommands.setHandler("menu.main", function () { mainMenu.popup() })
        appCommands.setHandler("menu.tools", function () { toolsMenu.popup() })
        appCommands.setHandler("help.about", function () { aboutDialog.open() })
        appCommands.setHandler("help.website", function () { Qt.openUrlExternally(AppInfo.projectUrl) })
        // Lot 4 : formes, texte, navigateurs, pages, grille (une ligne vers sa cible chacune).
        appCommands.setHandler("tool.shape", function () { appTools.activate("shape") })
        appCommands.setHandler("shape.choose", function (preset) { appTools.chooseShape(preset) })
        appCommands.setHandler("shape.filled", function () { appTools.setShapeFilled(!appTools.shapeFilled) })
        appCommands.setHandler("shape.fill-color", function (color) { appTools.setShapeFillColor(color) })
        appCommands.setHandler("shape.draw-from-center", function () { appTools.setShapeFromCenter(!appTools.shapeFromCenter) })
        appCommands.setHandler("tool.text", function () { appTools.activate("text") })
        appCommands.setHandler("tool.text-vertical", function () { appTools.activate("text-vertical") })
        appCommands.setHandler("text.edit-at", function (arg) {
            const at = root.editAtArgument(arg)
            if (at === null)
                appText.commit()
            else
                appText.beginAt(Qt.point(at.x, at.y), at.vertical)
        })
        for (const id of ["text.font", "text.size", "text.size-up", "text.size-down", "text.bold", "text.italic",
                          "text.underline", "text.superscript", "text.subscript", "text.color", "text.background",
                          "text.align-left", "text.align-center", "text.align-right", "text.spacing-up",
                          "text.spacing-down", "text.indent", "text.outdent", "text.bullets", "text.select-all",
                          "text.symbol"])
            appCommands.setHandler(id, function (arg) { appText.apply(id, arg) })
        appCommands.setHandler("view.browsers", function () { browserPanel.visible = !browserPanel.visible })
        appCommands.setHandler("browsers.dock-left", function () { browserPanel.side = "left" })
        appCommands.setHandler("browsers.dock-right", function () { browserPanel.side = "right" })
        appCommands.setHandler("page.goto", function (number) { appPages.goTo(number - 1) })
        appCommands.setHandler("page.insert-before", function () { appPages.insertBlank(false) })
        appCommands.setHandler("page.insert-after", function () { appPages.insertBlank(true) })
        appCommands.setHandler("page.cut", function () { appPages.cutSelection() })
        appCommands.setHandler("page.copy", function () { appPages.copySelection() })
        appCommands.setHandler("page.paste", function () { appPages.paste() })
        appCommands.setHandler("page.duplicate", function () { appPages.duplicateSelection() })
        appCommands.setHandler("page.delete", function () { appPages.deleteSelection() })
        appCommands.setHandler("page.background", function () { backgroundDialog.open() })
        appCommands.setHandler("grid.designer", function () { gridDesigner.open() })
        appCommands.setHandler("grid.hide", function () { appPageSettings.toggleGridHidden() })
        appCommands.setHandler("grid.snap", function () { appPageSettings.toggleSnap() })
        appCommands.setHandler("view.page-extension", function () {
            pageArea.pageExtensionHandle.visible = !pageArea.pageExtensionHandle.visible
        })
        root.syncGridCommands()
        appCommands.setChecked("view.page-extension", pageArea.pageExtensionHandle.visible)
        appCommands.setHandler("page.extend", function () { appPageSettings.extendPage() })
        appCommands.setHandler("page.zones", function (zones) { appPageSettings.setZones(zones) })
        appCommands.setChecked("tool.pen", true) // F-200 : le stylo est l'outil de départ
        appCommands.setChecked("view.trash", true)
        root.syncEditCommands()
        if (root.generatedPage)
            appDocument.loadGeneratedFirstPage()
    }

    function apiName(api) {
        switch (api) {
        case GraphicsInfo.Software: return "logiciel"
        case GraphicsInfo.Direct3D11: return "Direct3D 11"
        case GraphicsInfo.Direct3D12: return "Direct3D 12"
        case GraphicsInfo.Metal: return "Metal"
        case GraphicsInfo.Vulkan: return "Vulkan"
        case GraphicsInfo.OpenGL: return "OpenGL"
        default: return "inconnu"
        }
    }
}
