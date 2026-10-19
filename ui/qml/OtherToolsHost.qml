import QtQuick
import ActCraft.Ui

// Talon L7-S8 : rempli par L7-T17.
Item {
    id: root
    property Commands commands: null
    property DocumentController documentController: null
    property Tools tools: null
    property Item pageArea: null
    property QtObject controller: null // la tâche y lie son contrôleur : controller: curtainController
}
