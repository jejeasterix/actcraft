import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import ActCraft.Ui

// Aperçu d'une feuille imprimée (F-670) : l'image de PrintController.previewSheet, c'est-à-dire la feuille de
// print::renderSheet, la même que l'impression. Paresseux : tant que l'élément n'est pas visible, aucune feuille
// n'est composée ; visible, seule la feuille montrée l'est. Invisible par défaut (Main.qml en pose un sans le
// montrer) ; la boîte d'impression met `visible: true`.
Item {
    id: root
    visible: false

    required property PrintController printing
    property int index: 0
    readonly property int count: printing ? printing.sheetCount : 0
    readonly property int previewDpi: 72

    onCountChanged: if (index >= count) index = Math.max(0, count - 1)

    ColumnLayout {
        anchors.fill: parent
        spacing: 6

        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            color: "#d9d9d9"
            Image {
                id: previewImage
                objectName: "previewImage"
                anchors.fill: parent
                anchors.margins: 6
                fillMode: Image.PreserveAspectFit
                asynchronous: false
                cache: false
                // La lecture de sheetCount et de gray relance la liaison à chaque changement de réglage ou
                // de document (signal settingsChanged) : l'adresse porte la révision.
                source: root.visible && root.printing && root.printing.sheetCount > 0 && root.printing.gray !== undefined
                        ? root.printing.previewSource(root.index, root.previewDpi) : ""
            }
        }
        RowLayout {
            Layout.alignment: Qt.AlignHCenter
            Button {
                objectName: "previousSheetButton"
                text: qsTr("◀ Précédente")
                enabled: root.index > 0
                onClicked: root.index = Math.max(0, root.index - 1)
            }
            Label {
                objectName: "sheetLabel"
                text: qsTr("Feuille %1 sur %2").arg(root.count > 0 ? root.index + 1 : 0).arg(root.count)
            }
            Button {
                objectName: "nextSheetButton"
                text: qsTr("Suivante ▶")
                enabled: root.index + 1 < root.count
                onClicked: root.index = Math.min(root.count - 1, root.index + 1)
            }
        }
    }
}
