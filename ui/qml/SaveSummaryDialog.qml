// Talon L6 : rempli par L6-T24.
import QtQuick
import ActCraft.Ui

Item {
    id: root
    objectName: "saveSummaryDialog"
    property Commands commands: null
    property DocumentController documentController: null
    property Tools tools: null
    property PageView view: null
    property var profile: null
    property var preferences: null
    visible: false

    Component.onCompleted: {
        // Talon L6 : rempli par L6-T24.
        root.commands.setHandler("file.recent-clear", function () { console.info("file.recent-clear") })
    }
}
