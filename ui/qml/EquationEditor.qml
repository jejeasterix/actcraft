import QtQuick
import QtQuick.Controls.Basic
import ActCraft.Ui

// Éditeur d'équations à modèles (F-301, L7-T26) : panneau flottant (FloatingPanel : couverture de la page,
// titre, croix, déplaçable). La commande insert.equation l'ouvre sur une équation neuve, une case vide. Les
// palettes (douze catégories) insèrent un modèle au curseur ; chaque bouton dessine son modèle par l'aperçu
// lui-même. La saisie va dans la case surlignée : clavier physique, touches du panneau, ou clavier du système
// par un champ caché qui prend le focus quand on touche l'aperçu (écran sans clavier) ; les trois voies
// appellent type(). « Insérer » pose l'équation au centre de la vue, sans aucune marque de l'éditeur ; une
// case vide la fait refuser, et le message dit pourquoi.
FloatingPanel {
    id: root
    property Commands commands: null
    property DocumentController documentController: null
    property Tools tools: null
    property QtObject controller: equationController
    property string category: "fractions"

    title: qsTr("Équation")
    visible: false
    width: 460
    height: Theme.tailleBouton + column.implicitHeight + 2 * Theme.espace2
    x: parent ? Math.max(0, (parent.width - width) / 2) : 0
    y: 70

    EquationController {
        id: equationController
        objectName: "equation"
        documentController: root.documentController
    }

    // Modèles de la catégorie montrée.
    readonly property var entries: {
        const list = equationController.palettes
        for (let i = 0; i < list.length; ++i) {
            if (list[i].id === root.category)
                return list[i].entries
        }
        return []
    }

    // Touches du panneau : texte montré, saisie envoyée (input) ou touche d'édition (key), nom d'objet.
    readonly property var keys: [
        {"text": "1", "input": "1", "name": "1"}, {"text": "2", "input": "2", "name": "2"},
        {"text": "3", "input": "3", "name": "3"}, {"text": "4", "input": "4", "name": "4"},
        {"text": "5", "input": "5", "name": "5"}, {"text": "6", "input": "6", "name": "6"},
        {"text": "7", "input": "7", "name": "7"}, {"text": "8", "input": "8", "name": "8"},
        {"text": "9", "input": "9", "name": "9"}, {"text": "0", "input": "0", "name": "0"},
        {"text": "x", "input": "x", "name": "x"}, {"text": "y", "input": "y", "name": "y"},
        {"text": "n", "input": "n", "name": "n"}, {"text": "a", "input": "a", "name": "a"},
        {"text": "b", "input": "b", "name": "b"}, {"text": "+", "input": "+", "name": "Plus"},
        {"text": "−", "input": "-", "name": "Minus"}, {"text": "×", "input": "*", "name": "Times"},
        {"text": "=", "input": "=", "name": "Equals"}, {"text": ",", "input": ",", "name": "Comma"},
        {"text": "(", "input": "(", "name": "Open"}, {"text": ")", "input": ")", "name": "Close"},
        {"text": "÷", "input": "÷", "name": "Divide"}, {"text": "<", "input": "<", "name": "Less"},
        {"text": ">", "input": ">", "name": "Greater"},
        {"text": "←", "key": Qt.Key_Left, "name": "Left", "label": qsTr("Case précédente")},
        {"text": "→", "key": Qt.Key_Right, "name": "Right", "label": qsTr("Case suivante")},
        {"text": "↑", "key": Qt.Key_Up, "name": "Up", "label": qsTr("Case au-dessus")},
        {"text": "↓", "key": Qt.Key_Down, "name": "Down", "label": qsTr("Case au-dessous")},
        {"text": "⌫", "key": Qt.Key_Backspace, "name": "Backspace", "label": qsTr("Effacer")}
    ]

    // Centre de la vue en coordonnées de page ; NaN sans vue (le contrôleur prend alors le centre de la page).
    function viewCentre() {
        const view = root.pageArea ? root.pageArea.view : null
        if (!view)
            return Qt.point(NaN, NaN)
        return view.sceneToPage(view.mapToItem(null, view.width / 2, view.height / 2))
    }

    function accept() {
        if (equationController.accept(root.viewCentre()))
            root.close()
    }

    function openEditor() {
        equationController.open()
        root.visible = true
        root.forceActiveFocus()
    }

    // Touches d'édition, communes au panneau, au clavier physique et au champ caché ; vrai si prise.
    function editKey(key) {
        switch (key) {
        case Qt.Key_Left:
            equationController.moveLeft()
            return true
        case Qt.Key_Right:
            equationController.moveRight()
            return true
        case Qt.Key_Up:
            equationController.moveUp()
            return true
        case Qt.Key_Down:
            equationController.moveDown()
            return true
        case Qt.Key_Backspace:
            equationController.backspace()
            return true
        case Qt.Key_Return:
        case Qt.Key_Enter:
            root.accept()
            return true
        case Qt.Key_Escape:
            root.close()
            return true
        case Qt.Key_Delete:
            return true // aucun effacement vers l'avant : la touche est prise, sans effet
        default:
            return false
        }
    }

    // Touches que l'éditeur prend avant les raccourcis de l'application (Main.qml, Qt.ApplicationShortcut :
    // Échap pour tool.select, Suppr pour edit.delete) : édition, Entrée, Échap et saisie, sans Ctrl, Alt ni
    // Méta ; les combinaisons et les touches de fonction restent à l'application.
    function ownsKey(event) {
        if (!root.visible)
            return false // éditeur fermé : il garde le focus actif mais ne prend aucune touche
        if (event.modifiers & (Qt.ControlModifier | Qt.AltModifier | Qt.MetaModifier))
            return false
        switch (event.key) {
        case Qt.Key_Escape:
        case Qt.Key_Delete:
        case Qt.Key_Backspace:
        case Qt.Key_Return:
        case Qt.Key_Enter:
        case Qt.Key_Left:
        case Qt.Key_Right:
        case Qt.Key_Up:
        case Qt.Key_Down:
            return true
        default:
            return event.text.length > 0 && event.text.charCodeAt(0) >= 32
        }
    }

    focus: true
    Keys.onShortcutOverride: event => { event.accepted = root.ownsKey(event) }
    Keys.onPressed: event => {
        if (root.editKey(event.key)) {
            event.accepted = true
        } else if (event.text.length > 0 && event.text.charCodeAt(0) >= 32) {
            equationController.type(event.text)
            event.accepted = true
        }
    }

    Column {
        id: column
        width: parent ? parent.width : 0
        spacing: Theme.espace2

        Label {
            objectName: "equationHint"
            width: parent.width
            text: qsTr("Choisissez un modèle, puis écrivez dans la case surlignée.")
            textFormat: Text.PlainText
            wrapMode: Text.WordWrap
            color: Theme.texteDiscret
            font.pixelSize: Theme.taillePolice
        }

        // Catégories.
        Flow {
            width: parent.width
            spacing: Theme.espace1
            Repeater {
                model: equationController.palettes
                Button {
                    required property var modelData
                    objectName: "equationPalette_" + modelData.id
                    height: 28
                    text: modelData.label
                    font.pixelSize: 12
                    highlighted: root.category === modelData.id
                    focusPolicy: Qt.NoFocus
                    onClicked: root.category = modelData.id
                }
            }
        }

        // Modèles de la catégorie, chacun dessiné par l'aperçu.
        Flow {
            width: parent.width
            spacing: Theme.espace1
            Repeater {
                model: root.entries
                Button {
                    id: templateButton
                    required property var modelData
                    objectName: "equationTemplate_" + modelData.id
                    width: 60
                    height: 44
                    padding: Theme.espace1
                    focusPolicy: Qt.NoFocus
                    Accessible.name: modelData.label
                    background: Rectangle {
                        color: Theme.page
                        radius: Theme.rayon
                        border.width: templateButton.down ? 2 : 1
                        border.color: templateButton.down ? Theme.surfaceActive : Theme.contour
                    }
                    contentItem: EquationPreview {
                        mathml: templateButton.modelData.mathml
                    }
                    onClicked: equationController.insertTemplate(modelData.id)
                }
            }
        }

        // Aperçu : l'équation éditée, cases vides et curseur montrés ; le toucher donne le focus au champ caché.
        Rectangle {
            id: previewArea
            width: parent.width
            height: 110
            color: Theme.page
            radius: Theme.rayon

            EquationPreview {
                id: preview
                objectName: "equationPreview"
                // Taille naturelle, agrandie deux fois au plus, bornée à la zone.
                readonly property size natural: equationController.editorSize
                readonly property real fit: natural.width > 0 && natural.height > 0
                                            ? Math.min(2, (previewArea.width - 2 * Theme.espace2) / natural.width,
                                                       (previewArea.height - 2 * Theme.espace2) / natural.height)
                                            : 1
                anchors.centerIn: parent
                width: Math.max(1, natural.width * fit)
                height: Math.max(1, natural.height * fit)
                mathml: equationController.editorMathml
            }

            TextInput {
                id: hiddenInput
                objectName: "equationHiddenInput"
                width: 1
                height: 1
                opacity: 0
                inputMethodHints: Qt.ImhNoPredictiveText | Qt.ImhNoAutoUppercase
                Accessible.name: qsTr("Saisie de l'équation")
                onTextEdited: {
                    if (text.length > 0) {
                        const typed = text
                        clear()
                        equationController.type(typed)
                    }
                }
                Keys.onShortcutOverride: event => { event.accepted = root.ownsKey(event) }
                Keys.onPressed: event => {
                    if (root.editKey(event.key))
                        event.accepted = true
                }
            }

            TapHandler {
                onTapped: hiddenInput.forceActiveFocus()
            }
        }

        Label {
            objectName: "equationMessage"
            width: parent.width
            visible: text !== ""
            text: equationController.message
            textFormat: Text.PlainText
            wrapMode: Text.WordWrap
            color: Theme.surfaceActive
            font.pixelSize: Theme.taillePolice
        }

        Grid {
            id: grid
            columns: 10
            spacing: Theme.espace1
            Repeater {
                model: root.keys
                Button {
                    required property var modelData
                    objectName: "equationKey" + modelData.name
                    width: (column.width - 9 * grid.spacing) / 10
                    height: 36
                    text: modelData.text
                    focusPolicy: Qt.NoFocus
                    Accessible.name: modelData.label !== undefined ? modelData.label : modelData.text
                    onClicked: {
                        if (modelData.input !== undefined)
                            equationController.type(modelData.input)
                        else
                            root.editKey(modelData.key)
                    }
                }
            }
        }

        Row {
            width: parent.width
            spacing: Theme.espace2
            Button {
                objectName: "equationOkButton"
                width: (parent.width - parent.spacing) / 2
                height: 36
                text: qsTr("Insérer")
                enabled: root.documentController !== null
                focusPolicy: Qt.NoFocus
                onClicked: root.accept()
            }
            Button {
                objectName: "equationCancelButton"
                width: (parent.width - parent.spacing) / 2
                height: 36
                text: qsTr("Annuler")
                focusPolicy: Qt.NoFocus
                onClicked: root.close()
            }
        }
    }

    function registerCommands() {
        if (!root.commands) {
            return
        }
        root.commands.setHandler("insert.equation", function () {
            root.openEditor()
        })
    }

    onCommandsChanged: registerCommands()
    Component.onCompleted: registerCommands()
}
