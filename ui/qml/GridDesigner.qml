import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import ActCraft.Ui

// Créateur de grille (F-620, F-621) : fenêtre propre à ActCraft, cadre « Grille », cadre des niveaux, niveau
// courant. Chaque changement est un réglage de la grille de la page affichée (aperçu immédiat) ; la séance
// entière, de l'ouverture à la fermeture, ne fait qu'une étape d'annulation (PageSettings.beginDesigner et
// endDesigner). Ouverture : open() de la fenêtre.
Popup {
    id: designer

    // Réglages de la page (appPageSettings), donnés par la fenêtre principale.
    property PageSettings settings: null
    // Rang du niveau édité.
    property int level: 0

    readonly property var grid: settings ? settings.grid : ({})
    readonly property var levels: grid.levels ? grid.levels : []
    readonly property var current: level >= 0 && level < levels.length ? levels[level] : null
    readonly property var swatches: ["#000000", "#808080", "#D32F2F", "#2E7D32", "#1565C0", "#EF6C00"]

    modal: true
    focus: true
    anchors.centerIn: Overlay.overlay
    width: 560
    padding: Theme.espace4
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
    enter: null
    exit: null

    background: Rectangle {
        color: Theme.palette
        radius: Theme.rayon
        border.color: Theme.contour
    }

    // Copie modifiable de la grille : l'objet lu sur PageSettings ne se modifie pas en place côté JS.
    function editableGrid() {
        var g = Object.assign({}, settings.grid)
        var list = []
        var src = settings.grid.levels
        for (var i = 0; i < src.length; ++i) {
            list.push(Object.assign({}, src[i]))
        }
        g.levels = list
        return g
    }
    // Écrit une clé du niveau édité.
    function patchLevel(name, value) {
        if (!settings || !current) {
            return
        }
        var g = editableGrid()
        g.levels[level][name] = value
        settings.setGrid(g)
    }
    function patchGrid(name, value) {
        if (!settings) {
            return
        }
        var g = editableGrid()
        g[name] = value
        settings.setGrid(g)
    }

    function begin() {
        if (!settings) {
            return
        }
        settings.beginDesigner()
        level = 0
        // Une page sans grille visible en montre une, avec au moins un niveau : l'aperçu est immédiat.
        if (levels.length === 0 || !grid.visible) {
            var g = editableGrid()
            g.visible = true
            if (levels.length === 0) {
                g.levels = [{}]
            }
            settings.setGrid(g)
        }
        aboveCheck.forceActiveFocus()
    }
    function end() {
        if (settings) {
            settings.endDesigner()
        }
    }
    // Séance du créateur : de l'apparition à la disparition de la fenêtre.
    onVisibleChanged: {
        if (visible) {
            begin()
        } else {
            end()
        }
    }
    onLevelsChanged: {
        if (level >= levels.length && levels.length > 0) {
            level = levels.length - 1
        }
    }

    contentItem: ColumnLayout {
        spacing: Theme.espace3
        // Entrée ferme la fenêtre (Échap est déjà le raccourci de la commande « sélectionner » : il la
        // laisse ouverte quand le raccourci est joué en premier).
        Keys.onReturnPressed: designer.close()
        Keys.onEnterPressed: designer.close()

        // Cadre « Grille » : réglages de la page.
        GroupBox {
            title: qsTr("Grille")
            Layout.fillWidth: true
            label: Label { text: parent.title; color: Theme.texte; font.bold: true }
            background: Rectangle { color: "transparent"; border.color: Theme.separateur; radius: Theme.rayon; y: 12; height: parent.height - 12 }

            ColumnLayout {
                anchors.fill: parent
                CheckBox {
                    id: aboveCheck
                    objectName: "gridAboveCheck"
                    text: qsTr("Dessinée au-dessus des objets")
                    palette.windowText: Theme.texte
                    onToggled: designer.patchGrid("above", checked)
                    Binding { target: aboveCheck; property: "checked"; value: !!designer.grid.above; restoreMode: Binding.RestoreNone }
                }
                CheckBox {
                    id: visibleCheck
                    objectName: "gridVisibleCheck"
                    text: qsTr("Visible")
                    palette.windowText: Theme.texte
                    onToggled: designer.patchGrid("visible", checked)
                    Binding { target: visibleCheck; property: "checked"; value: !!designer.grid.visible; restoreMode: Binding.RestoreNone }
                }
                CheckBox {
                    id: snapCheck
                    objectName: "gridSnapCheck"
                    text: qsTr("Accrochage autorisé")
                    palette.windowText: Theme.texte
                    onToggled: designer.patchGrid("snap", checked)
                    Binding { target: snapCheck; property: "checked"; value: !!designer.grid.snap; restoreMode: Binding.RestoreNone }
                }
                RowLayout {
                    spacing: Theme.espace2
                    Label { text: qsTr("Taille des cases"); color: Theme.texte }
                    Button {
                        objectName: "gridSmaller"
                        text: "−"
                        onClicked: designer.settings.stepSpacing(-1)
                    }
                    Button {
                        objectName: "gridBigger"
                        text: "+"
                        onClicked: designer.settings.stepSpacing(1)
                    }
                    Button {
                        objectName: "gridReset"
                        text: qsTr("Réinitialiser")
                        onClicked: designer.settings.resetGrid()
                    }
                }
            }
        }

        // Cadre des niveaux : liste, ajout, retrait.
        GroupBox {
            title: qsTr("Niveaux")
            Layout.fillWidth: true
            label: Label { text: parent.title; color: Theme.texte; font.bold: true }
            background: Rectangle { color: "transparent"; border.color: Theme.separateur; radius: Theme.rayon; y: 12; height: parent.height - 12 }

            RowLayout {
                anchors.fill: parent
                spacing: Theme.espace2
                Repeater {
                    model: designer.levels.length
                    delegate: Button {
                        required property int index
                        objectName: "gridLevelButton" + index
                        text: qsTr("Niveau %1").arg(index + 1)
                        // Pas à bascule : un second clic sur le niveau courant ne le décoche pas.
                        highlighted: designer.level === index
                        onClicked: designer.level = index
                    }
                }
                Item { Layout.fillWidth: true }
                Button {
                    objectName: "gridAddLevel"
                    text: qsTr("Ajouter")
                    onClicked: designer.settings.addLevel()
                }
                Button {
                    objectName: "gridRemoveLevel"
                    text: qsTr("Retirer")
                    enabled: designer.levels.length > 1
                    onClicked: designer.settings.removeLevel(designer.level)
                }
            }
        }

        // Niveau courant.
        GroupBox {
            title: qsTr("Niveau courant")
            Layout.fillWidth: true
            enabled: designer.current !== null
            label: Label { text: parent.title; color: Theme.texte; font.bold: true }
            background: Rectangle { color: "transparent"; border.color: Theme.separateur; radius: Theme.rayon; y: 12; height: parent.height - 12 }

            GridLayout {
                anchors.fill: parent
                columns: 4
                columnSpacing: Theme.espace3

                Label { text: qsTr("Pas horizontal"); color: Theme.texte }
                SpinBox {
                    id: spacingX
                    objectName: "gridSpacingX"
                    from: 1; to: 4096; editable: true
                    onValueModified: designer.patchLevel("spacingX", value)
                    Binding { target: spacingX; property: "value"; value: designer.current ? Math.round(designer.current.spacingX) : 32; restoreMode: Binding.RestoreNone }
                }
                Label { text: qsTr("Pas vertical"); color: Theme.texte }
                SpinBox {
                    id: spacingY
                    objectName: "gridSpacingY"
                    from: 1; to: 4096; editable: true
                    onValueModified: designer.patchLevel("spacingY", value)
                    Binding { target: spacingY; property: "value"; value: designer.current ? Math.round(designer.current.spacingY) : 32; restoreMode: Binding.RestoreNone }
                }

                Label { text: qsTr("Style"); color: Theme.texte }
                ComboBox {
                    id: patternBox
                    objectName: "gridPattern"
                    model: [qsTr("Normal"), qsTr("Diagonal à 45°"), qsTr("Angle libre")]
                    onActivated: designer.patchLevel("pattern", currentIndex)
                    Binding { target: patternBox; property: "currentIndex"; value: designer.current ? designer.current.pattern : 0; restoreMode: Binding.RestoreNone }
                }
                Label { text: qsTr("Ligne"); color: Theme.texte }
                ComboBox {
                    id: lineBox
                    objectName: "gridLine"
                    model: [qsTr("Points"), qsTr("Croix"), qsTr("Trait plein"), qsTr("Pointillés"), qsTr("Tirets"), qsTr("Gros points")]
                    onActivated: designer.patchLevel("line", currentIndex)
                    Binding { target: lineBox; property: "currentIndex"; value: designer.current ? designer.current.line : 2; restoreMode: Binding.RestoreNone }
                }

                // Angle libre (style 2) : la grille s'incline de cet angle ; borne de l'accrochage (1 à 89).
                Label { text: qsTr("Angle"); color: Theme.texte; visible: angleBox.visible }
                SpinBox {
                    id: angleBox
                    objectName: "gridAngle"
                    from: 1; to: 89; editable: true
                    visible: !!designer.current && designer.current.pattern === 2
                    onValueModified: designer.patchLevel("angle", value)
                    Binding { target: angleBox; property: "value"; value: designer.current ? Math.round(designer.current.angle) : 30; restoreMode: Binding.RestoreNone }
                }
                Item { visible: angleBox.visible; Layout.columnSpan: 2 }

                Label { text: qsTr("Couleur"); color: Theme.texte }
                RowLayout {
                    Layout.columnSpan: 3
                    spacing: Theme.espace2
                    Repeater {
                        model: designer.swatches
                        delegate: Rectangle {
                            required property string modelData
                            required property int index
                            objectName: "gridSwatch" + index
                            width: 28; height: 28; radius: 4
                            color: modelData
                            border.width: designer.current && Qt.colorEqual(designer.current.color, modelData) ? 3 : 1
                            border.color: designer.current && Qt.colorEqual(designer.current.color, modelData) ? Theme.surfaceActive : Theme.contour
                            MouseArea {
                                anchors.fill: parent
                                onClicked: designer.patchLevel("color", parent.color)
                            }
                        }
                    }
                }
            }
        }

        Button {
            objectName: "gridClose"
            text: qsTr("Fermer")
            Layout.alignment: Qt.AlignRight
            onClicked: designer.close()
        }
    }
}
