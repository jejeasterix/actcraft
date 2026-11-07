import QtQuick
import ActCraft.Ui

// Menu au doigt (D11-7) : un doigt tenu immobile ouvre, avec l'outil de sélection, le menu de l'objet sous le doigt
// (menu d'édition d'objet) ou, hors de tout objet, le menu de la page. Sans logique propre : FingerMenus décide,
// ce composant ouvre. `visible` dit qu'un de ces menus est ouvert.
Item {
    id: menu
    objectName: "fingerMenu"
    property PageView view: null
    property Tools tools: null
    property Commands commands: null
    // Menu d'édition d'objet (ObjectEditMenu) de la fenêtre.
    property var objectMenu: null
    readonly property string kind: logic.kind
    // Recul du menu par rapport au doigt : le doigt tenu ne doit pas se retrouver sur une entrée au relâché.
    readonly property real offset: Theme.tailleBouton / 2
    signal popupOpened()
    signal popupClosed()
    visible: menu.kind !== ""

    FingerMenus {
        id: logic
        view: menu.view
        tools: menu.tools
        onObjectMenuRequested: windowPoint => {
            if (menu.objectMenu)
                menu.objectMenu.popupAtItem(null, windowPoint.x + menu.offset, windowPoint.y + menu.offset)
        }
        onPageMenuRequested: windowPoint => {
            const at = menu.mapFromItem(null, windowPoint.x + menu.offset, windowPoint.y + menu.offset)
            pageMenu.popupAt(at.x, at.y)
        }
    }
    Connections {
        target: menu.objectMenu
        ignoreUnknownSignals: true
        function onPopupClosed() { logic.reset() }
    }
    PageContextMenu {
        id: pageMenu
        objectName: "fingerPageMenu"
        commands: menu.commands
        onOpened: menu.popupOpened()
        onClosed: {
            menu.popupClosed()
            logic.reset()
        }
    }
}
