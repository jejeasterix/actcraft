import QtQuick
// Talon L8 : rempli par L8-T28.
Item {
    visible: false
    objectName: "recorderPalette"
    property var commands: null
    Component.onCompleted: {
        if (!commands)
            return
        for (const id of ["record.sound", "record.pause", "record.stop", "record.cancel", "record.screen-full", "record.screen-area"])
            commands.setHandler(id, function (arg) { console.info("Talon L8 : rempli par L8-T28.") })
    }
}
