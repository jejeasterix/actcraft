import QtQuick

// Zone de saisie d'un outil de maths (règle, équerre, rapporteur, compas) : curseur et geste propres. Le geste
// passe par l'outil `owner` : begin(kind, scenePress), drag(scenePoint), end(). `shaped` : la zone ne reçoit que
// les points de sa région, owner.zoneAt(x, y) === kind (équerre, rapporteur, compas) ; sinon tout son rectangle.
// Position et taille sont posées par l'outil.
Item {
    id: zone
    property var owner: null
    property string kind: "move"
    property int cursorShape: Qt.SizeAllCursor
    property bool shaped: false

    containmentMask: shaped ? regionMask : null
    QtObject {
        id: regionMask
        function contains(point: point): bool {
            return zone.owner !== null && zone.owner.zoneAt(point.x, point.y) === zone.kind
        }
    }
    HoverHandler { cursorShape: zone.cursorShape }
    DragHandler {
        target: null
        acceptedDevices: PointerDevice.AllDevices
        onActiveChanged: {
            if (!zone.owner)
                return
            if (active)
                zone.owner.begin(zone.kind, centroid.scenePressPosition)
            else
                zone.owner.end()
        }
        onCentroidChanged: if (active && zone.owner) zone.owner.drag(centroid.scenePosition)
    }
}
