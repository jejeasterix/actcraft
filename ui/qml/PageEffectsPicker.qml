import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import ActCraft.Ui

// Navigateur d'effets de changement de page (F-640, L7-T8) : un panneau flottant (couverture de la page, voir
// FloatingPanel) avec une vignette par entrée du catalogue du contrôleur d'effets (`transitions`). Chaque
// vignette joue son mouvement au survol, sur deux pages unies dessinées ici, avec les pièces que donne
// `transitions.motion(id)` : aucune image, aucun libellé étranger. Un appui choisit l'effet global ; avec la
// case « pour cette page seulement », il s'écrit dans la page affichée (page.effect). « Terminé » ferme.
// La commande view.page-effects ouvre le panneau.
FloatingPanel {
    id: root
    property Commands commands: null
    property DocumentController documentController: null
    property Tools tools: null
    property QtObject controller: null
    property QtObject transitions: null

    readonly property int thumbWidth: 96
    readonly property int thumbHeight: 60
    readonly property int cellWidth: thumbWidth + Theme.espace2
    readonly property int cellHeight: thumbHeight + 2 * Theme.taillePolice + Theme.espace4
    // Hauteur offerte à la grille : le reste de la fenêtre moins le titre, l'astuce et le pied.
    readonly property real gridRoom: root.parent ? Math.max(cellHeight, root.parent.height - 4 * Theme.tailleBouton
                                                            - 6 * Theme.espace2) : cellHeight

    title: qsTr("Effets de changement de page")
    visible: false
    width: root.parent ? Math.min(7 * root.cellWidth + 2 * Theme.espace4, root.parent.width - 2 * Theme.espace4)
                       : 7 * root.cellWidth
    height: Theme.tailleBouton + 2 * Theme.espace2 + layout.implicitHeight

    function openPicker() {
        if (root.parent) {
            root.x = Math.max(0, (root.parent.width - root.width) / 2)
            root.y = Math.max(0, (root.parent.height - root.height) / 2)
        }
        root.visible = true
    }

    function choose(id) {
        if (!root.transitions) {
            return
        }
        if (pageOnly.checked) {
            root.transitions.setPageEffect(id)
        } else {
            root.transitions.globalEffect = id
        }
    }

    function registerCommands() {
        if (!root.commands) {
            return
        }
        root.commands.setHandler("view.page-effects", function () { root.openPicker() })
    }

    Component.onCompleted: Qt.callLater(root.registerCommands)

    ColumnLayout {
        id: layout
        anchors.left: parent.left
        anchors.right: parent.right
        spacing: Theme.espace2

        Label {
            objectName: "effectsHint"
            Layout.fillWidth: true
            text: qsTr("Survolez une vignette pour voir l'effet, touchez-la pour le choisir.")
            textFormat: Text.PlainText
            wrapMode: Text.WordWrap
            color: Theme.texteDiscret
            font.pixelSize: Theme.taillePolice
        }

        Flickable {
            id: flick
            objectName: "effectsFlick"
            Layout.fillWidth: true
            Layout.preferredHeight: Math.min(grid.implicitHeight, root.gridRoom)
            contentWidth: width
            contentHeight: grid.implicitHeight
            clip: true
            boundsBehavior: Flickable.StopAtBounds

            Flow {
                id: grid
                width: flick.width
                spacing: Theme.espace1

                Repeater {
                    model: root.transitions ? root.transitions.catalogue : []

                    delegate: Item {
                        id: cell
                        required property var modelData
                        readonly property string effectId: String(modelData.id)
                        readonly property bool current: root.transitions
                                                        ? root.transitions.globalEffect === cell.effectId : false
                        readonly property var plan: root.transitions ? root.transitions.motion(cell.effectId) : []
                        readonly property bool previewing: hover.hovered && cell.plan.length > 0 && root.visible
                        property real t: 0 // avancement du mouvement, 0 à 1

                        objectName: "effect-" + cell.effectId
                        width: root.cellWidth
                        height: root.cellHeight
                        Accessible.role: Accessible.Button
                        Accessible.name: String(modelData.label)

                        HoverHandler {
                            id: hover
                        }

                        TapHandler {
                            objectName: "effectTap"
                            onTapped: root.choose(cell.effectId)
                        }

                        SequentialAnimation {
                            running: cell.previewing
                            loops: Animation.Infinite
                            onRunningChanged: if (!running) cell.t = 0
                            NumberAnimation {
                                target: cell; property: "t"; from: 0; to: 1; duration: 900
                                easing.type: Easing.InOutQuad
                            }
                            PauseAnimation { duration: 450 }
                        }

                        Rectangle {
                            id: stage
                            x: Theme.espace1
                            y: Theme.espace1
                            width: root.thumbWidth
                            height: root.thumbHeight
                            radius: 3
                            color: "#FFFFFF" // la page d'arrivée
                            border.width: cell.current ? 2 : 1
                            border.color: cell.current ? Theme.surfaceActive : Theme.contour
                            clip: true

                            // La page quittée, entière au repos.
                            Rectangle {
                                anchors.fill: parent
                                color: "#F2B880"
                                visible: !cell.previewing
                                Rectangle { x: 10; y: 14; width: parent.width - 36; height: 3; color: "#B5651D" }
                                Rectangle { x: 10; y: 26; width: parent.width - 52; height: 3; color: "#B5651D" }
                                Rectangle { x: 10; y: 38; width: parent.width - 28; height: 3; color: "#B5651D" }
                            }

                            // Aucun mouvement (sans effet, au hasard) : un signe tient lieu d'aperçu.
                            Label {
                                anchors.centerIn: parent
                                visible: cell.plan.length === 0
                                text: cell.effectId === "random" ? "?" : "–"
                                color: Theme.texteSurActive
                                font.pixelSize: Theme.taillePoliceTitre * 2
                            }

                            Repeater {
                                model: cell.previewing ? cell.plan : []

                                delegate: Rectangle {
                                    id: piece
                                    required property var modelData
                                    readonly property real progress: Math.min(1, cell.t / Math.max(0.01,
                                                                                                    modelData.span))
                                    x: modelData.x * stage.width + modelData.dx * stage.width * progress
                                    y: modelData.y * stage.height + modelData.dy * stage.height * progress
                                    width: modelData.width * stage.width
                                    height: modelData.height * stage.height
                                    transformOrigin: modelData.origin
                                    scale: 1 + (modelData.scale - 1) * progress
                                    opacity: 1 + (modelData.opacity - 1) * progress
                                    color: "#F2B880"
                                    border.width: 1
                                    border.color: "#B5651D"
                                }
                            }
                        }

                        Label {
                            anchors.left: parent.left
                            anchors.right: parent.right
                            anchors.top: stage.bottom
                            anchors.topMargin: Theme.espace1
                            text: String(modelData.label)
                            textFormat: Text.PlainText
                            horizontalAlignment: Text.AlignHCenter
                            wrapMode: Text.WordWrap
                            maximumLineCount: 2
                            elide: Text.ElideRight
                            color: cell.current ? Theme.surfaceActive : Theme.texte
                            font.pixelSize: Theme.taillePolice
                        }
                    }
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.espace2

            CheckBox {
                id: pageOnly
                objectName: "effectsPageOnly"
                text: qsTr("Pour cette page seulement")
                font.pixelSize: Theme.taillePolice
            }

            Item {
                Layout.fillWidth: true
            }

            Button {
                objectName: "effectsDone"
                text: qsTr("Terminé")
                font.pixelSize: Theme.taillePolice
                Layout.preferredHeight: Theme.tailleBouton
                onClicked: root.close()
            }
        }
    }
}
