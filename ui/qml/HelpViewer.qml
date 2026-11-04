// Talon L6 : rempli par L6-T35.
import QtQuick
import ActCraft.Ui

Item {
    id: root
    objectName: "helpViewer"
    property Commands commands: null
    property DocumentController documentController: null
    property Tools tools: null
    property PageView view: null
    property var profile: null
    property var preferences: null
    visible: false

    Component.onCompleted: {
        // Talon L6 : rempli par L6-T35.
        root.commands.setHandler("help.contents", function () { console.info("help.contents") })
    }
}
