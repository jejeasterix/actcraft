import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import ActCraft.Ui

// Onglet Format (F-900) : tout passe par les setters du profil, lus par root.profile.
Item {
    id: root
    objectName: "formatPage"
    property Commands commands: null
    property DocumentController documentController: null
    property Tools tools: null
    property PageView view: null
    property var profile: null
    property var preferences: null
    implicitWidth: 460
    implicitHeight: column.implicitHeight

    function flag(section, key) {
        return root.profile ? root.profile.setting(section, key, false) === true : false
    }

    ColumnLayout {
        id: column
        anchors.fill: parent
        spacing: 6

        Label { text: qsTr("Taille des icônes : %1 px").arg(iconSlider.value); font.bold: true }
        Slider {
            id: iconSlider
            objectName: "iconPixelsSlider"
            Layout.fillWidth: true
            from: 16; to: 32; stepSize: 1
            onMoved: root.profile.iconPixels = Math.round(value)
            // Focus clavier : le style n'entoure pas la réglette, sa poignée seule change à peine (L11-T24).
            FocusRing {
                visible: parent.visualFocus
                radius: Theme.rayon
            }
        }
        Binding {
            target: iconSlider; property: "value"
            value: root.profile ? root.profile.iconPixels : 24
        }

        Label { text: qsTr("Palette d'outils"); font.bold: true }
        RowLayout {
            Label { text: qsTr("Ancrage") }
            ComboBox {
                objectName: "paletteDockCombo"
                textRole: "text"
                valueRole: "value"
                model: [{ value: "left", text: qsTr("Gauche") }, { value: "right", text: qsTr("Droite") },
                        { value: "top", text: qsTr("Haut") }, { value: "bottom", text: qsTr("Bas") }]
                currentIndex: root.profile ? Math.max(0, indexOfValue(root.profile.paletteDock)) : 0
                onActivated: root.profile.paletteDock = currentValue
            }
            Label { text: qsTr("Couleurs") }
            ComboBox {
                objectName: "swatchesCombo"
                model: [8, 16, 24]
                currentIndex: root.profile ? Math.max(0, model.indexOf(root.profile.swatches)) : 1
                onActivated: root.profile.swatches = model[currentIndex]
            }
        }
        CheckBox {
            objectName: "palettePinnedBox"; text: qsTr("Palette épinglée")
            checked: root.profile ? root.profile.palettePinned : true
            onClicked: root.profile.palettePinned = checked
        }
        CheckBox {
            objectName: "paletteCollapsedBox"; text: qsTr("Palette réduite")
            checked: root.profile ? root.profile.paletteCollapsed : false
            onClicked: root.profile.paletteCollapsed = checked
        }
        CheckBox {
            objectName: "paletteCollapsedMaxBox"; text: qsTr("Palette réduite au maximum")
            checked: root.profile ? root.flag("palette", "collapsedMax") : false
            onClicked: root.profile.setSetting("palette", "collapsedMax", checked)
        }

        Label { text: qsTr("Navigateurs"); font.bold: true }
        RowLayout {
            Label { text: qsTr("Côté") }
            ComboBox {
                objectName: "browsersSideCombo"
                textRole: "text"
                valueRole: "value"
                model: [{ value: "left", text: qsTr("Gauche") }, { value: "right", text: qsTr("Droite") }]
                currentIndex: root.profile ? Math.max(0, indexOfValue(root.profile.browsersSide)) : 0
                onActivated: root.profile.browsersSide = currentValue
            }
        }
        CheckBox {
            objectName: "browsersPinnedBox"; text: qsTr("Navigateurs épinglés")
            checked: root.profile ? root.profile.browsersPinned : true
            onClicked: root.profile.browsersPinned = checked
        }
        CheckBox {
            objectName: "browsersVisibleBox"; text: qsTr("Navigateurs visibles")
            checked: root.profile ? root.profile.browsersVisible : false
            onClicked: root.profile.browsersVisible = checked
        }

        CheckBox {
            objectName: "trashVisibleBox"; text: qsTr("Corbeille visible")
            checked: root.profile ? root.profile.trashVisible : true
            onClicked: root.profile.trashVisible = checked
        }
        CheckBox {
            objectName: "fullScreenBox"; text: qsTr("Plein écran au démarrage")
            checked: root.profile ? root.flag("format", "fullScreenAtStart") : false
            onClicked: root.profile.setSetting("format", "fullScreenAtStart", checked)
        }
    }
}
