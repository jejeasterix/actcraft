import QtQuick
import QtQuick.Controls.Basic
import ActCraft.Ui

// Barre de mise en forme du texte (F-100, D8) : en haut, sous la ligne supérieure, tant qu'un texte horizontal
// s'édite. Ordre : police, taille, plus grand, plus petit, gras, italique, souligné, exposant, indice, couleur,
// fond, trois alignements, interligne plus et moins, retrait plus et moins, puces, tout sélectionner, symboles.
// Chaque bouton déclenche sa commande nommée (les boutons ne prennent pas le focus : le TextEdit garde la frappe
// et sa sélection). Libellés et infobulles à nous, lus dans le registre des commandes.
Item {
    id: bar
    property Commands commands: null
    property TextEditing editing: null

    // Identifiants lus depuis des propriétés : l'AOT refuse l'argument littéral (voir TrashCan.qml).
    readonly property string fontId: "text.font"
    readonly property string sizeId: "text.size"
    readonly property string colorId: "text.color"
    readonly property string backgroundId: "text.background"
    readonly property string selectAllId: "text.select-all"
    readonly property string symbolId: "text.symbol"
    readonly property var sizes: [8, 10, 12, 14, 18, 24, 36, 48, 72]
    // Symboles à nous.
    readonly property var symbols: ["Ω", "π", "±", "×", "÷", "≤", "≥", "≠", "√", "∞", "°", "€"]

    visible: editing !== null && editing.active && !editing.vertical
    implicitHeight: Theme.tailleBouton + Theme.espace2
    height: implicitHeight

    // Rend la frappe au texte après un choix dans une liste ou une fenêtre (le focus est passé au bouton).
    function backToText() {
        if (editing)
            editing.focusRequested()
    }

    // Combiné aux couleurs de la page : l'appui hors des boutons ne traverse pas la barre jusqu'à la page.
    Rectangle {
        anchors.fill: parent
        color: Theme.palette
        MouseArea { anchors.fill: parent }
        Rectangle {
            anchors.bottom: parent.bottom
            width: parent.width
            height: 1
            color: Theme.separateur
        }
    }

    component Separator: Item {
        width: Theme.espace2
        height: Theme.tailleBouton
        Rectangle {
            anchors.centerIn: parent
            width: 1
            height: Theme.tailleBouton - 14
            color: Theme.separateur
        }
    }

    // Liste déroulante à nous (le style de base est clair).
    component BarCombo: ComboBox {
        id: combo
        focusPolicy: Qt.NoFocus
        implicitHeight: Theme.tailleBouton - Theme.espace2
        font.pixelSize: Theme.taillePolice
        contentItem: Text {
            leftPadding: Theme.espace2
            rightPadding: combo.indicator.width + Theme.espace1
            text: combo.displayText
            font: combo.font
            color: Theme.texte
            elide: Text.ElideRight
            verticalAlignment: Text.AlignVCenter
        }
        indicator: Text {
            x: combo.width - width - Theme.espace2
            y: (combo.height - height) / 2
            text: "▾"
            color: Theme.texteDiscret
            font.pixelSize: Theme.taillePolice
        }
        background: Rectangle {
            radius: Theme.rayon
            color: combo.hovered || combo.popup.visible ? Theme.surfaceSurvol : Theme.surface
        }
        delegate: ItemDelegate {
            id: entry
            required property var modelData
            required property int index
            width: combo.width
            height: Theme.tailleBouton - Theme.espace2
            focusPolicy: Qt.NoFocus
            highlighted: combo.highlightedIndex === index
            contentItem: Text {
                text: entry.modelData
                font: combo.font
                color: entry.highlighted ? Theme.texteSurActive : Theme.texte
                elide: Text.ElideRight
                verticalAlignment: Text.AlignVCenter
            }
            background: Rectangle { color: entry.highlighted ? Theme.surfaceActive : "transparent" }
        }
        popup: Popup {
            y: combo.height
            width: combo.width
            height: Math.min(contentItem.implicitHeight, 8 * (Theme.tailleBouton - Theme.espace2))
            padding: 1
            contentItem: ListView {
                clip: true
                implicitHeight: contentHeight
                model: combo.popup.visible ? combo.delegateModel : null
                currentIndex: combo.highlightedIndex
            }
            background: Rectangle {
                color: Theme.palette
                border.color: Theme.contour
                radius: Theme.rayon
            }
        }
    }

    // Bouton d'une fenêtre de choix (couleurs, symboles) : icône ou lettre, sans commande propre.
    component ChoiceButton: ToolButton {
        id: choice
        property alias popupContent: choicePopup.contentItem
        focusPolicy: Qt.NoFocus
        implicitWidth: Theme.tailleBouton
        implicitHeight: Theme.tailleBouton
        background: Rectangle {
            radius: Theme.rayon
            color: choice.down || choicePopup.visible ? Theme.surfaceActive : choice.hovered ? Theme.surfaceSurvol : Theme.surface
        }
        onClicked: choicePopup.visible ? choicePopup.close() : choicePopup.open()
        function close() { choicePopup.close() }
        Popup {
            id: choicePopup
            y: choice.height + Theme.espace1
            padding: Theme.espace2
            focus: false
            closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
            background: Rectangle {
                color: Theme.palette
                border.color: Theme.contour
                radius: Theme.rayon
            }
        }
    }

    Flickable {
        id: scroller
        anchors.fill: parent
        anchors.leftMargin: Theme.espace2
        anchors.rightMargin: Theme.espace2
        contentWidth: row.width
        contentHeight: height
        flickableDirection: Flickable.HorizontalFlick
        boundsBehavior: Flickable.StopAtBounds
        clip: true

        Row {
            id: row
            height: scroller.height
            spacing: Theme.espace1

            BarCombo {
                objectName: "textFontCombo"
                width: 168
                y: (row.height - height) / 2
                model: Qt.fontFamilies()
                displayText: bar.editing ? bar.editing.fontFamily : ""
                onActivated: index => {
                    bar.commands.trigger(bar.fontId, model[index])
                    bar.backToText()
                }
            }
            BarCombo {
                objectName: "textSizeCombo"
                width: 76
                y: (row.height - height) / 2
                model: bar.sizes
                displayText: bar.editing ? String(Math.round(bar.editing.fontSize)) : ""
                onActivated: index => {
                    bar.commands.trigger(bar.sizeId, bar.sizes[index])
                    bar.backToText()
                }
            }
            CommandButton { commands: bar.commands; commandId: "text.size-up"; y: (row.height - height) / 2 }
            CommandButton { commands: bar.commands; commandId: "text.size-down"; y: (row.height - height) / 2 }
            Separator { y: (row.height - height) / 2 }
            CommandButton { commands: bar.commands; commandId: "text.bold"; checked: bar.editing !== null && bar.editing.bold; y: (row.height - height) / 2 }
            CommandButton { commands: bar.commands; commandId: "text.italic"; checked: bar.editing !== null && bar.editing.italic; y: (row.height - height) / 2 }
            CommandButton { commands: bar.commands; commandId: "text.underline"; checked: bar.editing !== null && bar.editing.underline; y: (row.height - height) / 2 }
            CommandButton { commands: bar.commands; commandId: "text.superscript"; y: (row.height - height) / 2 }
            CommandButton { commands: bar.commands; commandId: "text.subscript"; y: (row.height - height) / 2 }
            Separator { y: (row.height - height) / 2 }

            // Couleur du texte puis fond : une grille des couleurs du registre.
            Repeater {
                model: [{ id: bar.colorId, fond: false }, { id: bar.backgroundId, fond: true }]
                delegate: ChoiceButton {
                    id: colorChoice
                    required property var modelData
                    objectName: modelData.fond ? "textBackgroundButton" : "textColorButton"
                    y: (row.height - height) / 2
                    Accessible.name: bar.commands.label(modelData.id)
                    ToolTip.visible: hovered
                    ToolTip.delay: 600
                    ToolTip.text: bar.commands.label(modelData.id)
                    contentItem: Item {
                        Text {
                            anchors.centerIn: parent
                            text: colorChoice.modelData.fond ? "▮" : "A"
                            font.pixelSize: Theme.taillePoliceTitre
                            font.bold: !colorChoice.modelData.fond
                            color: colorChoice.down ? Theme.texteSurActive : Theme.texte
                        }
                    }
                    popupContent: Grid {
                        columns: 4
                        spacing: Theme.espace1
                        Repeater {
                            model: bar.commands.colors
                            delegate: Rectangle {
                                id: swatch
                                required property string modelData
                                width: Theme.taillePastille + Theme.espace1
                                height: width
                                radius: 3
                                color: modelData
                                border.width: 1
                                border.color: Theme.contour
                                Accessible.role: Accessible.Button
                                Accessible.name: qsTr("Couleur %1").arg(modelData)
                                TapHandler {
                                    onTapped: {
                                        bar.commands.trigger(colorChoice.modelData.id, swatch.modelData)
                                        colorChoice.close()
                                        bar.backToText()
                                    }
                                }
                            }
                        }
                    }
                }
            }
            Separator { y: (row.height - height) / 2 }
            CommandButton { commands: bar.commands; commandId: "text.align-left"; y: (row.height - height) / 2 }
            CommandButton { commands: bar.commands; commandId: "text.align-center"; y: (row.height - height) / 2 }
            CommandButton { commands: bar.commands; commandId: "text.align-right"; y: (row.height - height) / 2 }
            Separator { y: (row.height - height) / 2 }
            CommandButton { commands: bar.commands; commandId: "text.spacing-up"; y: (row.height - height) / 2 }
            CommandButton { commands: bar.commands; commandId: "text.spacing-down"; y: (row.height - height) / 2 }
            CommandButton { commands: bar.commands; commandId: "text.indent"; y: (row.height - height) / 2 }
            CommandButton { commands: bar.commands; commandId: "text.outdent"; y: (row.height - height) / 2 }
            CommandButton { commands: bar.commands; commandId: "text.bullets"; y: (row.height - height) / 2 }
            Separator { y: (row.height - height) / 2 }
            ToolButton {
                objectName: "textSelectAllButton"
                y: (row.height - height) / 2
                focusPolicy: Qt.NoFocus
                implicitHeight: Theme.tailleBouton
                text: bar.commands.label(bar.selectAllId)
                font.pixelSize: Theme.taillePolice
                contentItem: Text {
                    text: parent.text
                    font: parent.font
                    color: Theme.texte
                    verticalAlignment: Text.AlignVCenter
                    horizontalAlignment: Text.AlignHCenter
                }
                background: Rectangle {
                    radius: Theme.rayon
                    color: parent.down ? Theme.surfaceActive : parent.hovered ? Theme.surfaceSurvol : Theme.surface
                }
                onClicked: bar.commands.trigger(bar.selectAllId)
            }
            ChoiceButton {
                id: symbolChoice
                objectName: "textSymbolButton"
                y: (row.height - height) / 2
                icon.source: bar.commands.iconUrl(bar.symbolId)
                icon.width: Theme.tailleIcone
                icon.height: Theme.tailleIcone
                icon.color: Theme.texte
                display: AbstractButton.IconOnly
                Accessible.name: bar.commands.label(bar.symbolId)
                ToolTip.visible: hovered
                ToolTip.delay: 600
                ToolTip.text: bar.commands.label(bar.symbolId)
                popupContent: Grid {
                    columns: 4
                    spacing: Theme.espace1
                    Repeater {
                        model: bar.symbols
                        delegate: Rectangle {
                            id: sign
                            required property string modelData
                            width: Theme.tailleBouton - Theme.espace2
                            height: width
                            radius: Theme.rayon
                            color: signTap.pressed ? Theme.surfaceActive : Theme.surface
                            Accessible.role: Accessible.Button
                            Accessible.name: modelData
                            Text {
                                anchors.centerIn: parent
                                text: sign.modelData
                                font.pixelSize: Theme.taillePoliceTitre
                                color: signTap.pressed ? Theme.texteSurActive : Theme.texte
                            }
                            TapHandler {
                                id: signTap
                                onTapped: {
                                    bar.commands.trigger(bar.symbolId, sign.modelData)
                                    symbolChoice.close()
                                    bar.backToText()
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}
