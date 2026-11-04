// Talon L6 : rempli par L6-T28.
import QtQuick
import ActCraft.Ui

Item {
    id: root
    objectName: "networkPage"
    property Commands commands: null
    property DocumentController documentController: null
    property Tools tools: null
    property PageView view: null
    property var profile: null
    property var preferences: null
    visible: false
}
