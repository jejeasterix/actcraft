import QtQuick
// Talon L8 : rempli par L8-T25.
Item {
    visible: false
    objectName: "soundController"
    property var commands: null
    Component.onCompleted: {
        if (!commands)
            return
        for (const id of ["media.sound-controller"])
            commands.setHandler(id, function (arg) { console.info("Talon L8 : rempli par L8-T25.") })
    }
}
