import QtQuick
import ActCraft.Ui

// Menu d'édition d'objet (F-770, D20) : les entrées de commands.objectMenu, rendues comme un menu de la barre
// (sous-menus Transformer, Aligner, Organiser). Les deux entrées « Convertir » ne s'offrent que pour une
// annotation (editing.selectionIsAnnotation) et « Convertir en texte » seulement sous Windows. Les bascules
// (verrou, groupe, masque, copie au glissement) lisent leur coche dans le registre. Ouvert par object.menu
// (popupAtSelection), par le bouton de la barre d'objet et par le clic droit sur un objet (popupAtItem). Le
// contrat du lot 4 s'applique : popupOpened et popupClosed sont émis à l'ouverture et à la fermeture.
Item {
    id: root
    visible: false

    required property Commands commands
    required property ObjectEditing editing
    required property Tools tools
    signal popupOpened()
    signal popupClosed()

    // Entrées visibles, séparateurs de tête, de queue et doublés retirés.
    readonly property var entries: root.computeEntries()
    // Commandes visibles, séparateurs exclus ; un sous-menu compte pour une.
    readonly property int visibleCommandCount: root.countCommands(root.entries)
    readonly property bool isOpen: menu.opened

    function convertHidden(id) {
        if (id === "recognition.convert-to-shape")
            return !root.editing.selectionIsAnnotation
        if (id === "recognition.convert-to-text")
            return !root.editing.selectionIsAnnotation || root.commands.platformName !== "windows"
        return false
    }

    function computeEntries() {
        const out = []
        const all = root.commands.objectMenu
        for (let i = 0; i < all.length; ++i) {
            const item = all[i]
            const separator = item.separator === true
            if (!separator && item.submenu !== true && root.convertHidden(String(item.id)))
                continue
            if (separator && (out.length === 0 || out[out.length - 1].separator === true))
                continue
            out.push(item)
        }
        while (out.length > 0 && out[out.length - 1].separator === true)
            out.pop()
        return out
    }

    function countCommands(list) {
        let n = 0
        for (let i = 0; i < list.length; ++i) {
            if (list[i].separator !== true)
                ++n
        }
        return n
    }

    function open(x, y) {
        menu.spec = { id: "", label: "", items: root.entries }
        menu.popup(x, y)
    }

    // Ouvre le menu au point (x, y) d'un élément quelconque (clic droit de la vue).
    function popupAtItem(source, x, y) {
        const at = root.mapFromItem(source, x, y)
        root.open(at.x, at.y)
    }

    // Ouvre le menu au centre de la sélection (object.menu).
    function popupAtSelection() {
        const frame = root.tools.selectionFrame
        const view = root.tools.view
        if (!frame || !frame.corners || frame.corners.length < 4 || !view)
            return
        let cx = 0
        let cy = 0
        for (let i = 0; i < 4; ++i) {
            cx += frame.corners[i].x / 4
            cy += frame.corners[i].y / 4
        }
        const scene = view.pageToScene(Qt.point(cx, cy))
        const at = root.mapFromItem(null, scene.x, scene.y)
        root.open(at.x, at.y)
    }

    CommandMenu {
        id: menu
        objectName: "objectMenu"
        commands: root.commands
        onOpened: root.popupOpened()
        onClosed: root.popupClosed()
    }
}
