// Talon L6 : rempli par L6-T9.
import QtQuick
import ActCraft.Ui

Item {
    id: root
    objectName: "shortcutBar"
    property Commands commands: null
    property DocumentController documentController: null
    property Tools tools: null
    property PageView view: null
    property var profile: null
    property var preferences: null
    visible: false

    Component.onCompleted: {
        // Talon L6 : rempli par L6-T9.
        root.commands.setHandler("shortcutbar.insert", function () { console.info("shortcutbar.insert") })
    }
}
