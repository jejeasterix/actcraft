import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import ActCraft.Ui

// Recherche de mises à jour (F-760, F-930 ; D11-13) : `help.check-updates` ouvre la boîte et lance la vérification
// (une requête, sur demande). Le dépôt peut être privé ou injoignable : « impossible de vérifier » est un état
// ordinaire, dit calmement. « Voir la version » ouvre la page de la version dans le navigateur ; rien n'est téléchargé.
Dialog {
    id: dialog
    objectName: "updateDialog"
    required property Commands commands
    required property UpdateChecker updates
    title: qsTr("Mises à jour")
    modal: true
    anchors.centerIn: Overlay.overlay
    width: Math.min(460, Overlay.overlay ? Overlay.overlay.width - 2 * Theme.espace4 : 460)

    // Gestionnaire (argument, user) ; la coquille appelle start() une seule fois, ici, au chargement (M15) :
    // sans cet appel la case « au démarrage » des réglages ne ferait rien. start() ne fait rien si le réglage est faux.
    Component.onCompleted: {
        dialog.commands.setHandler("help.check-updates", function (argument, user) {
            dialog.updates.check()
            dialog.open()
        })
        dialog.updates.start()
        // start() lance la recherche seulement si le réglage est vrai : l'état « en cours » le dit aussitôt.
        dialog.startupSearch = dialog.updates.state === UpdateChecker.Checking
    }

    // La recherche du démarrage ouvre la boîte seulement si une nouvelle version existe : ni à jour, ni en erreur
    // (aucune boîte au démarrage sans nouvelle version, L11-T22).
    property bool startupSearch: false
    Connections {
        target: dialog.updates
        function onStateChanged() {
            if (!dialog.startupSearch || dialog.updates.state === UpdateChecker.Checking)
                return
            dialog.startupSearch = false
            if (dialog.updates.state === UpdateChecker.Available)
                dialog.open()
        }
    }

    Label {
        objectName: "updateMessage"
        width: parent ? parent.width : implicitWidth
        wrapMode: Text.WordWrap
        text: {
            switch (dialog.updates.state) {
            case UpdateChecker.Checking:
                return qsTr("Recherche en cours…")
            case UpdateChecker.UpToDate:
                return qsTr("ActCraft est à jour.")
            case UpdateChecker.Available:
                return qsTr("La version %1 est disponible.").arg(dialog.updates.latestVersion)
            case UpdateChecker.Failed:
                return qsTr("Impossible de vérifier pour le moment.")
            default:
                return ""
            }
        }
    }

    footer: DialogButtonBox {
        Button {
            objectName: "updateViewButton"
            text: qsTr("Voir la version")
            enabled: dialog.updates.state === UpdateChecker.Available
            DialogButtonBox.buttonRole: DialogButtonBox.ActionRole
            onClicked: Qt.openUrlExternally(dialog.updates.releasePage)
        }
        Button {
            objectName: "updateCloseButton"
            text: qsTr("Fermer")
            DialogButtonBox.buttonRole: DialogButtonBox.RejectRole
        }
    }
}
