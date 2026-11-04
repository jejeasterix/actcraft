// Talon L6 : rempli par L6-T23.
import QtQuick
import ActCraft.Ui

Item {
    id: root
    objectName: "dashboard"
    property Commands commands: null
    property DocumentController documentController: null
    property Tools tools: null
    property PageView view: null
    property var profile: null
    property var preferences: null
    visible: false

    Component.onCompleted: {
        // Talon L6 : rempli par L6-T23.
        root.commands.setHandler("view.dashboard", function () { console.info("view.dashboard") })
    }
}
