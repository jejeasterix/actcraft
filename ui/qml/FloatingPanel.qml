import QtQuick

// Talon L7-S8 : rempli par L7-T2.
Item {
    id: panel
    property string title
    property Item pageArea: null
    default property alias content: body.data
    signal closed()
    Item {
        id: body
        anchors.fill: parent
    }
}
