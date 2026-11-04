// Talon L6 : rempli par L6-T21.
import QtQuick
import ActCraft.Ui

Item {
    id: root
    objectName: "resourceBrowser"
    property Commands commands: null
    property DocumentController documentController: null
    property Tools tools: null
    property PageView view: null
    property var profile: null
    property var preferences: null
    visible: false

    Component.onCompleted: {
        // Talon L6 : rempli par L6-T21.
        root.commands.setHandler("resources.mine", function () { console.info("resources.mine") })
        // Talon L6 : rempli par L6-T21.
        root.commands.setHandler("resources.shared", function () { console.info("resources.shared") })
        // Talon L6 : rempli par L6-T21.
        root.commands.setHandler("resources.other", function () { console.info("resources.other") })
        // Talon L6 : rempli par L6-T21.
        root.commands.setHandler("resources.insert", function () { console.info("resources.insert") })
        // Talon L6 : rempli par L6-T21.
        root.commands.setHandler("resources.add", function () { console.info("resources.add") })
        // Talon L6 : rempli par L6-T21.
        root.commands.setHandler("resources.stamp", function () { console.info("resources.stamp") })
        // Talon L6 : rempli par L6-T21.
        root.commands.setHandler("resources.new-folder", function () { console.info("resources.new-folder") })
        // Talon L6 : rempli par L6-T21.
        root.commands.setHandler("resources.rename", function () { console.info("resources.rename") })
        // Talon L6 : rempli par L6-T21.
        root.commands.setHandler("resources.delete", function () { console.info("resources.delete") })
    }
}
