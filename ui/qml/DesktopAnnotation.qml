import QtQuick
import ActCraft.Ui

// Talon L7-S8 : rempli par L7-T30.
Item {
    id: root
    visible: false
    property Commands commands: null
    property DocumentController documentController: null
    property Tools tools: null
    property Item pageArea: null
    property QtObject controller: null // la tâche y lie son contrôleur : controller: curtainController
}
