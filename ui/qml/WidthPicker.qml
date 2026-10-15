import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import ActCraft.Ui

// Épaisseur du stylo (F-137, F-138) : quatre préréglages ronds, réglette de 1 à 100, échantillon de la largeur
// et de la couleur courantes. Tout passe par la commande « pen.width ».
// La réglette est logarithmique : les préréglages (2 à 8) occupent le début d'une échelle linéaire de 1 à 100 et
// laissaient le curseur collé à gauche. Position 0..1 = ln(épaisseur) / ln(100).
ColumnLayout {
    id: picker
    objectName: "widthPicker"
    required property Commands commands
    required property PageView view
    spacing: Theme.espace1
    readonly property real maxWidth: 100 // = kMaxPenWidth
    readonly property real widthPosition: Math.log(Math.max(1, picker.view.penWidth)) / Math.log(maxWidth)

    Row {
        spacing: Theme.espace1
        Repeater {
            model: picker.commands.penWidths
            // Même pas que les pastilles (Theme.taillePastille) : les deux grilles s'alignent (L1-C5).
            delegate: Rectangle {
                id: preset
                required property real modelData
                required property int index
                readonly property bool current: picker.view.penWidth === modelData
                objectName: "widthPreset-" + index
                width: Theme.taillePastille
                height: Theme.taillePastille
                radius: 4
                color: current ? Theme.surfaceActive : Theme.surface
                scale: presetTap.pressed ? 0.9 : 1 // appui visible au doigt
                Accessible.role: Accessible.RadioButton
                Accessible.name: qsTr("Épaisseur %1").arg(modelData)
                Accessible.checked: current
                Accessible.onPressAction: picker.commands.trigger("pen.width", preset.modelData)
                Rectangle {
                    anchors.centerIn: parent
                    width: Math.min(Theme.taillePastille - 4, 3 + preset.modelData * 1.5)
                    height: width
                    radius: width / 2
                    color: preset.current ? Theme.texteSurActive : Theme.texte
                }
                TapHandler {
                    id: presetTap
                    margin: Theme.espace1 / 2 // cible contiguë : le pas de la rangée (24 px)
                    onTapped: picker.commands.trigger("pen.width", preset.modelData)
                }
            }
        }
    }

    Slider {
        id: slider
        objectName: "widthSlider"
        Layout.fillWidth: true
        from: 0
        to: 1
        value: picker.widthPosition
        onMoved: {
            const width = Math.round(Math.pow(picker.maxWidth, value))
            if (width !== Math.round(picker.view.penWidth))
                picker.commands.trigger("pen.width", width)
        }
        // Le glissement peut casser la liaison : on la rend au stylo au relâché.
        onPressedChanged: if (!pressed) value = Qt.binding(() => picker.widthPosition)
        Accessible.name: qsTr("Épaisseur du stylo")

        // Dessinée d'après le thème (L1-C5) : le style Basic remplissait la partie parcourue d'un gris presque
        // invisible sur la palette et laissait claire la partie restante, la valeur se lisait à l'envers.
        implicitHeight: Theme.tailleBouton
        background: Rectangle {
            x: slider.leftPadding
            y: slider.topPadding + (slider.availableHeight - height) / 2
            width: slider.availableWidth
            height: 6
            radius: 3
            color: Theme.surfaceSurvol
            Rectangle {
                width: slider.visualPosition * parent.width
                height: parent.height
                radius: 3
                color: Theme.surfaceActive
            }
        }
        handle: Rectangle {
            x: slider.leftPadding + slider.visualPosition * (slider.availableWidth - width)
            y: slider.topPadding + (slider.availableHeight - height) / 2
            implicitWidth: Theme.taillePastille
            implicitHeight: Theme.taillePastille
            radius: width / 2
            color: slider.pressed ? Theme.surfaceActive : Theme.texte
            // Focus clavier (Tab) : anneau d'accent détaché de la poignée, sur la palette (6,30:1, WCAG 2.4.7).
            Rectangle {
                visible: slider.visualFocus
                anchors.fill: parent
                anchors.margins: -4
                radius: width / 2
                color: "transparent"
                border.width: 2
                border.color: Theme.surfaceActive
            }
        }
    }

    // Échantillon posé sur un puits couleur de page : l'encre se voit comme sur la page, y compris le noir par
    // défaut, invisible sur la palette sombre (L1-C5).
    Item {
        Layout.fillWidth: true
        implicitHeight: 40
        Rectangle {
            anchors.centerIn: parent
            width: parent.implicitHeight
            height: width
            radius: Theme.rayon
            color: Theme.page
        }
        Rectangle {
            objectName: "widthIndicatorSample"
            anchors.centerIn: parent
            width: 4 + 32 * picker.widthPosition
            height: width
            radius: width / 2
            color: picker.view.penColor
            border.width: 1
            border.color: Theme.separateur
        }
    }
}
