import QtQuick
import QtQuick.Controls.Basic
import ActCraft.Ui

// Boîte « Fond de la page » (F-610) : couleur unie ou dégradé à deux couleurs (couleurs et sens). Trois
// boutons (D15) : « Annuler » ferme sans rien changer de plus (ce qui a été appliqué reste, Annuler de la
// barre le défait), « Appliquer » pose le fond sur la page affichée en une étape et laisse la boîte ouverte,
// « Valider » applique ce qui ne l'est pas encore puis ferme. L'image de fond viendra avec l'insertion de
// ressources (lot 7) : aucune ressource n'est offerte ici (H4-18). Géométrie fixe : le scénario clique dedans.
Popup {
    id: dialog

    // Réglages de la page (appPageSettings), donnés par la fenêtre principale.
    property PageSettings settings: null
    // Choix en cours : « color » ou « gradient », deux couleurs, sens du dégradé.
    property string kind: "color"
    property color color: "#ffffff"
    property color color2: "#ffffff"
    property string direction: "vertical"
    // Vrai tant que le choix affiché est celui de la page (rien à appliquer : « appliquer » ne fait rien).
    property bool applied: true

    readonly property var swatches: ["#FFFFFF", "#000000", "#FFFF00", "#FF0000", "#00AA00", "#0066FF", "#FF8800",
                                     "#8800CC", "#F5F5DC", "#CCE5FF", "#FFE0E0", "#E0FFE0", "#FFF3B0", "#D9D9D9",
                                     "#404040", "#003366"]

    // Un choix identique à l'actuel n'invalide rien (« appliquer » resterait sans objet).
    function chooseKind(value) { if (kind !== value) { kind = value; applied = false } }
    function chooseColor(value) { if (!Qt.colorEqual(color, value)) { color = value; applied = false } }
    function chooseColor2(value) { if (!Qt.colorEqual(color2, value)) { color2 = value; applied = false } }
    function chooseDirection(value) { if (direction !== value) { direction = value; applied = false } }

    function loadFromPage() {
        const page = settings ? settings.background : ({})
        // « other » (aucun fond, motif) et un sens non offert (diagonale, radial) sont gardés tels que lus.
        kind = page.kind !== undefined ? page.kind : "color"
        color = page.color !== undefined ? page.color : "#ffffff"
        color2 = page.color2 !== undefined ? page.color2 : color
        direction = page.gradient !== undefined ? page.gradient : "vertical"
        applied = true
    }
    function apply() {
        if (!settings || applied)
            return
        settings.setBackground({ kind: kind, color: color, color2: color2, gradient: direction })
        applied = true
    }
    function accept() {
        apply()
        close()
    }

    modal: true
    focus: true
    anchors.centerIn: Overlay.overlay
    width: 440
    height: 330
    padding: Theme.espace4
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
    enter: null
    exit: null
    onAboutToShow: loadFromPage()
    // Pas d'assombrissement : la page, derrière, montre le fond appliqué tel qu'il sera.
    Overlay.modal: Rectangle { color: "transparent" }

    background: Rectangle {
        color: Theme.palette
        radius: Theme.rayon
        border.color: Theme.contour
    }

    // Une rangée de pastilles : 8 par ligne, 32 px, 8 px d'écart.
    component SwatchGrid: Grid {
        id: grid
        property color current: "#ffffff"
        property string prefix: "backgroundSwatch"
        property var swatches: []
        signal picked(string value)
        columns: 8
        spacing: 8
        Repeater {
            model: grid.swatches
            delegate: Rectangle {
                id: swatch
                required property string modelData
                required property int index
                objectName: grid.prefix + index
                width: 32
                height: 32
                radius: 3
                color: modelData
                border.width: Qt.colorEqual(grid.current, modelData) ? 3 : 1
                border.color: Qt.colorEqual(grid.current, modelData) ? Theme.cadreSelection : Theme.contour
                Accessible.role: Accessible.RadioButton
                Accessible.name: qsTr("Couleur %1").arg(modelData)
                TapHandler { onTapped: grid.picked(swatch.modelData) }
            }
        }
    }

    contentItem: Item {
        Label {
            id: title
            x: 0
            y: 0
            height: 24
            text: qsTr("Fond de la page")
            color: Theme.texte
            font.pixelSize: Theme.taillePolice
            font.bold: true
        }
        Row {
            id: kindRow
            y: 36
            spacing: 8
            Button {
                objectName: "backgroundKindColor"
                width: 100
                height: 36
                text: qsTr("Couleur")
                highlighted: dialog.kind === "color"
                onClicked: dialog.chooseKind("color")
            }
            Button {
                objectName: "backgroundKindGradient"
                width: 100
                height: 36
                text: qsTr("Dégradé")
                highlighted: dialog.kind === "gradient"
                onClicked: dialog.chooseKind("gradient")
            }
            Button {
                objectName: "backgroundVertical"
                visible: dialog.kind === "gradient"
                width: 60
                height: 36
                text: qsTr("Haut-bas")
                highlighted: dialog.direction === "vertical"
                onClicked: dialog.chooseDirection("vertical")
            }
            Button {
                objectName: "backgroundHorizontal"
                visible: dialog.kind === "gradient"
                width: 60
                height: 36
                text: qsTr("Côtés")
                highlighted: dialog.direction === "horizontal"
                onClicked: dialog.chooseDirection("horizontal")
            }
        }
        SwatchGrid {
            y: 84
            swatches: dialog.swatches
            current: dialog.color
            onPicked: value => dialog.chooseColor(value)
        }
        SwatchGrid {
            y: 168
            visible: dialog.kind === "gradient"
            prefix: "backgroundSwatchB"
            swatches: dialog.swatches
            current: dialog.color2
            onPicked: value => dialog.chooseColor2(value)
        }
        Row {
            x: parent.width - width
            y: parent.height - height
            spacing: 8
            Button {
                objectName: "backgroundCancel"
                width: 100
                height: 40
                text: qsTr("Annuler")
                onClicked: dialog.close()
            }
            Button {
                objectName: "backgroundApply"
                width: 100
                height: 40
                text: qsTr("Appliquer")
                onClicked: dialog.apply()
            }
            Button {
                objectName: "backgroundOk"
                width: 100
                height: 40
                text: qsTr("Valider")
                onClicked: dialog.accept()
            }
        }
    }
}
