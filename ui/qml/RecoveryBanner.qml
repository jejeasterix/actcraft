import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import ActCraft.Ui

// Bandeau de reprise (D11-8, L11-T13) : un travail interrompu (application mise en arrière-plan puis arrêtée)
// a été rouvert depuis sa copie de reprise. « Supprimer la copie » (recovery.discard) efface la copie ; le
// document affiché reste. « Fermer » masque le bandeau seulement.
Rectangle {
    id: banner
    objectName: "recoveryBanner"
    required property Commands commands
    required property LifecycleKeeper lifecycle
    property bool dismissed: false
    visible: banner.lifecycle.recovered && !banner.dismissed
    color: Theme.barre
    border.color: Theme.contour
    radius: Theme.rayon
    implicitHeight: row.implicitHeight + 2 * Theme.espace2

    Component.onCompleted: banner.commands.setHandler("recovery.discard", function (argument, user) {
        banner.lifecycle.discard()
    })

    RowLayout {
        id: row
        anchors.fill: parent
        anchors.margins: Theme.espace2
        Label {
            objectName: "recoveryText"
            Layout.fillWidth: true
            text: qsTr("Travail repris après une interruption")
            color: Theme.texte
            wrapMode: Text.Wrap
            font.pixelSize: Theme.taillePolice
        }
        Button {
            objectName: "recoveryDiscard"
            text: qsTr("Supprimer la copie")
            onClicked: banner.commands.trigger("recovery.discard")
        }
        Button {
            objectName: "recoveryClose"
            text: qsTr("Fermer")
            onClicked: banner.dismissed = true
        }
    }
}
