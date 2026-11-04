// Talon L6 : rempli par L6-T30.
import QtQuick
import ActCraft.Ui

Item {
    id: root
    objectName: "launchersPage"
    property Commands commands: null
    property DocumentController documentController: null
    property Tools tools: null
    property PageView view: null
    property var profile: null
    property var preferences: null
    visible: false

    Component.onCompleted: {
        // Talon L6 : rempli par L6-T30.
        root.commands.setHandler("launcher.run", function () { console.info("launcher.run") })
        // Talon L6 : rempli par L6-T30.
        root.commands.setHandler("launcher.edit", function () { console.info("launcher.edit") })
    }
}
