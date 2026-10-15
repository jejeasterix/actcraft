import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import ActCraft.Ui

// « À propos » (F-760, décision 34) : version, licence, page du projet, notices des composants tiers.
Dialog {
    id: dialog
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
        // Texte stylé, pas riche : seul le premier suit `linkColor` (la palette de la fenêtre, Theme.surfaceActive) ;
        // en texte riche le lien prenait le bleu par défaut de Qt, illisible sur le dialogue sombre (L1-C5).
        Label {
            textFormat: Text.StyledText
            text: "<a href=\"%1\">%1</a>".arg(AppInfo.projectUrl)
            onLinkActivated: link => Qt.openUrlExternally(link)
        }
        // Les notices sont un document à lire : posées sur un puits couleur de page, en texte sombre. Le Markdown
        // colore ses liens d'après la palette de l'application (bleu de Qt), lisible sur ce fond clair.
        ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            // Pouce lisible sur le puits blanc (le gris du style Basic y tombait sous 3:1 une fois appuyé).
            ScrollBar.vertical: ScrollBar {
                objectName: "aboutScrollBar"
                contentItem: ScrollThumb {}
            }
            TextArea {
                objectName: "aboutNotices"
                readOnly: true
                wrapMode: TextEdit.Wrap
                textFormat: TextEdit.MarkdownText
                text: AppInfo.notices
                color: Theme.barre
                background: Rectangle {
                    color: Theme.page
                    radius: Theme.rayon
                }
            }
        }
    }

    footer: DialogButtonBox {
        Button {
            text: qsTr("Fermer")
            DialogButtonBox.buttonRole: DialogButtonBox.RejectRole
        }
    }
}
