import QtQuick
import QtQuick.Controls.Basic
import ActCraft.Ui

// Barre de mise en forme du texte (F-100, D8) : en haut, sous la ligne supérieure, tant qu'un texte horizontal
// s'édite. Ordre : police, taille, plus grand, plus petit, gras, italique, souligné, exposant, indice, couleur,
// fond, trois alignements, interligne plus et moins, retrait plus et moins, puces, tout sélectionner, symboles.
// Chaque bouton déclenche sa commande nommée (les boutons ne prennent pas le focus : le TextEdit garde la frappe
// et sa sélection). Libellés et infobulles à nous, lus dans le registre des commandes.
TextBarBase {
    id: bar
    // Listes ouvertes (police, taille, couleurs, symboles) : la vue ne reçoit rien tant que l'une l'est.
    function closeLists() {
        fontCombo.popup.close()
        sizeCombo.popup.close()
        symbolChoice.close()
        for (let i = 0; i < colorChoices.count; ++i)
            colorChoices.itemAt(i).close()
    }
    onCloseRequested: bar.closeLists()

    readonly property string colorId: "text.color"
    readonly property string backgroundId: "text.background"
    readonly property string selectAllId: "text.select-all"
    readonly property string symbolId: "text.symbol"
    // Symboles à nous.
    readonly property var symbols: ["Ω", "π", "±", "×", "÷", "≤", "≥", "≠", "√", "∞", "°", "€"]

    visible: editing !== null && editing.active && !editing.vertical

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

    // Bouton d'une fenêtre de choix (couleurs, symboles) : icône ou lettre, sans commande propre.
    component ChoiceButton: ToolButton {
        id: choice
        property alias popupContent: choicePopup.contentItem
        signal popupOpened()
        signal popupClosed()
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
            onOpened: choice.popupOpened()
            onClosed: choice.popupClosed()
            y: choice.height + Theme.espace1
            padding: Theme.espace2
            focus: false
            closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutsideParent
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
                id: fontCombo
                objectName: "textFontCombo"
                onPopupOpened: bar.listOpened()
                onPopupClosed: bar.listClosed()
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
                id: sizeCombo
                objectName: "textSizeCombo"
                onPopupOpened: bar.listOpened()
                onPopupClosed: bar.listClosed()
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
                id: colorChoices
                model: [{ id: bar.colorId, fond: false }, { id: bar.backgroundId, fond: true }]
                delegate: ChoiceButton {
                    id: colorChoice
                    onPopupOpened: bar.listOpened()
                    onPopupClosed: bar.listClosed()
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
                onPopupOpened: bar.listOpened()
                onPopupClosed: bar.listClosed()
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
