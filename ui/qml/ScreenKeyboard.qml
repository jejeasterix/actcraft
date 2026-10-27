import QtQuick
import ActCraft.Ui

// Clavier à l'écran (F-294), L7-T29. Racine non visuelle d'une fonction à fenêtre propre (décision 4 bis) : elle
// porte le contrôleur et le Component de la fenêtre du clavier, créée à la demande SANS parent
// (createObject(null) : un Window enfant d'un Item prend la fenêtre principale pour parent et serait réduit
// avec elle) et gardée dans `window` (sinon le ramasse-miettes peut la détruire ouverte), détruite à la
// fermeture. La fenêtre ne prend jamais le focus : la frappe va au champ qui l'a, ou à l'application au
// premier plan (KeyboardController). Elle n'est pas une couverture de la vue principale.
Item {
    id: root
    visible: false
    width: 0
    height: 0
    property Commands commands: null
    property DocumentController documentController: null
    property Tools tools: null
    property Item pageArea: null
    property QtObject controller: keyboardController
    property Window window: null

    readonly property real unit: 44 // côté d'une touche
    readonly property real gap: 4

    KeyboardController {
        id: keyboardController
        objectName: "keyboardController"
        onOpenChanged: open ? root.openWindow() : root.closeWindow()
    }

    // Touche dessinée par nous ; prend le stylet, le doigt et la souris. Un composant en ligne ne voit pas les
    // id du document : la taille est posée là où la touche est créée.
    component KeyButton: Rectangle {
        id: button
        property string label
        property bool active: false
        signal activated

        radius: 4
        color: tap.pressed ? "#C8D5E8" : (button.active ? "#DCE7F5" : "#FFFFFF")
        border.width: button.active ? 2 : 1
        border.color: Theme.contour

        Text {
            anchors.centerIn: parent
            text: button.label
            font.pixelSize: 16
            color: "#1E1E1E"
        }

        TapHandler {
            id: tap
            acceptedDevices: PointerDevice.Mouse | PointerDevice.TouchScreen | PointerDevice.Stylus
            // Saisie exclusive dès l'appui (comme un bouton) : avec la saisie passive par défaut, le relâché du
            // stylet n'arrive pas jusqu'au gestionnaire et la touche ne tape pas.
            gesturePolicy: TapHandler.ReleaseWithinBounds
            onTapped: button.activated()
        }
    }

    function capLabel(key) {
        if (key.id.length === 1 && (keyboardController.shifted || keyboardController.capsLocked)) {
            return key.label.toUpperCase()
        }
        return key.label
    }

    function openWindow() {
        if (!root.window) {
            root.window = keyboardComponent.createObject(null)
            keyboardController.window = root.window
        }
        if (root.window) {
            root.window.show()
        }
    }

    function closeWindow() {
        if (!root.window) {
            return
        }
        const closing = root.window
        keyboardController.window = null
        root.window = null
        closing.destroy()
    }

    Component {
        id: keyboardComponent

        Window {
            id: keyboardWindow
            objectName: "screenKeyboardWindow"
            title: qsTr("Clavier à l'écran")
            // Trois drapeaux de 7b : jamais le focus, toujours devant, petite fenêtre d'outil (barre de titre et
            // croix de fermeture du système).
            flags: Qt.Tool | Qt.WindowStaysOnTopHint | Qt.WindowDoesNotAcceptFocus
            color: "#EEF1F5"
            width: content.implicitWidth + 4 * root.gap
            height: content.implicitHeight + 4 * root.gap

            // La croix ferme le clavier (la racine détruit la fenêtre).
            onClosing: keyboardController.open = false

            Component.onCompleted: {
                x = Math.round((Screen.width - width) / 2)
                y = Math.max(0, Screen.height - height - 64)
            }

            Column {
                id: content
                x: 2 * root.gap
                y: 2 * root.gap
                spacing: root.gap

                // Nombre de touches (compact ou complet) et mode étendu (chiffres, accents, symboles).
                Row {
                    spacing: root.gap

                    KeyButton {
                        objectName: "keyboardLayout_compact"
                        width: 2.5 * root.unit
                        height: root.unit
                        label: qsTr("Compact")
                        active: keyboardController.layout === KeyboardController.Layout.Compact
                        onActivated: keyboardController.layout = KeyboardController.Layout.Compact
                    }
                    KeyButton {
                        objectName: "keyboardLayout_standard"
                        width: 2.5 * root.unit
                        height: root.unit
                        label: qsTr("Complet")
                        active: keyboardController.layout === KeyboardController.Layout.Standard
                        onActivated: keyboardController.layout = KeyboardController.Layout.Standard
                    }
                    KeyButton {
                        objectName: "keyboardLayout_extended"
                        width: 2.5 * root.unit
                        height: root.unit
                        label: qsTr("Étendu")
                        active: keyboardController.layout === KeyboardController.Layout.Extended
                        onActivated: keyboardController.layout = KeyboardController.Layout.Extended
                    }
                }

                Repeater {
                    model: keyboardController.rows

                    delegate: Row {
                        id: keyRow
                        required property var modelData
                        spacing: root.gap

                        Repeater {
                            model: keyRow.modelData

                            delegate: KeyButton {
                                required property var modelData
                                objectName: "keyboardKey_" + modelData.id
                                width: modelData.width * root.unit + (Math.ceil(modelData.width) - 1) * root.gap
                                height: root.unit
                                label: root.capLabel(modelData)
                                active: (modelData.id === "shift" && keyboardController.shifted)
                                        || (modelData.id === "caps" && keyboardController.capsLocked)
                                onActivated: keyboardController.press(modelData.id)
                            }
                        }
                    }
                }
            }
        }
    }

    function registerCommands() {
        if (!root.commands) {
            return
        }
        root.commands.setHandler("tool.keyboard", function () {
            keyboardController.toggle()
        })
    }
    onCommandsChanged: registerCommands()

    Component.onCompleted: registerCommands()

    Component.onDestruction: {
        if (root.window) {
            root.window.destroy()
        }
    }
}
