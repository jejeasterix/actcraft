// Talon L6 : rempli par L6-T27.
import QtQuick
import ActCraft.Ui

Item {
    id: root
    objectName: "settingsPage"
    property Commands commands: null
    property DocumentController documentController: null
    property Tools tools: null
    property PageView view: null
    property var profile: null
    property var preferences: null
    visible: false

    Component.onCompleted: {
        // Talon L6 : rempli par L6-T27.
        root.commands.setHandler("settings.language", function () { console.info("settings.language") })
    }
}
