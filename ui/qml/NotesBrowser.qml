// Talon L6 : rempli par L6-T16.
import QtQuick
import ActCraft.Ui

Item {
    id: root
    objectName: "notesBrowser"
    property Commands commands: null
    property DocumentController documentController: null
    property Tools tools: null
    property PageView view: null
    property var profile: null
    property var preferences: null
    property bool hasNotes: false
    visible: false
}
