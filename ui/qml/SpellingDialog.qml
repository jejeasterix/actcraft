// Talon L6 : rempli par L6-T34.
import QtQuick
import ActCraft.Ui

Item {
    id: root
    objectName: "spellingDialog"
    property Commands commands: null
    property DocumentController documentController: null
    property Tools tools: null
    property PageView view: null
    property var profile: null
    property var preferences: null
    visible: false

    Component.onCompleted: {
        // Talon L6 : rempli par L6-T34.
        root.commands.setHandler("spell.check", function () { console.info("spell.check") })
    }
}
