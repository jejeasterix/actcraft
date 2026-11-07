import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Dialogs
import ActCraft.Ui

// Ligne du navigateur de propriétés (F-570 à F-576) : libellé et éditeur choisi par le rôle `editor`. Délégué
// de PropertyBrowser : les propriétés requises viennent des rôles du PropertyModel. L'éditeur n'écrit jamais
// dans le document : il appelle setValue (et endEdit au relâcher de la réglette) ; la valeur affichée est
// relue du modèle après sa remise à jour, jamais liée dans l'autre sens. Une valeur refusée (hors bornes) laisse
// le modèle intact : l'éditeur reprend alors la valeur du modèle (resync). Interactivité (L8-T17, L8-C2) : le
// rôle `enabled` faux grise la ligne entière (désactivée, atténuée) ; « object » propose les objets de la page
// par leur nom (rôle `choices` : {id, name}, « Aucun » d'abord) ; « actions » ouvre l'onglet « actions » du
// navigateur (showNavigator de BrowserPanel, trouvé en remontant les parents) ; « point » : deux nombres x, y.
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
    required enabled // rôle « enabled » : faux quand l'éditeur est grisé (interactivityprops.h, kEnabledRole)
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

    // Onglet « actions » du navigateur : le premier parent qui sait l'ouvrir (BrowserPanel).
    function openActions() {
        for (let p = root.parent; p; p = p.parent) {
            if (typeof p.showNavigator === "function") {
                p.showNavigator("actions")
                return
            }
        }
    }
    function decimal(v) {
        return Number(v).toFixed(2).replace(/\.?0+$/, "")
    }

    visible: !root.folded
    opacity: root.enabled ? 1 : 0.5
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
            case "object": return objectEditor
            case "actions": return actionsEditor
            case "point": return pointEditor
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

    // Objet choisi sur la page : identifiant (0 : aucun), proposé par son nom.
    Component {
        id: objectEditor
        ComboBox {
            id: objects
            objectName: "object-editor"
            readonly property int shown: objects.count >= 0 && root.hasValue ? objects.indexOfValue(root.value) : -1
            model: root.choices
            textRole: "name"
            valueRole: "id"
            enabled: root.editable
            currentIndex: objects.shown
            font.pixelSize: Theme.taillePolice
            displayText: objects.currentIndex < 0 ? "" : objects.currentText
            onActivated: index => root.commit(objects.valueAt(index))
            Connections {
                target: root
                function onResync() { objects.currentIndex = Qt.binding(() => objects.shown) }
            }
        }
    }

    // Actions de l'objet : liste en lecture (libellés du catalogue), modifiée dans le navigateur d'actions.
    Component {
        id: actionsEditor
        Item {
            implicitHeight: open.implicitHeight
            Text {
                anchors.left: parent.left
                anchors.right: open.left
                anchors.rightMargin: Theme.espace1
                anchors.verticalCenter: parent.verticalCenter
                text: root.hasValue && root.value.length > 0 ? root.value.join(", ") : qsTr("Aucune action")
                color: Theme.texte
                font.pixelSize: Theme.taillePolice
                elide: Text.ElideRight
            }
            Button {
                id: open
                objectName: "actions-open"
                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                text: qsTr("Modifier…")
                font.pixelSize: Theme.taillePolice
                onClicked: root.openActions()
            }
        }
    }

    // Point choisi : x et y dans la convention du champ (le libellé la dit) ; validé quand les deux sont lus.
    Component {
        id: pointEditor
        Row {
            id: point
            spacing: Theme.espace1
            readonly property string shownX: root.hasValue ? root.decimal(root.value.x) : ""
            readonly property string shownY: root.hasValue ? root.decimal(root.value.y) : ""
            function validate() {
                const x = Number.parseFloat(fieldX.text)
                const y = Number.parseFloat(fieldY.text)
                if (Number.isFinite(x) && Number.isFinite(y)
                        && (!root.hasValue || x !== Number.parseFloat(point.shownX)
                                         || y !== Number.parseFloat(point.shownY)))   // arrondi contre arrondi
                    root.commit(Qt.point(x, y))
                else if (fieldX.text !== "" && fieldY.text !== "")
                    root.resync()
            }
            Text {
                anchors.verticalCenter: parent.verticalCenter
                text: "x"
                color: Theme.texte
                font.pixelSize: Theme.taillePolice
            }
            TextField {
                id: fieldX
                objectName: "point-x"
                width: Math.max(40, (loader.width - 3 * Theme.espace1) / 2 - 12)
                text: point.shownX
                readOnly: !root.editable
                color: Theme.texte
                selectByMouse: true
                font.pixelSize: Theme.taillePolice
                validator: DoubleValidator { notation: DoubleValidator.StandardNotation; locale: "C" }
                onEditingFinished: point.validate()
            }
            Text {
                anchors.verticalCenter: parent.verticalCenter
                text: "y"
                color: Theme.texte
                font.pixelSize: Theme.taillePolice
            }
            TextField {
                id: fieldY
                objectName: "point-y"
                width: fieldX.width
                text: point.shownY
                readOnly: !root.editable
                color: Theme.texte
                selectByMouse: true
                font.pixelSize: Theme.taillePolice
                validator: DoubleValidator { notation: DoubleValidator.StandardNotation; locale: "C" }
                onEditingFinished: point.validate()
            }
            Connections {
                target: root
                function onResync() {
                    fieldX.text = Qt.binding(() => point.shownX)
                    fieldY.text = Qt.binding(() => point.shownY)
                }
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
