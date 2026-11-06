import QtQuick
// Talon L8 : rempli par L8-T18.
Item {
    visible: false
    objectName: "actionOverlays"
    property var commands: null
    Component.onCompleted: {
        if (!commands)
            return
        for (const id of ["tool.reveal-ink"])
            commands.setHandler(id, function (arg) { console.info("Talon L8 : rempli par L8-T18.") })
        for (const id of ["page.history-back", "page.history-forward"])
            commands.setHandler(id, function (arg) { console.info("Talon L8 : rempli par L8-T9.") })
    }
}
