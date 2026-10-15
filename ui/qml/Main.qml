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
    title: qsTr("ActCraft — rendu : %1").arg(root.apiName(GraphicsInfo.api))
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
        // reste sur sa page.
        onPageAboutToChange: pageArea.view.finishStroke()
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

        PageArea {
            id: pageArea
            Layout.fillWidth: true
            Layout.fillHeight: true
            cacheEnabled: root.cacheEnabled
            inputBlocked: root.openPopups > 0
        }

        ToolPalette {
            id: toolPalette
            Layout.fillHeight: true
            Layout.preferredWidth: Theme.largeurPalette
            commands: appCommands
            view: pageArea.view
        }
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
        appCommands.setHandler("file.quit", function () { Qt.quit() })
        appCommands.setHandler("view.fullscreen", function () { root.setFullScreen(!root.fullScreen) })
        appCommands.setHandler("view.fit", function () { pageArea.view.fitPage() })
        appCommands.setHandler("view.zoom", function (percent) { pageArea.view.zoom = percent / 100 })
        appCommands.setHandler("page.next", function () { appDocument.nextPage() })
        appCommands.setHandler("page.previous", function () { appDocument.previousPage() })
        appCommands.setHandler("tool.pen", function () { appCommands.setChecked("tool.pen", true) })
        appCommands.setHandler("pen.color", function (color) { pageArea.view.penColor = color })
        appCommands.setHandler("pen.width", function (width) { pageArea.view.penWidth = width })
        appCommands.setHandler("menu.main", function () { mainMenu.popup() })
        appCommands.setHandler("menu.tools", function () { toolsMenu.popup() })
        appCommands.setHandler("help.about", function () { aboutDialog.open() })
        appCommands.setHandler("help.website", function () { Qt.openUrlExternally(AppInfo.projectUrl) })
        appCommands.setChecked("tool.pen", true) // F-200 : le stylo est l'outil de départ
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
