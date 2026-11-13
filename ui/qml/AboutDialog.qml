import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import ActCraft.Ui

// « À propos » (F-760, décision 34) : version, licence, page du projet, notices des composants tiers.
Dialog {
    id: dialog
    property bool showsLicense: false
    objectName: "aboutDialog"
    title: qsTr("À propos d'ActCraft")
    modal: true
    anchors.centerIn: Overlay.overlay
    width: Math.min(640, Overlay.overlay ? Overlay.overlay.width - 2 * Theme.espace4 : 640)
    height: Math.min(520, Overlay.overlay ? Overlay.overlay.height - 2 * Theme.espace4 : 520)

    ColumnLayout {
        anchors.fill: parent
        spacing: Theme.espace2

        Label {
            objectName: "aboutVersion"
            text: qsTr("ActCraft %1").arg(AppInfo.version)
            font.pixelSize: Theme.taillePoliceTitre
            font.bold: true
        }
        Label {
            Layout.fillWidth: true
            wrapMode: Text.WordWrap
            text: qsTr("Logiciel libre de tableau blanc interactif, sous licence %1.").arg(AppInfo.licenseName)
        }
        // Mentions légales appropriées de la GPLv3 (§ 0, § 5 d) : copyright, absence de garantie, droit de
        // redistribuer ; le bouton « Licence » montre le texte de la licence à la place des notices.
        Label {
            objectName: "aboutLegal"
            Layout.fillWidth: true
            wrapMode: Text.WordWrap
            text: qsTr("%1. Ce programme est fourni sans aucune garantie, dans la mesure permise par la loi. C'est un "
                       + "logiciel libre : vous pouvez le redistribuer et le modifier selon les termes de la GNU General "
                       + "Public License version 3, que le bouton « Licence » affiche.").arg(AppInfo.copyright)
        }
        // Texte stylé, pas riche : seul le premier suit `linkColor` (la palette de la fenêtre, Theme.surfaceActive) ;
        // en texte riche le lien prenait le bleu par défaut de Qt, illisible sur le dialogue sombre (L1-C5).
        Label {
            objectName: "projectLink"
            textFormat: Text.StyledText
            text: "<a href=\"%1\">%1</a>".arg(AppInfo.projectUrl)
            onLinkActivated: link => Qt.openUrlExternally(link)
        }
        // Les notices sont un document à lire : posées sur un puits couleur de page, en texte sombre. Le Markdown
        // colore ses liens d'après la palette de l'application (bleu de Qt), lisible sur ce fond clair.
        ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            // Anneau de focus du champ (L11-T24) : le style n'en dessine pas pour un TextArea. Accent posé sur le
            // dialogue sombre, autour du champ : sur le blanc du puits, il tomberait sous 3:1.
            background: Rectangle {
                anchors.fill: parent
                anchors.margins: -3
                visible: notices.activeFocus
                color: "transparent"
                radius: Theme.rayon + 3
                border.width: 2
                border.color: Theme.surfaceActive
            }
            // Pouce lisible sur le puits blanc (le gris du style Basic y tombait sous 3:1 une fois appuyé).
            ScrollBar.vertical: ScrollBar {
                objectName: "aboutScrollBar"
                contentItem: ScrollThumb {}
            }
            TextArea {
                id: notices
                objectName: "aboutNotices"
                readOnly: true
                wrapMode: TextEdit.Wrap
                textFormat: dialog.showsLicense ? TextEdit.PlainText : TextEdit.MarkdownText
                text: dialog.showsLicense ? AppInfo.licenseText : AppInfo.notices
                color: Theme.barre
                background: Rectangle {
                    color: Theme.page
                    radius: Theme.rayon
                }
            }
        }
    }

    onAboutToShow: showsLicense = false

    footer: DialogButtonBox {
        Button {
            objectName: "aboutLicenseButton"
            text: dialog.showsLicense ? qsTr("Composants tiers") : qsTr("Licence")
            DialogButtonBox.buttonRole: DialogButtonBox.ActionRole
            onClicked: dialog.showsLicense = !dialog.showsLicense
        }
        Button {
            text: qsTr("Fermer")
            DialogButtonBox.buttonRole: DialogButtonBox.RejectRole
        }
    }
}
