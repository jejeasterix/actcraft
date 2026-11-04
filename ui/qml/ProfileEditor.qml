// Talon L6 : rempli par L6-T25.
import QtQuick
import ActCraft.Ui

Item {
    id: root
    objectName: "profileEditor"
    property Commands commands: null
    property DocumentController documentController: null
    property Tools tools: null
    property PageView view: null
    property var profile: null
    property var preferences: null
    visible: false

    Component.onCompleted: {
        // Talon L6 : rempli par L6-T25.
        root.commands.setHandler("profile.choose", function () { console.info("profile.choose") })
        // Talon L6 : rempli par L6-T25.
        root.commands.setHandler("profile.edit", function () { console.info("profile.edit") })
        // Talon L6 : rempli par L6-T25.
        root.commands.setHandler("profile.save-as", function () { console.info("profile.save-as") })
        // Talon L6 : rempli par L6-T25.
        root.commands.setHandler("profile.import", function () { console.info("profile.import") })
        // Talon L6 : rempli par L6-T25.
        root.commands.setHandler("profile.export", function () { console.info("profile.export") })
        // Talon L6 : rempli par L6-T25.
        root.commands.setHandler("profile.delete", function () { console.info("profile.delete") })
        // Talon L6 : rempli par L6-T25.
        root.commands.setHandler("profile.reset", function () { console.info("profile.reset") })
    }
}
