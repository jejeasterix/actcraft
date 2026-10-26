import QtQuick
import QtQuick.Controls.Basic
import ActCraft.Ui

// Annotation du bureau (F-070) et fonds de bureau (F-071), L7-T30. Racine non visuelle d'une fonction à fenêtre
// propre (décision 4 bis) : elle porte le contrôleur et le Component de la fenêtre, que DesktopController crée
// lui-même (format à canal alpha posé avant la création native). La zone de la fenêtre écrit dans le paperboard
// du bureau (desktopController.desktopDocument), JAMAIS dans root.documentController : celui-là est le document
// ouvert, que Presentation.qml passe à toutes les racines ; il n'est ici que la cible de la capture hors
// annotation. tools: null : le stylo dessine sans les outils de l'utilisateur (pageAreaWithoutToolsDraws), à la
// couleur et à l'épaisseur de la vue principale.
Item {
    id: root
    visible: false
    width: 0
    height: 0
    property Commands commands: null
    property DocumentController documentController: null
    property Tools tools: null
    property Item pageArea: null
    property QtObject controller: desktopController

    DesktopController {
        id: desktopController
        objectName: "desktopController"
        openDocument: root.documentController
        windowComponent: annotationWindow
    }

    Component {
        id: annotationWindow

        Window {
            id: annotation
            objectName: "desktopAnnotationWindow"
            title: qsTr("Annotation du bureau")
            color: "transparent"
            // Comme Main.qml pour le document ouvert : le paperboard du bureau montre sa page dans cette zone
            // (détaché par DesktopController à la fermeture).
            Component.onCompleted: desktopController.desktopDocument.fixedPage = desktopArea.fixedPage

            PageArea {
                id: desktopArea
                objectName: "desktopPageArea"
                anchors.fill: parent
                tools: null
                commands: null
                documentController: desktopController.desktopDocument
            }

            // Couleur et épaisseur de la palette principale (sans toucher aux gestionnaires de Main.qml).
            Binding {
                target: desktopArea.view
                property: "inkColor"
                when: root.pageArea !== null
                value: root.pageArea ? root.pageArea.view.inkColor : "black"
            }
            Binding {
                target: desktopArea.view
                property: "inkWidth"
                when: root.pageArea !== null
                value: root.pageArea ? root.pageArea.view.inkWidth : 2
            }

            // Barre de la fenêtre : couverture de la vue (un trait qui y commence n'est pas versé).
            Rectangle {
                id: toolbar
                objectName: "desktopToolbar"
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                anchors.margins: 16
                width: buttons.implicitWidth + 24
                height: buttons.implicitHeight + 24
                radius: 6
                color: "#F2FFFFFF"
                border.width: 1
                border.color: Theme.contour

                MouseArea {
                    anchors.fill: parent
                }

                Row {
                    id: buttons
                    anchors.centerIn: parent
                    spacing: 8

                    Button {
                        objectName: "desktopPageBackButton"
                        text: qsTr("Page précédente")
                        onClicked: desktopController.pageBack()
                    }
                    Button {
                        objectName: "desktopNextPageButton"
                        text: qsTr("Page suivante")
                        onClicked: desktopController.nextPage()
                    }
                    Button {
                        objectName: "desktopPassThroughButton"
                        text: qsTr("Laisser passer les clics")
                        highlighted: desktopController.passThrough
                        onClicked: desktopController.passThrough = !desktopController.passThrough
                    }
                    Button {
                        objectName: "desktopCloseButton"
                        text: qsTr("Fermer")
                        onClicked: desktopController.annotating = false
                    }
                }

                Component.onCompleted: desktopArea.view.addInputCover(toolbar)
            }
        }
    }

    function syncChecks() {
        if (!root.commands) {
            return
        }
        root.commands.setChecked("desktop.annotate", desktopController.annotating)
        root.commands.setChecked("page.background-desktop-overlay", desktopController.liveDesktopBackground)
    }

    function registerCommands() {
        if (!root.commands) {
            return
        }
        root.commands.setHandler("desktop.annotate", function () {
            desktopController.toggle() // la coche suit annotatingChanged
        })
        root.commands.setHandler("page.background-desktop-capture", function () {
            desktopController.captureBackground()
        })
        root.commands.setHandler("page.background-desktop-overlay", function () {
            desktopController.toggleLiveDesktopBackground()
        })
        syncChecks()
    }

    Connections {
        target: desktopController
        function onAnnotatingChanged() {
            root.syncChecks()
        }
        function onLiveDesktopBackgroundChanged() {
            root.syncChecks()
        }
    }
    onCommandsChanged: registerCommands()

    Component.onCompleted: registerCommands()
}
