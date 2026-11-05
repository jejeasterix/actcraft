import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import ActCraft.Ui

// Vérification de l'orthographe (F-297, F7) : le mot en rouge, ses suggestions, un champ de remplacement et les
// boutons ignorer, ignorer toujours, ajouter au dictionnaire, remplacer, remplacer tout, fermer. La page du mot
// devient la page affichée, le mot est surligné. Raccourcis dans la boîte : Entrée remplace, Échap ferme,
// Alt+I ignore, Alt+T ignore toujours, Alt+A ajoute au dictionnaire, Alt+R remplace tout.
Item {
    id: root
    objectName: "spellingDialog"
    property Commands commands: null
    property DocumentController documentController: null
    property Tools tools: null
    property PageView view: null
    property var profile: null
    property var preferences: null
    readonly property bool opened: popup.opened
    // Lectures des scénarios.
    readonly property string word: appSpelling.word
    readonly property string suggestion0: appSpelling.suggestions.length > 0 ? appSpelling.suggestions[0] : ""
    readonly property string message: appSpelling.message
    visible: false

    Spelling {
        id: appSpelling
        objectName: "appSpelling"
        document: root.documentController
        profile: root.profile
        contactActive: root.profile ? root.profile.contactActive : false
    }

    // Le mot, surligné sur la page (coordonnées de page → fenêtre).
    Rectangle {
        id: marker
        objectName: "spellingHighlight"
        parent: Overlay.overlay
        readonly property rect box: appSpelling.highlight
        readonly property point topLeft: root.view && appSpelling.running
                                         ? (root.view.zoom + root.view.pan.x + root.view.pan.y >= 0
                                            ? root.view.pageToScene(Qt.point(box.x, box.y)) : Qt.point(0, 0))
                                         : Qt.point(0, 0)
        readonly property point bottomRight: root.view && appSpelling.running
                                             ? (root.view.zoom + root.view.pan.x + root.view.pan.y >= 0
                                                ? root.view.pageToScene(Qt.point(box.x + box.width, box.y + box.height))
                                                : Qt.point(0, 0))
                                             : Qt.point(0, 0)
        visible: appSpelling.running && box.width > 0 && popup.opened
        x: topLeft.x - 2
        y: topLeft.y - 2
        width: bottomRight.x - topLeft.x + 4
        height: bottomRight.y - topLeft.y + 4
        radius: 3
        color: "#66FFD54F"
        border.width: 2
        border.color: "#E0A800"
    }

    Popup {
        id: popup
        objectName: "spellingPopup"
        parent: Overlay.overlay
        x: Math.round((parent.width - width) / 2)
        y: parent.height - height - 24
        modal: false
        dim: false
        padding: 12
        closePolicy: Popup.NoAutoClose
        onOpened: replacement.forceActiveFocus()
        onClosed: appSpelling.close()
        background: Rectangle {
            color: Theme.palette
            radius: Theme.rayon
            border.width: 1
            border.color: Theme.contour
        }
        contentItem: ColumnLayout {
            spacing: 8
            Label {
                objectName: "spellingTitle"
                Layout.fillWidth: true
                visible: !appSpelling.finished
                text: appSpelling.repeated ? qsTr("Mot répété") : qsTr("Mot inconnu")
                color: Theme.texteDiscret
            }
            Label {
                objectName: "spellingWord"
                Layout.fillWidth: true
                visible: !appSpelling.finished
                text: appSpelling.word
                font.bold: true
                font.pixelSize: Theme.taillePoliceTitre
                color: "#FF6B6B"
            }
            Flow {
                Layout.fillWidth: true
                Layout.preferredWidth: 420
                spacing: 6
                visible: !appSpelling.finished
                Repeater {
                    model: appSpelling.suggestions
                    delegate: Button {
                        required property string modelData
                        objectName: "spellingSuggestion-" + modelData
                        text: modelData
                        onClicked: replacement.text = modelData
                    }
                }
            }
            TextField {
                id: replacement
                objectName: "spellingReplacement"
                Layout.fillWidth: true
                visible: !appSpelling.finished
                placeholderText: qsTr("Remplacer par")
                onAccepted: appSpelling.replace(text)
            }
            Label {
                objectName: "spellingEnd"
                Layout.fillWidth: true
                visible: appSpelling.finished
                text: appSpelling.message
                font.bold: true
                color: Theme.texte
            }
            RowLayout {
                spacing: 6
                visible: !appSpelling.finished
                Button { objectName: "spellingIgnore"; text: qsTr("Ignorer"); onClicked: appSpelling.ignore() }
                Button { objectName: "spellingIgnoreAlways"; text: qsTr("Ignorer toujours"); onClicked: appSpelling.ignoreAlways() }
                Button { objectName: "spellingAdd"; text: qsTr("Ajouter au dictionnaire"); onClicked: appSpelling.addToDictionary() }
            }
            RowLayout {
                spacing: 6
                Button {
                    objectName: "spellingReplace"
                    visible: !appSpelling.finished
                    text: qsTr("Remplacer")
                    onClicked: appSpelling.replace(replacement.text)
                }
                Button {
                    objectName: "spellingReplaceAll"
                    visible: !appSpelling.finished
                    text: qsTr("Remplacer tout")
                    onClicked: appSpelling.replaceAll(replacement.text)
                }
                Button { objectName: "spellingClose"; text: qsTr("Fermer"); onClicked: popup.close() }
            }
        }
    }

    Connections {
        target: appSpelling
        function onStateChanged() {
            if (appSpelling.running) {
                replacement.text = appSpelling.suggestions.length > 0 ? appSpelling.suggestions[0] : appSpelling.word
                replacement.selectAll()
                replacement.forceActiveFocus()
            }
        }
    }

    Shortcut { sequence: "Escape"; enabled: popup.opened; onActivated: popup.close() }
    Shortcut { sequence: "Alt+I"; enabled: popup.opened && appSpelling.running; onActivated: appSpelling.ignore() }
    Shortcut { sequence: "Alt+T"; enabled: popup.opened && appSpelling.running; onActivated: appSpelling.ignoreAlways() }
    Shortcut { sequence: "Alt+A"; enabled: popup.opened && appSpelling.running; onActivated: appSpelling.addToDictionary() }
    Shortcut { sequence: "Alt+R"; enabled: popup.opened && appSpelling.running; onActivated: appSpelling.replaceAll(replacement.text) }

    Component.onCompleted: {
        root.commands.setHandler("spell.check", function () {
            appSpelling.start()
            popup.open()
        })
    }
}
