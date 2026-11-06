import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import ActCraft.Ui

// Onglet « Sélection » du navigateur d'actions (F-850, F-851) : catégories, liste des actions du catalogue,
// zone des propriétés repliée au départ (bouton « + »), « Appliquer » et « Retirer les actions existantes ».
// Le modèle est ActionBrowserModel (L8-T11) ; la liste est grisée tant qu'aucun objet n'est sélectionné.
Item {
    id: root
    objectName: "actionBrowser"
    property Commands commands: null
    // Modèle de l'onglet : posé par le panneau des navigateurs (BrowserPanel.actionModel).
    property var model: null
    property bool expanded: false // zone des propriétés (V17) : repliée au départ

    function setParameter(name, value) {
        const next = Object.assign({}, root.model.parameters)
        next[name] = value
        root.model.parameters = next
    }
    readonly property bool ready: root.model !== null && root.model !== undefined

    Component.onCompleted: {
        if (!root.commands)
            return
        root.commands.setHandler("action.apply", function (arg) {
            if (!root.ready)
                return
            if (arg !== undefined && arg !== null && String(arg).length > 0)
                root.model.currentId = String(arg)
            root.model.apply()
        })
        root.commands.setHandler("action.remove", function () {
            if (root.ready)
                root.model.removeExisting()
        })
        root.commands.setHandler("action.pick-target", function () {
            if (root.ready)
                root.model.pickTarget()
        })
    }

    Rectangle {
        anchors.fill: parent
        color: Theme.surface
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: Theme.espace2
        spacing: Theme.espace2

        ComboBox {
            id: categories
            objectName: "actionCategories"
            Layout.fillWidth: true
            focusPolicy: Qt.NoFocus
            model: root.ready ? root.model.categoryLabels() : []
            currentIndex: root.ready ? root.model.category : 0
            onActivated: index => root.model.category = index
            Accessible.name: qsTr("Catégorie")
        }

        ListView {
            id: list
            objectName: "actionList"
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            enabled: root.ready && root.model.enabled // grisée sans sélection
            opacity: enabled ? 1.0 : 0.5
            model: root.ready ? root.model : null
            delegate: ItemDelegate {
                id: row
                required property string id
                required property var model
                required property string label
                required property bool existing
                width: ListView.view.width
                height: Theme.tailleBouton
                focusPolicy: Qt.NoFocus
                highlighted: root.model.currentId === row.id
                onClicked: root.model.currentId = row.id
                Accessible.name: row.label
                contentItem: RowLayout {
                    spacing: Theme.espace2
                    Image {
                        source: "qrc:/qt/qml/ActCraft/Ui/icons/" + row.model.icon + ".svg"
                        sourceSize.width: Theme.tailleIcone
                        sourceSize.height: Theme.tailleIcone
                        Layout.preferredWidth: Theme.tailleIcone
                        Layout.preferredHeight: Theme.tailleIcone
                    }
                    Text {
                        Layout.fillWidth: true
                        text: row.label
                        textFormat: Text.PlainText
                        elide: Text.ElideRight
                        color: row.highlighted ? Theme.texteSurActive : Theme.texte
                        font.pixelSize: Theme.taillePolice
                    }
                    Text { // l'action est déjà posée sur tous les objets sélectionnés
                        visible: row.existing
                        text: "✓"
                        color: row.highlighted ? Theme.texteSurActive : Theme.texteDiscret
                        font.pixelSize: Theme.taillePolice
                    }
                }
                background: Rectangle {
                    radius: Theme.rayon
                    color: row.highlighted ? Theme.surfaceActive
                           : row.hovered ? Theme.surfaceSurvol : "transparent"
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Text {
                Layout.fillWidth: true
                text: qsTr("Propriétés")
                color: Theme.texte
                font.pixelSize: Theme.taillePolice
                font.bold: true
            }
            ToolButton {
                id: toggle
                objectName: "actionPropertiesToggle"
                text: root.expanded ? "−" : "+"
                focusPolicy: Qt.NoFocus
                onClicked: root.expanded = !root.expanded
                Accessible.name: root.expanded ? qsTr("Replier les propriétés") : qsTr("Déplier les propriétés")
                contentItem: Text {
                    text: toggle.text
                    color: Theme.texte
                    font.pixelSize: Theme.taillePoliceTitre
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
            }
        }

        ColumnLayout {
            id: properties
            objectName: "actionProperties"
            visible: root.expanded
            Layout.fillWidth: true
            spacing: Theme.espace1

            Repeater {
                model: root.ready ? root.model.propertyRows : []
                delegate: RowLayout {
                    id: prop
                    required property var modelData
                    Layout.fillWidth: true
                    spacing: Theme.espace2
                    readonly property var value: root.model.parameters[prop.modelData.name]
                    Text {
                        text: prop.modelData.label
                        color: Theme.texte
                        font.pixelSize: Theme.taillePolice
                        Layout.preferredWidth: 80
                        elide: Text.ElideRight
                    }
                    Loader {
                        Layout.fillWidth: true
                        sourceComponent: {
                            const t = prop.modelData.type
                            if (t === "object")
                                return targetEditor
                            if (t === "text" || t === "url" || t === "file")
                                return textEditor
                            return numberEditor // nombre, angle, page, translucidité
                        }
                        property var row: prop.modelData
                        property var current: prop.value
                    }
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.espace2
            Button {
                objectName: "actionApply"
                text: qsTr("Appliquer")
                focusPolicy: Qt.NoFocus
                enabled: list.enabled && root.model.currentId.length > 0
                onClicked: root.model.apply()
            }
            Button {
                objectName: "actionRemove"
                Layout.fillWidth: true
                text: qsTr("Retirer les actions existantes")
                focusPolicy: Qt.NoFocus
                enabled: list.enabled
                onClicked: root.model.removeExisting()
            }
        }
    }

    Component {
        id: numberEditor
        SpinBox {
            readonly property bool bounded: row.maximum > row.minimum
            from: bounded ? row.minimum : -100000
            to: bounded ? row.maximum : 100000
            value: current === undefined ? from : Number(current)
            editable: true
            focusPolicy: Qt.StrongFocus
            onValueModified: root.setParameter(row.name, value)
        }
    }
    Component {
        id: textEditor
        TextField {
            text: current === undefined ? "" : String(current)
            selectByMouse: true
            onTextEdited: root.setParameter(row.name, text)
        }
    }
    Component {
        id: targetEditor
        Button { // le prochain clic sur la page choisit l'objet visé (outil Sélection)
            objectName: "actionPickTarget"
            focusPolicy: Qt.NoFocus
            text: root.model.picking ? qsTr("Cliquer sur l'objet…")
                  : root.model.target > 0 ? qsTr("Objet %1").arg(root.model.target) : qsTr("Choisir l'objet")
            onClicked: root.model.picking ? root.model.cancelPick() : root.model.pickTarget()
        }
    }
}
