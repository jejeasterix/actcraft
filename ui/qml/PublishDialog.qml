import QtQuick
// Talon L8 : rempli par L8-T31.
Item {
    visible: false
    objectName: "publishDialog"
    property var commands: null
    Component.onCompleted: {
        if (!commands)
            return
        for (const id of ["file.publish"])
            commands.setHandler(id, function (arg) { console.info("Talon L8 : rempli par L8-T31.") })
    }
}
