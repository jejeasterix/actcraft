import QtQuick
// Talon L8 : rempli par L8-T12.
Item {
    visible: false
    objectName: "actionBrowser"
    property var commands: null
    Component.onCompleted: {
        if (!commands)
            return
        for (const id of ["browser.actions", "action.apply", "action.remove", "action.pick-target"])
            commands.setHandler(id, function (arg) { console.info("Talon L8 : rempli par L8-T12.") })
    }
}
