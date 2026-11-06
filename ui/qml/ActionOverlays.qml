import QtQuick
// Surcouches des actions du lot 8 et gestionnaires des commandes qu'aucun autre fichier ne porte.
Item {
    id: overlays
    visible: false
    objectName: "actionOverlays"
    property var commands: null
    // Outils de l'application ; à défaut, ceux de la zone de page qui porte cette surcouche (lus à l'appel).
    property var tools: null

    function toolHost() {
        if (tools)
            return tools
        for (let p = parent; p; p = p.parent) {
            if (p.tools)
                return p.tools
        }
        return null
    }

    Component.onCompleted: {
        if (!commands)
            return
        // Lot 8 (L8-T18, F-255) : encre révélatrice.
        commands.setHandler("tool.reveal-ink", function () {
            const host = overlays.toolHost()
            if (host)
                host.activate("reveal-ink")
        })
        for (const id of ["page.history-back", "page.history-forward"])
            commands.setHandler(id, function (arg) { console.info("Talon L8 : rempli par L8-T9.") })
    }
}
