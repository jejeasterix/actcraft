// Talon L6 : rempli par L6-T25.
import QtQuick
import ActCraft.Ui

Item {
    id: root
    objectName: "formatPage"
    property Commands commands: null
    property DocumentController documentController: null
    property Tools tools: null
    property PageView view: null
    property var profile: null
    property var preferences: null
    visible: false
}
