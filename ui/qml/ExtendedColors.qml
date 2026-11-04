// Talon L6 : rempli par L6-T8.
import QtQuick
import ActCraft.Ui

Item {
    id: root
    objectName: "extendedColors"
    property Commands commands: null
    property DocumentController documentController: null
    property Tools tools: null
    property PageView view: null
    property var profile: null
    property var preferences: null
    visible: false

    Component.onCompleted: {
        // Talon L6 : rempli par L6-T8.
        root.commands.setHandler("color.more", function () { console.info("color.more") })
        // Talon L6 : rempli par L6-T8.
        root.commands.setHandler("color.pick", function () { console.info("color.pick") })
        // Talon L6 : rempli par L6-T8.
        root.commands.setHandler("color.dialog", function () { console.info("color.dialog") })
        // Talon L6 : rempli par L6-T8.
        root.commands.setHandler("color.add", function () { console.info("color.add") })
    }
}
