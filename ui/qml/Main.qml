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
    palette.brightText: Theme.texteSurActive
    palette.light: Theme.surfaceSurvol
    palette.midlight: Theme.surfaceSurvol
    palette.mid: Theme.surfaceSurvol
    palette.dark: Theme.surfaceActive // remplissage d'un bouton coché (L11-T24) ; son libellé : brightText
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
    // Dossier de la copie de reprise ; vide : <AppDataLocation>/reprise (StartupOptions::recoveryDir, L11-T13).
    property string recoveryDir: ""
    // Accueil au démarrage : posée à vrai par app/main.cpp seulement (StartupOptions::showDashboard) ; le joueur
    // de captures, le banc et les tests la laissent fausse.
    property bool showDashboard: false
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

    // Lot 6 (L6-T28) : le mandataire gardé s'applique dès la construction ; sans hôte saisi, ActCraft rend la main
    // à la configuration du système. La page Réseau a sa propre instance (mêmes réglages).
    NetworkSettings {
        id: appNetwork
        objectName: "appNetwork"
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

    // Lot 11 (L11-T3) : contrôleurs du lot, posés une fois ; chaque tâche suivante y ajoute son comportement.
    TouchModeController { id: appTouchMode; objectName: "appTouchMode"; commands: appCommands; preferences: appPreferences; view: pageArea.view }
    DualMode { id: appDualMode; objectName: "appDualMode"; commands: appCommands; view: pageArea.view; tools: appTools; touchMode: appTouchMode }
    LayoutMetrics { id: appLayout; objectName: "appLayout"; windowWidth: root.width; windowHeight: root.height; preferences: appPreferences }
    // L11-T10 (D11-5, D11-6) : les tailles du jeu de jetons suivent la mise en page (48 dp au doigt, jamais moins de
    // 44 ; palette élargie d'une colonne de barre de défilement).
    Binding { target: Theme; property: "tailleBouton"; value: appLayout.targetSize }
    Binding { target: Theme; property: "hauteurBarre"; value: appLayout.targetSize }
    Binding { target: Theme; property: "espaceCible"; value: appLayout.spacing }
    Binding { target: Theme; property: "touchUi"; value: appLayout.touchUi }
    Binding {
        target: Theme; property: "largeurPalette"
        value: appLayout.touchUi ? 2 * appLayout.targetSize + appLayout.spacing + 3 * Theme.espace2 : 112
    }
    LifecycleKeeper { id: appLifecycle; objectName: "appLifecycle"; document: appDocument; view: pageArea.view; tabs: appTabs; recoveryDir: root.recoveryDir }
    DocumentPortal { id: appPortal; objectName: "appPortal" }
    UpdateChecker { id: appUpdates; objectName: "appUpdates"; preferences: appPreferences }

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
        designMode: appDesign // L8-T16 : restrictions et objets bloquants du mode conception
    }

    // Lot 5 : édition des objets, insertion, onglets, échanges de fichiers et impression (types C++ de L5-T1).
    ObjectEditing {
        id: appEditing
        objectName: "appEditing"
        document: appDocument
        tools: appTools
    }
    MediaInsert {
        id: appMediaInsert
        objectName: "appMediaInsert"
        portal: appPortal
        commands: appCommands
        document: appDocument
    }

    Insertion {
        id: appInsertion
        objectName: "appInsertion"
        document: appDocument
        portal: appPortal
        mediaInsert: appMediaInsert // les médias déposés ou choisis passent par lui (action, espace réservé, verrou)
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
        portal: appPortal
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
    // L11-T9 : même chemin pour un utilisateur (étape command à champ "user" ; 0 : le premier).
    function runCommandAs(id, argument, user) {
        return appCommands.trigger(id, argument, user)
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
        touchModeSource: appTouchMode
        notesVisible: browserPanel.hasNotes
        compact: appLayout.compact // L11-C2 : menus réunis derrière le bouton « … » de la barre
        mainMenuOpened: mainMenu.opened
        onOverflowRequested: anchorItem => mainMenu.popup(anchorItem, 0, anchorItem.height)
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

        // Lot 11 : deuxième palette du mode à deux utilisateurs, au bord gauche (L11-T9).
        SecondPalette {
            id: secondPalette
            Layout.fillHeight: true
            Layout.preferredWidth: secondPalette.implicitWidth
            visible: appDualMode.active
            commands: appCommands
        }

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
            actionModel: appActionBrowser
            broadcast: appBroadcast // L8-T32 : les notes de la page sont du contenu protégé (H8-16)
            drawerHost: drawerLayer // L11-T10 : tiroir en fenêtre étroite
            drawerMode: appLayout.narrow
            stacker: appLayout
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
            narrowMode: appLayout.narrow // L11-T10 : repliée en un bouton

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

    // L11-T10 : couche du tiroir des navigateurs, sur la région de la page. Le fond (objectName drawerScrim) capte l'appui
    // hors du tiroir et le referme ; il est couverture d'entrée de la page tant que le tiroir est ouvert.
    Item {
        id: drawerLayer
        objectName: "drawerLayer"
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: appTabs.position === "bottom" ? topBar.bottom : documentTabBar.bottom
        anchors.bottom: appTabs.position === "bottom" ? documentTabBar.top : parent.bottom
        visible: appLayout.narrow
        z: 5
        Rectangle {
            anchors.fill: parent
            color: "#59000000"
            visible: browserPanel.visible
        }
        MouseArea {
            id: drawerScrim
            objectName: "drawerScrim"
            anchors.fill: parent
            enabled: browserPanel.visible
            onClicked: browserPanel.visible = false
        }
    }
    // Bouton du tiroir (fenêtre étroite, tiroir fermé) : coin haut gauche de la page.
    CommandButton {
        id: browsersButton
        commands: appCommands
        commandId: "view.browsers"
        visible: appLayout.narrow && !browserPanel.visible
        x: Theme.espace2
        y: (appTabs.position === "bottom" ? topBar.y + topBar.height : documentTabBar.y + documentTabBar.height) + Theme.espace2
        z: 6
    }
    // Le fond du tiroir n'est couverture d'entrée de la page que tiroir ouvert (fermé, le stylet doit passer).
    Connections {
        target: browserPanel
        function onVisibleChanged() {
            if (browserPanel.visible && appLayout.narrow)
                pageArea.view.addInputCover(drawerScrim)
            else
                pageArea.view.removeInputCover(drawerScrim)
        }
    }

    // Lot 11 : emplacements du bandeau de reprise, du menu au doigt et de la boîte des mises à jour (talons).
    RecoveryBanner {
        id: recoveryBanner
        commands: appCommands
        lifecycle: appLifecycle
        parent: pageArea
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.margins: Theme.espace2
        z: 11
    }
    FingerMenu {
        id: fingerMenu
        view: pageArea.view
        tools: appTools
        commands: appCommands
        objectMenu: objectEditMenu
        onPopupOpened: root.popupOpened()
        onPopupClosed: root.popupClosed()
    }
    UpdateDialog {
        id: updateDialog
        commands: appCommands
        updates: appUpdates
        onOpened: root.popupOpened()
        onClosed: root.popupClosed()
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
        onImagePicked: (url) => root.insertImageAdmitted(url)
        onPdfPicked: (url) => root.importPdfAdmitted(url)
        onPopupOpened: root.popupOpened()
        onPopupClosed: root.popupClosed()
    }
    PrintSetupDialog {
        id: printSetupDialog
        objectName: "printSetupDialog"
        printing: appPrint
        exchange: appExchange
        fileExchangeActions: fileExchangeActions
        commands: appCommands
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
                recents: fileActions.recents
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

    // L8-T9 : exécuteur des actions d'objet, devant l'outil de sélection (appTools.setActionRunner) ; exécute au
    // relâché d'un clic, après la retombée du contact ; porte l'historique des pages (page.history-*).
    ActionRunner {
        id: appActionRunner
        objectName: "appActionRunner"
        commands: appCommands
        contactActive: pageArea.view ? pageArea.view.contactActive : false
        document: appDocument
        designMode: appDesign
        links: appLinks
        mediaItems: appMediaItems // F-552 : « jouer un son » passe par le lecteur intégré
        Component.onCompleted: appTools.setActionRunner(appActionRunner)
    }
    // L8-T25 : médias de la page affichée (vidéo et GIF en surcouche, lecteur intégré de deux pistes).
    MediaItems {
        id: appMediaItems
        objectName: "appMediaItems"
        document: appDocument
        contactActive: pageArea.view ? pageArea.view.contactActive : false
        links: appLinks
        tools: appTools
        // La couche des médias est posée par PageArea (pageOverlays, z 4) : elle reçoit ici son modèle et sa vue.
        Component.onCompleted: {
            pageArea.view.addInputCover(browsersButton)
            for (const layer of pageArea.pageOverlays.children) {
                if (layer.objectName === "mediaLayer") {
                    layer.view = pageArea.view
                    layer.items = appMediaItems
                }
            }
        }
    }
    LinkLauncher { // L8-T10 : seul chemin d'un lien ou d'un fichier de document vers le système (D8-4)
        id: appLinks
        objectName: "appLinks"
    }
    ActionBrowserModel { // L8-T11 : onglet « Sélection » du navigateur d'actions
        id: appActionBrowser
        objectName: "appActionBrowser"
        commands: appCommands
        contactActive: pageArea.view.contactActive
        document: appDocument
        toolHost: appTools
    }
    DragDropActionsModel { // L8-T13 : actions par glisser-déposer (F-892)
        id: appDragDrop
        objectName: "appDragDrop"
        commands: appCommands
        document: appDocument
    }
    DragDropActions { // plateau d'icônes à côté du panneau des navigateurs
        objectName: "dragDropActions"
        model: appDragDrop
        panel: browserPanel
        view: pageArea.view
    }
    LinkDialogs { // L8-T22 : création des liens vers un fichier ou un site (F-550, F-551)
        id: appLinkDialogs
        objectName: "appLinkDialogs"
        commands: appCommands
        document: appDocument
        toolHost: appTools
        links: appLinks
    }
    LinkDialog {
        id: linkDialog
        commands: appCommands
        controller: appLinkDialogs
        onOpened: root.popupOpened()
        onClosed: root.popupClosed()
    }
    LinkConfirmDialog {
        objectName: "linkConfirmDialog"
        launcher: appLinks
        onOpened: root.popupOpened()
        onClosed: root.popupClosed()
    }
    WebFrameDialog { objectName: "webFrameDialog" }
    PublishDialog {
        id: publishDialog
        commands: appCommands
        document: appDocument
        banner: messageBanner
        onPopupOpened: root.popupOpened()
        onPopupClosed: root.popupClosed()
    }
    // L8-T32 : diffusion protégée, porte du registre. Les règles du document ouvert grisent et refusent les
    // commandes de leurs catégories (verrou de classe compris), la copie du champ de texte aussi ; un essai
    // refusé s'affiche dans la bannière.
    BroadcastGate {
        id: appBroadcast
        objectName: "appBroadcast"
        commands: appCommands
        document: appDocument
        textEditing: appText
        onRefused: (id, message) => messageBanner.show(message)
    }
    SoundController {
        id: soundController
        commands: appCommands
        items: appMediaItems
        view: pageArea.view
    }
    RecorderPalette {
        id: recorderPalette
        commands: appCommands
        document: appDocument
        mediaInsert: appMediaInsert
        preferences: appPreferences
    }
    RecordingIndicator { recorders: recorderPalette.recorders }
    AreaSelector { recorders: recorderPalette.recorders; view: pageArea.view }

    AboutDialog {
        id: aboutDialog
        onOpened: root.popupOpened()
        onClosed: root.popupClosed()
    }

    FileActions {
        id: fileActions
        objectName: "fileActions"
        document: appDocument
        tabs: appTabs
        textEditing: appText
        preferences: appPreferences // documents récents gardés (L6-T24)
        portal: appPortal
        onNotice: (message) => messageBanner.show(message)
        broadcast: appBroadcast // L8-T32 : « Enregistrer les modifications ? » refusé dans un document protégé
        commands: appCommands // L8-C7 : « Enregistrer » de Quitter passe par file.save, comme la croix d'un onglet
        onPopupOpened: root.popupOpened()
        onPopupClosed: root.popupClosed()
        onSaveAbandoned: documentTabBar.closeAfterSave = false
        onSaved: (url) => {
            if (appProfile.setting("document", "summary-on-save", false))
                saveSummaryDialog.show(fileActions.recents.summary(appDocument, url))
        }
    }

    // Fermeture de la fenêtre : même demande que « Quitter ».
    onClosing: close => {
        appText.commit() // avant de lire `modified` : le texte en cours compte
        if (!fileActions.quitting && fileActions.hasUnsavedTabs()) {
            close.accepted = false
            fileActions.quit()
            return
        }
        // Fermeture acceptée (tout est enregistré, ou « Ne pas enregistrer » choisi) : la copie de reprise n'a
        // plus d'objet et n'est pas rouverte au lancement suivant (L11-T13).
        appLifecycle.discard()
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
    // L8-C7 : les refus de l'exécuteur d'actions, du navigateur d'actions et du glisser-déposer, et l'échec d'une
    // insertion de média, arrivent à la bannière.
    Connections {
        target: appActionRunner
        function onActionRefused(reason) { messageBanner.show(reason) }
    }
    Connections {
        target: appActionBrowser
        function onRefused(reason) { messageBanner.show(reason) }
    }
    Connections {
        target: appDragDrop
        function onRefused(reason) { messageBanner.show(reason) }
    }
    // Refus asynchrone d'un fichier rangé (illisible, vide, trop gros, verrou) : la boîte est déjà fermée, la
    // bannière le dit. Boîte ouverte, son propre message d'erreur suffit.
    Connections {
        target: appLinkDialogs
        function onRefused(reason) {
            if (!linkDialog.visible)
                messageBanner.show(reason)
        }
    }
    Connections {
        target: appMediaInsert
        function onFailed(message) { messageBanner.show(message) }
    }
    // D11-9 : un document refusé par le portail n'est pas lu ; le message donne son nom, jamais son URL.
    Connections {
        target: appPortal
        function onRefused(name) {
            messageBanner.show(qsTr("« %1 » n'est plus accessible : l'accès au document est perdu ou le fournisseur ne répond pas.").arg(name))
        }
    }
    // Permissions prêtées pour une lecture unique (image, PDF) : rendues quand plus aucune lecture ne court.
    function returnLentPermissions() {
        if (!appInsertion.busy && !appExchange.busy && !appMediaInsert.busy)
            appPortal.returnLent()
    }
    Connections {
        target: appInsertion
        function onBusyChanged() { root.returnLentPermissions() }
    }
    Connections {
        target: appExchange
        function onBusyChanged() { root.returnLentPermissions() }
    }
    Connections {
        target: appMediaInsert
        function onBusyChanged() { root.returnLentPermissions() }
    }
    // Admission d'un fichier choisi, puis lecture ; un refus rend les permissions prêtées si plus rien ne lit.
    function admitThen(url, read) {
        if (appPortal.admit(url))
            read(url)
        else
            root.returnLentPermissions()
    }
    function insertImageAdmitted(url) { root.admitThen(url, function(u) { appInsertion.insertImage(u) }) }
    function importPdfAdmitted(url) { root.admitThen(url, function(u) { appExchange.importPdf(u) }) }
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
        recents: fileActions.recents
        showDashboard: root.showDashboard
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
        recents: fileActions.recents
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
        appCommands.setHandler("tool.pen", function (argument, user) { appTools.activateFor("pen", user) })
        // La vue n'a plus de couleur propre : appTools la tient à jour (inkColor, inkWidth).
        appCommands.setHandler("pen.color", function (color, user) { appTools.setColor(color, user) })
        appCommands.setHandler("pen.width", function (width, user) { appTools.setWidth(width, user) })
        // Échap (M9) : abandon du geste en cours ; au repos, outil Sélectionner. Jamais livré à l'outil.
        appCommands.setHandler("tool.select", function (argument, user) {
            if (user) { // L11-T9 : le second utilisateur n'a pas d'Échap ; son outil seul change
                appTools.activateFor("select", user)
                return
            }
            appTools.cancelGesture()
            appTools.activate("select")
        })
        appCommands.setHandler("tool.highlighter", function (argument, user) { appTools.activateFor("highlighter", user) })
        appCommands.setHandler("tool.eraser", function (argument, user) { appTools.activateFor("eraser", user) })
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
        appCommands.setHandler("tool.shape", function (argument, user) { appTools.activateFor("shape", user) })
        appCommands.setHandler("shape.choose", function (preset) { appTools.chooseShape(preset) })
        appCommands.setHandler("shape.filled", function () { appTools.setShapeFilled(!appTools.shapeFilled) })
        appCommands.setHandler("shape.fill-color", function (color) { appTools.setShapeFillColor(color) })
        appCommands.setHandler("shape.draw-from-center", function () { appTools.setShapeFromCenter(!appTools.shapeFromCenter) })
        appCommands.setHandler("tool.text", function (argument, user) { appTools.activateFor("text", user) })
        appCommands.setHandler("tool.text-vertical", function (argument, user) { appTools.activateFor("text-vertical", user) })
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
        appCommands.setHandler("page.first", function () { appDocument.showPage(0) })
        appCommands.setHandler("page.last", function () { appDocument.showPage(appDocument.pageCount - 1) })
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
        appCommands.setHandler("tool.fill", function (argument, user) { appTools.activateFor("fill", user) })
        appCommands.setHandler("tool.connector", function (argument, user) { appTools.activateFor("connector", user) })
        appCommands.setHandler("pen.shape", function (shape) {
            if (root.hasArgument(shape))
                appTools.activate("pen-shape", shape)
            else
                appTools.activate("pen-shape")
        })
        appCommands.setHandler("connector.style", function (rank) {
            if (root.hasArgument(rank))
                appTools.chooseConnectorStyle(Number(rank)) // H5-33 : règle aussi les extrémités
        })
        appCommands.setHandler("connector.caps", function (caps) {
            const ranks = root.hasArgument(caps) ? String(caps).split(",") : []
            if (ranks.length === 2)
                appTools.setConnectorCaps(Number(ranks[0]), Number(ranks[1]))
        })
        appCommands.setHandler("view.pen-modifiers", function () { penModifierBar.visible = !penModifierBar.visible })
        appCommands.setChecked("view.pen-modifiers", penModifierBar.visible)
        appCommands.setHandler("insert.image", root.argumentOrDialog(
            function (arg) { root.insertImageAdmitted(appDocument.urlFromArgument(arg)) },
            function () { fileExchangeActions.chooseImage() }))
        appCommands.setHandler("insert.media", root.argumentOrDialog(
            function (url) { appMediaInsert.insertFile(url) }, function () { fileExchangeActions.chooseMedia() }))
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
            function (url) { root.importPdfAdmitted(url) }, function () { fileExchangeActions.chooseImportPdf() }))
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
