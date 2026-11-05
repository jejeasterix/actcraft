import QtQuick
import QtQuick.Controls.Basic
import ActCraft.Ui

// Navigateur de propriétés (F-570 à F-576, F-583, S9.4) : titre (Page, Trait, 3 objets…) et liste en sections
// repliables, un éditeur par ligne (PropertyEditor). Le modèle vit ici : il suit la page affichée et la
// sélection ; la liste garde ses délégués tant que les lignes sont les mêmes (le modèle ne fait alors que
// signaler la valeur changée), ce qui laisse un glissé de réglette se poursuivre.
Item {
    id: root
    objectName: "propertyBrowser"
    property Commands commands: null
    property DocumentController documentController: null
    property Tools tools: null
    property PageView view: null
    property var profile: null
    property var preferences: null

    // Sections repliées, par libellé : { "Contour": true }.
    property var folded: ({})
    function toggle(section) {
        const next = Object.assign({}, root.folded)
        next[section] = !next[section]
        root.folded = next
    }

    PropertyModel {
        id: appProperties
        objectName: "appProperties"
        document: root.documentController
        tools: root.tools
        commands: root.commands
        contactActive: root.view ? root.view.contactActive : false
    }

    Rectangle {
        anchors.fill: parent
        color: Theme.surface
    }

    Text {
        id: heading
        objectName: "propertyTitle"
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.margins: Theme.espace2
        text: appProperties.title
        textFormat: Text.PlainText // titre tiré du fichier ouvert
        color: Theme.texte
        font.pixelSize: Theme.taillePoliceTitre
        font.bold: true
        elide: Text.ElideRight
    }

    ListView {
        id: list
        objectName: "propertyList"
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: heading.bottom
        anchors.bottom: parent.bottom
        anchors.topMargin: Theme.espace2
        clip: true
        model: appProperties
        boundsBehavior: Flickable.StopAtBounds
        ScrollBar.vertical: ScrollBar {}

        section.property: "sectionLabel"
        section.criteria: ViewSection.FullString
        section.delegate: Rectangle {
            id: header
            required property string section
            width: list.width
            height: Theme.tailleBouton - 12
            color: Theme.surfaceSurvol
            Text {
                anchors.left: parent.left
                anchors.leftMargin: Theme.espace2
                anchors.verticalCenter: parent.verticalCenter
                text: (root.folded[header.section] ? "▸ " : "▾ ") + header.section
                color: Theme.texte
                font.pixelSize: Theme.taillePolice
                font.bold: true
            }
            MouseArea {
                anchors.fill: parent
                onClicked: root.toggle(header.section)
            }
        }

        delegate: PropertyEditor {
            id: row
            required property string sectionLabel
            width: list.width
            properties: appProperties
            folded: root.folded[row.sectionLabel] === true
            commands: root.commands
            documentController: root.documentController
            tools: root.tools
            view: root.view
            profile: root.profile
            preferences: root.preferences
        }
    }
}
