import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import ActCraft.Ui

// Confirmation avant d'ouvrir un lien ou un fichier d'un paperboard (D8-4) : la cible complète, en texte brut (elle
// vient du document), puis « Ouvrir » ou « Annuler ». Seul « Ouvrir » répond oui ; Annuler, Échap, la fermeture de la
// boîte ou une nouvelle demande répondent non. Un refus du lanceur montre son motif.
Dialog {
    id: dialog
    objectName: "linkConfirmDialog"
    property LinkLauncher launcher: null
    property string shownTarget: ""
    property string reason: ""
    property int token: -1
    readonly property bool asking: token >= 0
    title: asking ? qsTr("Ouvrir ce lien ?") : qsTr("Ouverture refusée")
    modal: true
    width: 520
    anchors.centerIn: Overlay.overlay
    closePolicy: Popup.CloseOnEscape

    function answer(accepted) {
        const pending = dialog.token
        dialog.token = -1
        if (pending >= 0 && dialog.launcher)
            dialog.launcher.answer(pending, accepted)
    }
    function ask(target, newToken) {
        dialog.answer(false) // une demande restée sans réponse est refusée, jamais ouverte
        dialog.reason = ""
        dialog.shownTarget = target
        dialog.token = newToken
        dialog.open()
    }
    function showRefusal(text) {
        dialog.answer(false)
        dialog.shownTarget = ""
        dialog.reason = text
        dialog.open()
    }
    // Jeton mis de côté, boîte fermée, puis « oui » : un refus tardif (fichier changé depuis la demande)
    // rouvre la boîte avec son motif, que la fermeture n'efface plus.
    function openTarget() {
        const pending = dialog.token
        dialog.token = -1
        dialog.close()
        if (pending >= 0 && dialog.launcher)
            dialog.launcher.answer(pending, true)
    }

    onOpened: cancelButton.forceActiveFocus() // Entrée ou Espace n'ouvrent rien par défaut
    onClosed: dialog.answer(false)

    Connections {
        target: dialog.launcher
        function onConfirmationNeeded(shownTarget, token) { dialog.ask(shownTarget, token) }
        function onRefused(reason) { dialog.showRefusal(reason) }
    }

    contentItem: ColumnLayout {
        spacing: 8
        Label {
            objectName: "linkConfirmText"
            Layout.fillWidth: true
            text: dialog.asking ? qsTr("Le paperboard demande d'ouvrir :") : dialog.reason
            textFormat: Text.PlainText
            wrapMode: Text.Wrap
        }
        Label {
            objectName: "linkConfirmTarget"
            Layout.fillWidth: true
            visible: dialog.asking
            text: dialog.shownTarget
            textFormat: Text.PlainText
            wrapMode: Text.WrapAnywhere
        }
    }
    footer: DialogButtonBox {
        Button {
            objectName: "linkConfirmOpen"
            enabled: dialog.asking
            text: qsTr("Ouvrir")
            onClicked: dialog.openTarget()
        }
        Button {
            id: cancelButton
            objectName: "linkConfirmCancel"
            text: dialog.asking ? qsTr("Annuler") : qsTr("Fermer")
            onClicked: dialog.close()
        }
    }
}
