// Talon L6 : rempli par L6-T31.
import QtQuick
import ActCraft.Ui

Item {
    id: root
    objectName: "classroomLockDialog"
    property Commands commands: null
    property DocumentController documentController: null
    property Tools tools: null
    property PageView view: null
    property var profile: null
    property var preferences: null
    visible: false

    Component.onCompleted: {
        // Talon L6 : rempli par L6-T31.
        root.commands.setHandler("classroom.lock", function () { console.info("classroom.lock") })
        // Talon L6 : rempli par L6-T31.
        root.commands.setHandler("classroom.password", function () { console.info("classroom.password") })
    }
}
