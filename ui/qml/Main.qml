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
    // Dossier des profils de l'utilisateur ; vide : <AppConfigLocation>/profiles (StartupOptions::profileRoot).
    property string profileRoot: ""
    // État de l'interface, lu par les tests et par actcraft-captures. Le plein écran est l'état réel de la
    // fenêtre : il suit aussi une sortie par le système, et la coche de « Plein écran » le suit.
    readonly property bool fullScreen: root.visibility === Window.FullScreen
    property int openPopups: 0

    onFullScreenChanged: appCommands.setChecked("view.fullscreen", root.fullScreen)

    Commands {
        id: appCommands
        objectName: "commands"
    }

    // Lot 6 (L6-T5) : réglages du poste et profil courant, relu au démarrage (à défaut, « preparation »).
    Preferences {
        id: appPreferences
        objectName: "appPreferences"
    }
    ProfileController {
        id: appProfile
        objectName: "appProfile"
        commands: appCommands
        preferences: appPreferences
        userRoot: root.profileRoot
    }

    // Lot 6 (L6-T22) : mode conception (F-560). Gestionnaire natif de `mode.design`, réglages du profil
    // (section design-mode), bascule gardée pendant un contact. Il ne touche pas au document : la vue de page
    // montre les objets masqués, les outils ignorent le verrou et l'accrochage ; miniatures et export non.
    DesignMode {
        id: appDesign
        objectName: "appDesign"
        commands: appCommands
        profile: appProfile
        contactActive: pageArea.view ? pageArea.view.contactActive : false
    }
    Binding {
        target: pageArea.fixedPage
        property: "showHiddenTranslucent"
        value: appDesign.hiddenShown
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
        // Amendement 5 (L4-T19) : aucun rendu de miniature panneau des navigateurs caché.
        active: browserPanel.visible && browserPanel.current === "pages"
    }
    Pages {
        id: appPages
        objectName: "appPages"
        documentController: appDocument
        commands: appCommands // amendement 12 : gestes du navigateur refusés sous verrou
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
        lockIgnored: appDesign.lockIgnored
        snapDisabled: appDesign.snapDisabled
    }

    // Lot 5 : édition des objets, insertion, onglets, échanges de fichiers et impression (types C++ de L5-T1).
    ObjectEditing {
        id: appEditing
        objectName: "appEditing"
        document: appDocument
        tools: appTools
    }
    Insertion {
        id: appInsertion
        objectName: "appInsertion"
        document: appDocument
    }
    DocumentTabs {
        id: appTabs
        objectName: "appTabs"
        document: appDocument
        view: pageArea.view
        commands: appCommands // amendement 12 : onglets refusés sous verrou
    }
    FileExchange {
        id: appExchange
        objectName: "appExchange"
        document: appDocument
        tabs: appTabs
    }
    PrintController {
        id: appPrint
        objectName: "appPrint"
        document: appDocument
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
        target: penModifierBar
        function onVisibleChanged() { appCommands.setChecked("view.pen-modifiers", penModifierBar.visible) }
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
    // Coches des bascules d'objet : l'état de la sélection (verrou, groupe, masque, copie au glissement).
    Connections {
        target: appEditing
        function onSelectionStateChanged() { root.syncObjectFlags() }
    }
    function syncObjectFlags() {
        appCommands.setChecked("object.lock", appEditing.selectionLocked)
        appCommands.setChecked("object.group", appEditing.selectionIsGroup)
        appCommands.setChecked("object.hide", appEditing.selectionHidden)
        appCommands.setChecked("object.drag-copy", appEditing.selectionDragCopy)
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

    // Argument de commande donné ? (un chemin ou une adresse ; absent ou vide : ouvrir le dialogue)
    function hasArgument(arg) {
        return arg !== undefined && arg !== null && arg !== ""
    }

    // Gestionnaire de commande « argument donné : on agit ; sinon : on ouvre le dialogue ».
    function argumentOrDialog(act, openDialog) {
        return function (arg) {
            if (root.hasArgument(arg))
                act(arg)
            else
                openDialog()
        }
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
        notesVisible: browserPanel.hasNotes
        onPopupOpened: root.popupOpened()
        onPopupClosed: root.popupClosed()
    }

    // Lot 5 : onglets sous la ligne supérieure (sans hauteur tant qu'un seul document est ouvert).
    DocumentTabBar {
        id: documentTabBar
        objectName: "documentTabBar"
        tabs: appTabs
        commands: appCommands
        anchors.left: parent.left
        anchors.right: parent.right
        // Ni ancre haute ni basse : ancrées en alternance, elles étirent la barre le temps d'un changement.
        y: appTabs.position === "bottom" ? parent.height - height : topBar.y + topBar.height
        onPopupOpened: root.popupOpened()
        onPopupClosed: root.popupClosed()
    }

    RowLayout {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: appTabs.position === "bottom" ? topBar.bottom : documentTabBar.bottom
        anchors.bottom: appTabs.position === "bottom" ? documentTabBar.top : parent.bottom
        spacing: 0

        // Lot 4 : panneau des navigateurs (ancre réglée par L4-T19), à gauche de la zone de page.
        BrowserPanel {
            id: browserPanel
            objectName: "browserPanel"
            thumbnails: appThumbnails
            pages: appPages
            documentController: appDocument
            commands: appCommands
            tools: appTools
            view: pageArea.view
            profile: appProfile
            preferences: appPreferences
            onPopupOpened: root.popupOpened()
            onPopupClosed: root.popupClosed()
        }

        PageArea {
            id: pageArea
            Layout.fillWidth: true
            Layout.fillHeight: true
            cacheEnabled: root.cacheEnabled
            inputBlocked: root.openPopups > 0
            documentController: appDocument
            zones: appPageSettings.zones
            zoneFrameSize: appPageSettings.zoneFrameSize
            pages: appPages
            tools: appTools
            commands: appCommands
            objectMenu: objectEditMenu
            textEditing: appText
            textListOpen: textFormatBar.openLists + verticalTextBar.openLists > 0
            onPopupOpened: root.popupOpened()
            onPopupClosed: root.popupClosed()
        }

        // Lot 4 : palette des formes, à droite, avant la palette d'outils.
        ShapePalette {
            id: shapePalette
            objectName: "shapePalette"
            tools: appTools
            commands: appCommands
        }

        // Lot 5 : modifications du stylo et panneau du connecteur, à côté de la palette d'outils.
        PenModifierBar {
            id: penModifierBar
            objectName: "penModifierBar"
            commands: appCommands
            tools: appTools
        }
        ConnectorPanel {
            id: connectorPanel
            objectName: "connectorPanel"
            commands: appCommands
            tools: appTools
        }

        // Lot 6 : cadre de la palette (L6-T7), ancré à droite comme avant.
        PaletteFrame {
            id: paletteFrame
            Layout.fillHeight: true
            implicitHeight: 0 // la hauteur vient de la ligne, pas du contenu (sinon boucle de mise en page)
            Layout.preferredWidth: paletteFrame.slotWidth
            commands: appCommands
            documentController: appDocument
            tools: appTools
            view: pageArea.view
            profile: appProfile
            preferences: appPreferences

            ToolPalette {
                id: toolPalette
                anchors.fill: parent
                commands: appCommands
                view: pageArea.view
                tools: appTools
                documentController: appDocument
                profile: appProfile
                preferences: appPreferences
            }
        }
    }

    // Lot 4 : barres de texte sous la ligne supérieure, boîtes de la grille et du fond (talons).
    TextFormatBar {
        id: textFormatBar
        objectName: "textFormatBar"
        commands: appCommands
        editing: appText
        pageView: pageArea.view
        onPopupOpened: root.popupOpened()
        onPopupClosed: root.popupClosed()
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: appTabs.position === "bottom" ? topBar.bottom : documentTabBar.bottom
    }
    VerticalTextBar {
        id: verticalTextBar
        objectName: "verticalTextBar"
        commands: appCommands
        editing: appText
        pageView: pageArea.view
        onPopupOpened: root.popupOpened()
        onPopupClosed: root.popupClosed()
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: appTabs.position === "bottom" ? topBar.bottom : documentTabBar.bottom
    }
    GridDesigner {
        id: gridDesigner
        objectName: "gridDesigner"
        settings: appPageSettings
        onOpened: root.popupOpened()
        onClosed: root.popupClosed()
    }
    BackgroundDialog {
        id: backgroundDialog
        objectName: "backgroundDialog"
        settings: appPageSettings
        onOpened: root.popupOpened()
        onClosed: root.popupClosed()
    }

    // Lot 5 : menu d'édition d'objet, dialogues de fichiers et boîte d'impression (l'aperçu est dans la boîte).
    ObjectEditMenu {
        id: objectEditMenu
        objectName: "objectEditMenu"
        commands: appCommands
        editing: appEditing
        tools: appTools
        onPopupOpened: root.popupOpened()
        onPopupClosed: root.popupClosed()
    }
    FileExchangeActions {
        id: fileExchangeActions
        objectName: "fileExchangeActions"
        exchange: appExchange
        insertion: appInsertion
        onPopupOpened: root.popupOpened()
        onPopupClosed: root.popupClosed()
    }
    PrintSetupDialog {
        id: printSetupDialog
        objectName: "printSetupDialog"
        printing: appPrint
        exchange: appExchange
        fileExchangeActions: fileExchangeActions
        onPopupOpened: root.popupOpened()
        onPopupClosed: root.popupClosed()
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
        tabs: appTabs
        textEditing: appText
        onPopupOpened: root.popupOpened()
        onPopupClosed: root.popupClosed()
        onSaveAbandoned: documentTabBar.closeAfterSave = false
    }

    // Fermeture de la fenêtre : même demande que « Quitter ».
    onClosing: close => {
        appText.commit() // avant de lire `modified` : le texte en cours compte
        if (!fileActions.quitting && fileActions.hasUnsavedTabs()) {
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
    // Échanges de fichiers (L5-T25) : l'échec et la fin arrivent à la bannière.
    Connections {
        target: appExchange
        function onFailed(message) { messageBanner.show(message) }
        function onFinished(message) { messageBanner.show(message) }
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

    // Lot 6 : emplacements de la coquille, instanciés une fois, masqués (talons remplis par leurs tâches).
    ExtendedColors {
        id: extendedColors
        commands: appCommands
        documentController: appDocument
        tools: appTools
        view: pageArea.view
        profile: appProfile
        preferences: appPreferences
    }
    ColorLoupe {
        id: colorLoupe
        commands: appCommands
        documentController: appDocument
        tools: appTools
        view: pageArea.view
        profile: appProfile
        preferences: appPreferences
    }
    LabelTip {
        id: labelTip
        commands: appCommands
        documentController: appDocument
        tools: appTools
        view: pageArea.view
        profile: appProfile
        preferences: appPreferences
    }
    Dashboard {
        id: dashboard
        commands: appCommands
        documentController: appDocument
        tools: appTools
        view: pageArea.view
        profile: appProfile
        preferences: appPreferences
    }
    ProfileChooser {
        id: profileChooser
        commands: appCommands
        documentController: appDocument
        tools: appTools
        view: pageArea.view
        profile: appProfile
        preferences: appPreferences
    }
    ProfileEditor {
        id: profileEditor
        commands: appCommands
        documentController: appDocument
        tools: appTools
        view: pageArea.view
        profile: appProfile
        preferences: appPreferences
    }
    ClassroomLockDialog {
        id: classroomLockDialog
        commands: appCommands
        documentController: appDocument
        tools: appTools
        view: pageArea.view
        profile: appProfile
        preferences: appPreferences
    }
    SpellingDialog {
        id: spellingDialog
        commands: appCommands
        documentController: appDocument
        tools: appTools
        view: pageArea.view
        profile: appProfile
        preferences: appPreferences
    }
    HelpViewer {
        id: helpViewer
        commands: appCommands
        documentController: appDocument
        tools: appTools
        view: pageArea.view
        profile: appProfile
        preferences: appPreferences
    }
    SaveSummaryDialog {
        id: saveSummaryDialog
        commands: appCommands
        documentController: appDocument
        tools: appTools
        view: pageArea.view
        profile: appProfile
        preferences: appPreferences
    }
    SettingsPage {
        id: settingsPage
        commands: appCommands
        documentController: appDocument
        tools: appTools
        view: pageArea.view
        profile: appProfile
        preferences: appPreferences
    }
    LaunchersPage {
        id: launchersPage
        commands: appCommands
        documentController: appDocument
        tools: appTools
        view: pageArea.view
        profile: appProfile
        preferences: appPreferences
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
            // Échap appartient à la boîte ouverte (« Annuler ») : le raccourci global ne lui prend pas la touche.
            enabled: !(root.openPopups > 0 && /^esc(ape)?$/i.test(String(modelData.sequence)))
            onActivated: appCommands.trigger(modelData.id)
        }
    }

    // Les entrées d'import n'apparaissent aux menus que si elles peuvent servir.
    Connections {
        target: appExchange
        function onConverterChanged() {
            appCommands.setHidden("file.import-powerpoint-images", !appExchange.powerpointAvailable)
        }
    }

    Component.onCompleted: {
        appCommands.setHidden("file.import-pdf", !appExchange.pdfImportAvailable)
        appCommands.setHidden("file.import-powerpoint-images", !appExchange.powerpointAvailable)
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
        root.setLot5Handlers()
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

    // Lot 5 : un gestionnaire par commande, une ligne vers sa cible chacune.
    function setLot5Handlers() {
        appCommands.setHandler("edit.cut", function () { appEditing.cut() })
        appCommands.setHandler("edit.copy", function () { appEditing.copy() })
        appCommands.setHandler("edit.paste", function () { appEditing.paste() })
        appCommands.setHandler("edit.paste-onto", function () { appEditing.pasteOnto() })
        appCommands.setHandler("edit.duplicate", function () { appEditing.duplicate() })
        appCommands.setHandler("object.group", function () { appEditing.toggleGroup() })
        for (const flag of ["lock", "hide", "drag-copy"])
            appCommands.setHandler("object." + flag, function () { appEditing.toggleFlag(flag) })
        for (const how of ["front", "back", "forward", "backward", "layer-top", "layer-middle", "layer-bottom"])
            appCommands.setHandler("arrange." + how, function () { appEditing.arrange(how) })
        for (const how of ["left", "center-x", "right", "top", "center-y", "bottom", "width", "height", "angle", "all"])
            appCommands.setHandler("align." + how, function () { appEditing.align(how) })
        for (const how of ["original-size", "fit-width", "fit-height", "fit-page", "mirror-x", "mirror-y", "flip-x",
                           "flip-y", "origin-flip-x", "origin-flip-y"])
            appCommands.setHandler("transform." + how, function () { appEditing.transform(how) })
        appCommands.setHandler("object.menu", function () { objectEditMenu.popupAtSelection() })
        // L5-T17 : l'argument de l'outil (style, forme) est passé par les outils eux-mêmes.
        appCommands.setHandler("object.shape-points", function () { appTools.activate("select-points") })
        appCommands.setHandler("tool.fill", function () { appTools.activate("fill") })
        appCommands.setHandler("tool.connector", function () { appTools.activate("connector") })
        appCommands.setHandler("pen.shape", function (shape) {
            if (root.hasArgument(shape))
                appTools.activate("pen-shape", shape)
            else
                appTools.activate("pen-shape")
        })
        appCommands.setHandler("connector.style", function (rank) {
            if (root.hasArgument(rank))
                appTools.setConnectorStyle(Number(rank))
        })
        appCommands.setHandler("connector.caps", function (caps) {
            const ranks = root.hasArgument(caps) ? String(caps).split(",") : []
            if (ranks.length === 2)
                appTools.setConnectorCaps(Number(ranks[0]), Number(ranks[1]))
        })
        appCommands.setHandler("view.pen-modifiers", function () { penModifierBar.visible = !penModifierBar.visible })
        appCommands.setChecked("view.pen-modifiers", penModifierBar.visible)
        appCommands.setHandler("insert.image", root.argumentOrDialog(
            function (url) { appInsertion.insertImage(url) }, function () { fileExchangeActions.chooseImage() }))
        appCommands.setHandler("insert.media", root.argumentOrDialog(
            function (url) { appInsertion.insertMedia(url) }, function () { fileExchangeActions.chooseMedia() }))
        appCommands.setHandler("tab.next", function () { appTabs.next() })
        appCommands.setHandler("tab.previous", function () { appTabs.previous() })
        // Sans argument (le clic du menu) : position suivante ; avec argument : seulement une position connue.
        appCommands.setHandler("view.tabs", function (position) {
            const positions = ["top", "bottom", "hidden"]
            if (root.hasArgument(position)) {
                if (positions.indexOf(String(position)) >= 0)
                    appTabs.position = String(position)
            } else {
                appTabs.position = positions[(positions.indexOf(appTabs.position) + 1) % positions.length]
            }
        })
        appCommands.setHandler("export.page-image", root.argumentOrDialog(
            function (url) { appExchange.exportPageImage(url) }, function () { fileExchangeActions.chooseExportPageImage() }))
        appCommands.setHandler("export.pdf", root.argumentOrDialog(
            function (url) { appExchange.exportPdf(url) }, function () { fileExchangeActions.chooseExportPdf() }))
        appCommands.setHandler("file.import-pdf", root.argumentOrDialog(
            function (url) { appExchange.importPdf(url) }, function () { fileExchangeActions.chooseImportPdf() }))
        appCommands.setHandler("file.import-powerpoint-images", root.argumentOrDialog(
            function (url) { appExchange.importPowerpointImages(url) }, function () { fileExchangeActions.chooseImportPowerpoint() }))
        appCommands.setHandler("file.print", function () {
            appExchange.settleEditing() // l'aperçu et le PDF contiennent l'objet soulevé
            printSetupDialog.open()
        })
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
