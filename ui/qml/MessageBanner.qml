import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import ActCraft.Ui

// Avertissements de lecture et erreurs : affichés sans bloquer, refermables. Le message peut contenir un nom de
// fichier ou un avertissement venu d'un fichier ouvert : texte brut, jamais interprété.
Rectangle {
    id: banner
    objectName: "messageBanner"
    property string message: ""
    visible: message.length > 0
    color: Theme.barre
    border.color: Theme.contour
    radius: Theme.rayon
    implicitHeight: row.implicitHeight + 2 * Theme.espace2

    function show(text) { banner.message = text }
    function dismiss() { banner.message = "" }

    RowLayout {
        id: row
        anchors.fill: parent
        anchors.margins: Theme.espace2
        Label {
            objectName: "messageText"
            Layout.fillWidth: true
            text: banner.message
            textFormat: Text.PlainText
            color: Theme.texte
            wrapMode: Text.Wrap
            font.pixelSize: Theme.taillePolice
        }
        Button {
            objectName: "messageClose"
            text: qsTr("Fermer")
            onClicked: banner.dismiss()
        }
    }
}
