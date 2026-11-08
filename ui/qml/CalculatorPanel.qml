import QtQuick
import QtQuick.Controls.Basic
import ActCraft.Ui

// Calculatrice (F-286, L7-T15) : panneau flottant (FloatingPanel : couverture de la page, titre, croix,
// déplaçable). La commande tool.calculator applique le réglage « calculatrice » : le panneau d'ActCraft se montre,
// ou le programme choisi est lancé (Windows). Deux lignes, le calcul en cours au-dessus du résultat ; « C »
// efface tout le calcul, « CE » la dernière saisie ; « Poser sur la page » écrit le calcul en texte sur la page
// affichée. Le clavier physique tape aussi, tant que le panneau a le focus.
FloatingPanel {
    id: root
    property Commands commands: null
    property DocumentController documentController: null
    property Tools tools: null
    property QtObject controller: calculatorController

    title: qsTr("Calculatrice")
    visible: false
    width: 280
    height: Theme.tailleBouton + column.implicitHeight + 2 * Theme.espace2
    x: parent ? Math.max(0, parent.width - width - 40) : 0
    y: 90

    CalculatorController {
        id: calculatorController
        objectName: "calculator"
        documentController: root.documentController
    }

    // Les touches, ligne par ligne : texte affiché, touche envoyée au contrôleur, nom d'objet.
    readonly property var keys: [
        {"text": "MC", "key": "MC", "name": "MC"}, {"text": "MR", "key": "MR", "name": "MR"},
        {"text": "M−", "key": "M-", "name": "MMinus"}, {"text": "M+", "key": "M+", "name": "MPlus"},
        {"text": "C", "key": "C", "name": "C"}, {"text": "CE", "key": "CE", "name": "CE"},
        {"text": "%", "key": "%", "name": "Percent"}, {"text": "÷", "key": "/", "name": "Divide"},
        {"text": "7", "key": "7", "name": "7"}, {"text": "8", "key": "8", "name": "8"},
        {"text": "9", "key": "9", "name": "9"}, {"text": "×", "key": "*", "name": "Times"},
        {"text": "4", "key": "4", "name": "4"}, {"text": "5", "key": "5", "name": "5"},
        {"text": "6", "key": "6", "name": "6"}, {"text": "−", "key": "-", "name": "Minus"},
        {"text": "1", "key": "1", "name": "1"}, {"text": "2", "key": "2", "name": "2"},
        {"text": "3", "key": "3", "name": "3"}, {"text": "+", "key": "+", "name": "Plus"},
        {"text": "±", "key": "±", "name": "Sign"}, {"text": "0", "key": "0", "name": "0"},
        {"text": ",", "key": ",", "name": "Comma"}, {"text": "=", "key": "=", "name": "Equals"}
    ]

    // Touche du clavier physique vers la touche de la calculatrice (vide : aucune).
    function keyFor(event) {
        if (event.text.length === 1 && event.text >= "0" && event.text <= "9") {
            return event.text
        }
        switch (event.key) {
        case Qt.Key_Comma:
        case Qt.Key_Period:
            return ","
        case Qt.Key_Plus:
            return "+"
        case Qt.Key_Minus:
            return "-"
        case Qt.Key_Asterisk:
            return "*"
        case Qt.Key_Slash:
            return "/"
        case Qt.Key_Percent:
            return "%"
        case Qt.Key_Equal:
        case Qt.Key_Enter:
        case Qt.Key_Return:
            return "="
        case Qt.Key_Backspace:
            return "CE"
        case Qt.Key_C:
            return "C"
        default:
            return ""
        }
    }

    focus: true
    Keys.onPressed: event => {
        const key = root.keyFor(event)
        if (key !== "") {
            calculatorController.press(key)
            event.accepted = true
        }
    }

    Column {
        id: column
        width: parent ? parent.width : 0
        spacing: Theme.espace2

        // Deux lignes : le calcul en cours, puis le résultat ou la saisie.
        Label {
            objectName: "calculatorExpression"
            width: parent.width
            height: Theme.taillePolice * 1.6
            horizontalAlignment: Text.AlignRight
            verticalAlignment: Text.AlignVCenter
            elide: Text.ElideLeft
            text: calculatorController.expression
            textFormat: Text.PlainText
            color: Theme.texte
            font.pixelSize: Theme.taillePolice
        }

        Label {
            objectName: "calculatorDisplay"
            width: parent.width
            height: Theme.tailleBouton
            horizontalAlignment: Text.AlignRight
            verticalAlignment: Text.AlignVCenter
            elide: Text.ElideLeft
            text: calculatorController.display
            textFormat: Text.PlainText
            color: Theme.texte
            font.pixelSize: Theme.taillePoliceTitre

            // Le résultat se glisse hors du panneau : lâché sur la page, le calcul s'y écrit à cet endroit
            // (une étape d'annulation). Lâché dans le panneau, hors de la vue (barre d'outils, miniatures) ou
            // hors de la page, rien ne s'écrit. Sans document où écrire, pas de glisser (comme le bouton).
            DragHandler {
                id: resultDrag
                objectName: "calculatorResultDrag"
                target: null
                enabled: calculatorController.canWrite
                property point dropScene: Qt.point(0, 0)
                onCentroidChanged: {
                    if (active) {
                        dropScene = centroid.scenePosition
                    }
                }
                onActiveChanged: {
                    if (active || !root.pageArea || !root.pageArea.view) {
                        return
                    }
                    const local = root.mapFromItem(null, dropScene.x, dropScene.y)
                    if (root.contains(local)) {
                        return
                    }
                    const view = root.pageArea.view
                    const content = view.content
                    if (!content || !view.contains(view.mapFromItem(null, dropScene.x, dropScene.y))) {
                        return
                    }
                    const page = view.sceneToPage(dropScene)
                    if (page.x < 0 || page.y < 0 || page.x > content.width || page.y > content.height) {
                        return
                    }
                    calculatorController.placeAt(page.x, page.y)
                }
            }
        }

        Grid {
            id: grid
            columns: 4
            spacing: Theme.espace1
            Repeater {
                model: root.keys
                Button {
                    required property var modelData
                    objectName: "calcKey" + modelData.name
                    width: (column.width - 3 * grid.spacing) / 4
                    height: Theme.tailleBouton
                    text: modelData.text
                    focusPolicy: Qt.NoFocus
                    Accessible.name: modelData.text
                    onClicked: calculatorController.press(modelData.key)
                }
            }
        }

        Button {
            objectName: "calculatorPlaceButton"
            width: parent.width
            height: Theme.tailleBouton
            text: qsTr("Poser sur la page")
            enabled: calculatorController.canWrite
            focusPolicy: Qt.NoFocus
            onClicked: calculatorController.placeOnPage()
        }
    }

    function registerCommands() {
        if (!root.commands) {
            return
        }
        root.commands.setHandler("tool.calculator", function () {
            // Calculatrice du système (mode 1 ou 2) lancée : aucun panneau, l'annotation reste (Ma1 essai 3).
            if (calculatorController.open()) {
                root.commands.requestMainWindow("tool.calculator")
                root.visible = true
                root.forceActiveFocus()
            }
        })
    }

    onCommandsChanged: registerCommands()
    Component.onCompleted: registerCommands()
}
