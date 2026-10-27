import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Shapes
import ActCraft.Ui

// Caméra (F-275), L7-T28. Racine non visuelle d'une fonction à fenêtre propre (décision 4 bis) : elle porte le
// contrôleur et le Component de la fenêtre de sélection, que CameraController crée lui-même, sans parent (un
// Window enfant d'un Item prend la fenêtre principale pour parent et se réduit avec elle). La palette est un
// élément de CETTE fenêtre, avec pageArea: null : elle n'est pas une couverture de la vue principale, dont
// elle ne partage pas les coordonnées. Dans sa fenêtre, la surface de sélection lui laisse les points qui
// tombent sur elle (containmentMask), pour le stylet, le doigt et la souris.
Item {
    id: root
    visible: false
    width: 0
    height: 0
    property Commands commands: null
    property DocumentController documentController: null
    property Tools tools: null
    property Item pageArea: null
    property QtObject controller: cameraController

    CameraController {
        id: cameraController
        objectName: "cameraController"
        documentController: root.documentController
        pageView: root.pageArea ? root.pageArea.view : null
        // L'écran photographié est celui de la fenêtre principale.
        mainWindow: root.pageArea ? root.pageArea.Window.window : null
        windowComponent: captureWindow
    }

    // Capture refusée (image vide : Wayland, macOS sans autorisation d'enregistrer l'écran) : un message dans
    // la fenêtre principale, aucune fenêtre de sélection.
    Rectangle {
        id: messageBox
        objectName: "cameraMessage"
        parent: root.pageArea
        visible: cameraController.message !== ""
        z: 100
        x: parent ? (parent.width - width) / 2 : 0
        y: parent ? parent.height - height - Theme.espace4 : 0
        width: Math.min(messageText.implicitWidth, 520) + 2 * Theme.espace4
        height: messageText.implicitHeight + 2 * Theme.espace3
        radius: Theme.rayon
        color: Theme.barre
        border.color: Theme.contour

        Label {
            id: messageText
            x: Theme.espace4
            y: Theme.espace3
            width: messageBox.width - 2 * Theme.espace4
            text: cameraController.message
            textFormat: Text.PlainText
            wrapMode: Text.WordWrap
            color: Theme.texte
            font.pixelSize: Theme.taillePolice
        }

        Timer {
            running: messageBox.visible
            interval: 6000
            onTriggered: cameraController.clearMessage()
        }
    }

    Component {
        id: captureWindow

        Window {
            id: capture
            objectName: "cameraCaptureWindow"
            title: qsTr("Caméra")
            color: "black"

            Image {
                objectName: "cameraFrozenImage"
                anchors.fill: parent
                source: cameraController.frozenSource
                cache: false
                fillMode: Image.Stretch
            }

            // Voile hors de la zone choisie (zone, fenêtre).
            Item {
                id: veil
                anchors.fill: parent
                readonly property rect area: cameraController.selection
                visible: (cameraController.mode === CameraController.AreaMode
                          || cameraController.mode === CameraController.WindowMode) && area.width > 0

                Rectangle {
                    color: "#66000000"
                    width: veil.width
                    height: veil.area.y
                }
                Rectangle {
                    color: "#66000000"
                    y: veil.area.y + veil.area.height
                    width: veil.width
                    height: Math.max(0, veil.height - y)
                }
                Rectangle {
                    color: "#66000000"
                    y: veil.area.y
                    width: veil.area.x
                    height: veil.area.height
                }
                Rectangle {
                    color: "#66000000"
                    x: veil.area.x + veil.area.width
                    y: veil.area.y
                    width: Math.max(0, veil.width - x)
                    height: veil.area.height
                }
            }

            // Contour pointillé de la forme (rectangle, polygone, tracé à main levée).
            Shape {
                anchors.fill: parent
                visible: cameraController.outline.length > 1

                ShapePath {
                    strokeColor: "white"
                    strokeWidth: 2
                    strokeStyle: ShapePath.DashLine
                    dashPattern: [4, 3]
                    fillColor: "transparent"
                    PathPolyline { path: cameraController.outline }
                }
            }

            Item {
                id: surface
                objectName: "cameraSurface"
                anchors.fill: parent
                focus: true
                Keys.onEscapePressed: cameraController.cancel()
                // Les points qui tombent sur la palette sont à elle, même ceux qu'aucun de ses éléments ne prend.
                containmentMask: QtObject {
                    function contains(point: point): bool {
                        if (!palette.visible)
                            return true
                        return !palette.contains(surface.mapToItem(palette, point.x, point.y))
                    }
                }

                // Stylet, doigt et souris : appui, glisser, relâché.
                PointHandler {
                    id: pointer
                    objectName: "cameraPointer"
                    property point last
                    onActiveChanged: {
                        if (active) {
                            last = point.pressPosition
                            cameraController.press(point.pressPosition)
                        } else {
                            cameraController.release(last)
                        }
                    }
                    onPointChanged: {
                        if (active) {
                            last = point.position
                            cameraController.move(point.position)
                        }
                    }
                }

                // Capture d'une fenêtre : la fenêtre sous le pointeur est encadrée.
                HoverHandler {
                    id: hover
                    onPointChanged: cameraController.hover(hover.point.position)
                }
            }

            CameraPalette {
                id: palette
                objectName: "cameraPalette"
                controller: cameraController
                x: Math.max(0, (capture.width - width) / 2)
                y: Theme.espace4
                onClosed: cameraController.cancel()
            }
        }
    }

    function registerCommands() {
        if (!root.commands)
            return
        root.commands.setHandler("tool.camera-area", function () {
            return cameraController.start(CameraController.AreaMode)
        })
        root.commands.setHandler("tool.camera-polygon", function () {
            return cameraController.start(CameraController.PolygonMode)
        })
        root.commands.setHandler("tool.camera-freehand", function () {
            return cameraController.start(CameraController.FreehandMode)
        })
        root.commands.setHandler("tool.camera-window", function () {
            return cameraController.start(CameraController.WindowMode)
        })
        root.commands.setHandler("tool.camera-screen", function () {
            return cameraController.start(CameraController.ScreenMode)
        })
    }

    onCommandsChanged: registerCommands()
    Component.onCompleted: registerCommands()
}
