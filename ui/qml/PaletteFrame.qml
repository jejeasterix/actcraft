// Talon L6 : rempli par L6-T7.
import QtQuick
import ActCraft.Ui

Item {
    id: root
    objectName: "paletteFrame"
    default property alias content: holder.data
    property Commands commands: null
    property DocumentController documentController: null
    property Tools tools: null
    property PageView view: null
    property var profile: null
    property var preferences: null
    implicitWidth: holder.childrenRect.width
    implicitHeight: holder.childrenRect.height
    Item {
        id: holder
        anchors.fill: parent
    }

    Component.onCompleted: {
        root.commands.setHandler("palette.dock-left", function () { console.info("palette", "palette.dock-left") })
        root.commands.setHandler("palette.dock-right", function () { console.info("palette", "palette.dock-right") })
        root.commands.setHandler("palette.dock-top", function () { console.info("palette", "palette.dock-top") })
        root.commands.setHandler("palette.dock-bottom", function () { console.info("palette", "palette.dock-bottom") })
        root.commands.setHandler("palette.float", function () { console.info("palette", "palette.float") })
        root.commands.setHandler("palette.collapse", function () { console.info("palette", "palette.collapse") })
        root.commands.setHandler("palette.collapse-full", function () { console.info("palette", "palette.collapse-full") })
        root.commands.setHandler("palette.pin", function () { console.info("palette", "palette.pin") })
        root.commands.setHandler("palette.icon-size", function () { console.info("palette", "palette.icon-size") })
    }
}
