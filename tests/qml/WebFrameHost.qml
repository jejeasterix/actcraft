import QtQuick
import ActCraft.Ui

// Zone de page minimale : ce que WebFrameLayer lit de PageArea, sans page ni encre.
Item {
    id: host
    objectName: "pageArea"
    width: 1024
    height: 768
    property DocumentController documentController: null
    property bool inputBlocked: false
    property var commands: null // posé avant la fin de la création : le gestionnaire s'inscrit à ce moment
    property var profile: null
    signal popupOpened()
    signal popupClosed()
    readonly property alias view: fakeView

    Item {
        id: fakeView
        anchors.fill: parent
        readonly property real zoom: 1
        readonly property point pan: Qt.point(0, 0)
        function pageToScene(p) { return mapToItem(null, p.x, p.y) }
        function sceneToPage(p) { return mapFromItem(null, p.x, p.y) }
        function addInputCover(item) {}
        function removeInputCover(item) {}
    }
    WebFrameLayer {
        objectName: "webFrameLayer"
        commands: host.commands
    }
}
