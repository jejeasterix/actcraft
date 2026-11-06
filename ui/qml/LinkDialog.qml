import QtQuick
// Talon L8 : rempli par L8-T22.
Item {
    visible: false
    objectName: "linkDialog"
    property var commands: null
    Component.onCompleted: {
        if (!commands)
            return
        for (const id of ["link.file", "link.website"])
            commands.setHandler(id, function (arg) { console.info("Talon L8 : rempli par L8-T22.") })
    }
}
