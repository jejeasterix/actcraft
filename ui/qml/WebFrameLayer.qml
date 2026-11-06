import QtQuick
// Talon L8 : rempli par L8-T30.
Item {
    visible: false
    objectName: "webFrameLayer"
    property var commands: null
    Component.onCompleted: {
        if (!commands)
            return
        for (const id of ["insert.web-frame", "webframe.back", "webframe.forward", "webframe.reload", "webframe.snapshot", "webframe.unblock"])
            commands.setHandler(id, function (arg) { console.info("Talon L8 : rempli par L8-T30.") })
    }
}
