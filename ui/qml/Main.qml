import QtQuick

Window {
    id: root
    width: 1280
    height: 800
    visible: true
    title: qsTr("ActCraft")
    color: "#3c3f45"

    // Page de démonstration au format d'un paperboard (1024 × 768).
    Rectangle {
        id: page
        objectName: "page"
        width: 1024
        height: 768
        anchors.centerIn: parent
        color: "white"

        Text {
            anchors.centerIn: parent
            text: qsTr("ActCraft — rendu : %1").arg(root.apiName(GraphicsInfo.api))
            font.pixelSize: 24
            color: "#3c3f45"
        }
    }

    function apiName(api) {
        switch (api) {
        case GraphicsInfo.Software: return "logiciel"
        case GraphicsInfo.Direct3D11: return "Direct3D 11"
        case GraphicsInfo.Direct3D12: return "Direct3D 12"
        case GraphicsInfo.Metal: return "Metal"
        case GraphicsInfo.Vulkan: return "Vulkan"
        case GraphicsInfo.OpenGL: return "OpenGL"
        default: return "inconnu"
        }
    }
}
