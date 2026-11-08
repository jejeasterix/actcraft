import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import ActCraft.Ui

// Indication non modale d'une nouvelle version trouvée par la recherche du démarrage (D11-13, L11-T22 : aucune
// boîte au démarrage). Rien n'apparaît si la recherche ne trouve rien, échoue ou n'a pas lieu. « Voir la version »
// ouvre la page de la version dans le navigateur ; « Fermer » masque le bandeau seulement.
Rectangle {
    id: banner
    objectName: "updateBanner"
    required property UpdateChecker updates
    // Vrai quand la recherche du démarrage a trouvé une version (posé par UpdateDialog).
    required property bool found
    property bool dismissed: false
    // Version et page copiées au moment de la découverte : une recherche manuelle ultérieure vide l'état de
    // `updates` (Checking, échec, « à jour ») sans changer ce que le bandeau annonce.
    property string version: ""
    property url page
    function capture() {
        if (banner.found && banner.version.length === 0) {
            banner.version = banner.updates.latestVersion
            banner.page = banner.updates.releasePage
        }
    }
    onFoundChanged: capture()
    Component.onCompleted: capture()
    visible: banner.found && !banner.dismissed
    color: Theme.barre
    border.color: Theme.contour
    radius: Theme.rayon
    implicitHeight: row.implicitHeight + 2 * Theme.espace2
    Accessible.role: Accessible.AlertMessage
    Accessible.name: updateText.text

    RowLayout {
        id: row
        anchors.fill: parent
        anchors.margins: Theme.espace2
        Label {
            id: updateText
            objectName: "updateBannerText"
            Layout.fillWidth: true
            text: qsTr("Version %1 disponible").arg(banner.version)
            color: Theme.texte
            wrapMode: Text.Wrap
            font.pixelSize: Theme.taillePolice
        }
        Button {
            objectName: "updateBannerView"
            text: qsTr("Voir la version")
            onClicked: Qt.openUrlExternally(banner.page)
        }
        Button {
            objectName: "updateBannerClose"
            text: qsTr("Fermer")
            onClicked: banner.dismissed = true
        }
    }
}
