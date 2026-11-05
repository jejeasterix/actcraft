import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Dialogs
import ActCraft.Ui

// Ligne du navigateur de propriétés (F-570 à F-576) : libellé et éditeur choisi par le rôle `editor`. Délégué
// de PropertyBrowser : les propriétés requises viennent des rôles du PropertyModel. L'éditeur n'écrit jamais
// dans le document : il appelle setValue (et endEdit au relâcher de la réglette) ; la valeur affichée est
// relue du modèle après sa remise à jour, jamais liée dans l'autre sens. Une valeur refusée (hors bornes) laisse
// le modèle intact : l'éditeur reprend alors la valeur du modèle (resync).
FocusScope {
    id: root
    objectName: "prop-" + root.key
    property Commands commands: null
    property DocumentController documentController: null
    property Tools tools: null
    property PageView view: null
    property var profile: null
    property var preferences: null

    // Rôles du PropertyModel.
    required property int index
    required property string key
    required property string label
    required property string editor
    required property var value
    required property var choices
    required property var minimum
    required property var maximum
    property PropertyModel properties: null
    property bool folded: false

    readonly property bool editable: root.properties ? root.properties.editable : false
    readonly property bool hasValue: root.value !== undefined && root.value !== null
    readonly property real low: root.minimum === undefined || root.minimum === null ? -1000000 : root.minimum
    readonly property real high: root.maximum === undefined || root.maximum === null ? 1000000 : root.maximum
    readonly property string valueText: !root.hasValue ? "" : root.editor === "keywords"
        ? root.value.join(" ") : String(root.value)

    signal resync()
    function commit(newValue) {
        if (root.properties)
            root.properties.setValue(root.index, newValue)
        root.resync()
    }

    visible: !root.folded
    implicitHeight: root.folded ? 0 : Math.max(Theme.tailleBouton - 8, loader.implicitHeight) + Theme.espace1
    height: implicitHeight

    Text {
        id: caption
        anchors.left: parent.left
        anchors.leftMargin: Theme.espace2
        anchors.verticalCenter: parent.verticalCenter
        width: Math.round(root.width * 0.38)
        text: root.label
        color: Theme.texte
        font.pixelSize: Theme.taillePolice
        elide: Text.ElideRight
    }

    Loader {
        id: loader
        anchors.left: caption.right
        anchors.leftMargin: Theme.espace1
        anchors.right: parent.right
        anchors.rightMargin: Theme.espace2
        anchors.verticalCenter: parent.verticalCenter
        focus: true
        sourceComponent: {
            switch (root.editor) {
            case "number": return numberEditor
            case "color": return colorEditor
            case "slider": return sliderEditor
            case "choice": return choiceEditor
            case "bool": return boolEditor
            default: return textEditor // text, keywords
            }
        }
    }

    // Texte et mots-clés (séparés par des espaces ; une virgule sépare aussi, côté modèle).
    Component {
        id: textEditor
        TextField {
            id: field
            text: root.valueText
            readOnly: !root.editable
            color: Theme.texte
            selectByMouse: true
            font.pixelSize: Theme.taillePolice
            background: Rectangle {
                radius: Theme.rayon
                color: Theme.surface
                border.color: field.activeFocus ? Theme.surfaceActive : Theme.separateur
            }
            onEditingFinished: {
                if (field.text !== root.valueText)
                    root.commit(field.text)
            }
            Connections {
                target: root
                function onResync() { field.text = Qt.binding(() => root.valueText) }
            }
        }
    }

    // Nombre réel : SpinBox à deux décimales (valeur entière = nombre × 100), bornes du modèle.
    Component {
        id: numberEditor
        SpinBox {
            id: spin
            readonly property int decimals: 2
            readonly property real factor: 100
            readonly property int shown: root.hasValue ? Math.round(Number(root.value) * spin.factor) : 0
            editable: true
            enabled: root.editable
            from: Math.round(root.low * spin.factor)
            to: Math.round(root.high * spin.factor)
            stepSize: spin.factor
            value: spin.shown
            font.pixelSize: Theme.taillePolice
            validator: DoubleValidator {
                bottom: root.low
                top: root.high
                decimals: spin.decimals
                notation: DoubleValidator.StandardNotation
                locale: "C"
            }
            textFromValue: function (v) {
                return root.hasValue ? Number(v / spin.factor).toFixed(spin.decimals).replace(/\.?0+$/, "") : ""
            }
            valueFromText: function (text) {
                const n = Number.parseFloat(text)
                return Number.isFinite(n) ? Math.round(n * spin.factor) : spin.value
            }
            contentItem: TextInput {
                text: spin.displayText
                font: spin.font
                color: Theme.texte
                selectByMouse: true
                selectionColor: Theme.surfaceActive
                selectedTextColor: Theme.texteSurActive
                horizontalAlignment: Qt.AlignLeft
                verticalAlignment: Qt.AlignVCenter
                readOnly: !spin.editable
                validator: spin.validator
                inputMethodHints: Qt.ImhFormattedNumbersOnly
                onActiveFocusChanged: if (activeFocus) selectAll()
                // Entrée valide la saisie ; la touche est prise ici (le champ la laisse sinon passer).
                function validate(event) {
                    spin.commitText(text)
                    event.accepted = true
                }
                Keys.onReturnPressed: event => validate(event)
                Keys.onEnterPressed: event => validate(event)
            }
            background: Rectangle {
                radius: Theme.rayon
                color: Theme.surface
                border.color: spin.activeFocus ? Theme.surfaceActive : Theme.separateur
            }
            function commitText(text) {
                const v = spin.valueFromText(text) / spin.factor
                if (!root.hasValue || v !== Number(root.value))
                    root.commit(v)
                else
                    root.resync()
            }
            onValueModified: spin.commitText(spin.textFromValue(spin.value))
            Connections {
                target: root
                function onResync() {
                    spin.value = Qt.binding(() => spin.shown)
                }
            }
        }
    }

    // Pastille et boîte de couleur.
    Component {
        id: colorEditor
        Item {
            implicitHeight: Theme.taillePastille + Theme.espace1
            Rectangle {
                id: swatch
                width: Theme.taillePastille * 2
                height: Theme.taillePastille
                radius: Theme.rayon
                color: root.hasValue ? root.value : "transparent"
                border.color: Theme.contour
                opacity: root.editable ? 1 : 0.5
                MouseArea {
                    anchors.fill: parent
                    enabled: root.editable
                    onClicked: {
                        dialog.selectedColor = root.hasValue ? root.value : "white"
                        dialog.open()
                    }
                }
            }
            ColorDialog {
                id: dialog
                title: qsTr("Choisir une couleur")
                onAccepted: root.commit(dialog.selectedColor)
            }
        }
    }

    // Translucidité : un pas par déplacement (le modèle fusionne le glissé en une étape), clos au relâcher.
    Component {
        id: sliderEditor
        Slider {
            id: slider
            from: root.low
            to: root.high
            stepSize: 1
            enabled: root.editable
            value: root.hasValue ? Number(root.value) : root.low
            onMoved: root.commit(slider.value)
            onPressedChanged: {
                if (!slider.pressed && root.properties)
                    root.properties.endEdit()
            }
        }
    }

    Component {
        id: choiceEditor
        ComboBox {
            id: combo
            readonly property int shown: root.hasValue ? Number(root.value) : -1
            model: root.choices
            enabled: root.editable
            currentIndex: combo.shown
            font.pixelSize: Theme.taillePolice
            displayText: combo.currentIndex < 0 ? "" : combo.currentText
            onActivated: index => root.commit(index)
            Connections {
                target: root
                function onResync() { combo.currentIndex = Qt.binding(() => combo.shown) }
            }
        }
    }

    Component {
        id: boolEditor
        Switch {
            id: toggle
            readonly property bool on: root.value === true
            enabled: root.editable
            checked: toggle.on
            onToggled: root.commit(toggle.checked)
            Connections {
                target: root
                function onResync() { toggle.checked = Qt.binding(() => toggle.on) }
            }
        }
    }
}
