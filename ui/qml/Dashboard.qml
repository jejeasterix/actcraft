import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import ActCraft.Ui

// Accueil (F-090) : fenêtre devant la fenêtre principale. Colonne de gauche : Paperboards, Réglages, Aide ;
// panneau Paperboards : créer, ouvrir, importer un PDF, puis les documents récents. « Annoter le bureau » est une
// commande du lot 7 (sans gestionnaire à ce jour) : son entrée viendra avec elle. S'ouvre au démarrage de
// l'application seulement (`showDashboard`, posée par app/main.cpp) et si la case le demande ; rouvrable par
// `view.dashboard` (F11).
Item {
    id: root
    objectName: "dashboard"
    property Commands commands: null
    property DocumentController documentController: null
    property Tools tools: null
    property PageView view: null
    property var profile: null
    property var preferences: null
    // Documents récents (fileActions.recents, L6-T24).
    property var recents: null
    // Ouverture au démarrage : vrai dans l'application seulement (StartupOptions::showDashboard).
    property bool showDashboard: false
    readonly property int recentCount: root.recents ? root.recents.count : 0

    anchors.fill: parent
    z: 40
    visible: false
    focus: visible

    function open() { root.visible = true }
    function close() { root.visible = false }
    // Ferme l'accueil puis joue la commande (un dialogue ne s'ouvre pas sous lui).
    function run(id, argument) {
        root.close()
        root.commands.trigger(id, argument)
    }

    Keys.onEscapePressed: root.close()

    // Aucune lecture de vignette pendant un contact sur la page.
    Binding {
        target: root.recents
        property: "contactActive"
        value: root.view ? root.view.contactActive : false
        when: root.recents !== null
    }

    MouseArea {
        anchors.fill: parent
        onClicked: root.close()
    }
    Rectangle {
        anchors.fill: parent
        color: "#80000000"
        z: -1
    }

    Rectangle {
        id: card
        objectName: "dashboardCard"
        width: 880
        height: 540
        anchors.centerIn: parent
        radius: Theme.rayon
        color: Theme.palette
        border.color: Theme.contour

        // Absorbe les clics : seul le fond ferme l'accueil.
        MouseArea { anchors.fill: parent }

        Column {
            id: side
            x: Theme.espace4
            y: Theme.espace4
            width: 200
            spacing: Theme.espace2
            Repeater {
                model: [
                    { name: "dashboardPaperboards", text: qsTr("Paperboards"), command: "" },
                    { name: "dashboardSettings", text: qsTr("Réglages"), command: "profile.edit" },
                    { name: "dashboardHelp", text: qsTr("Aide"), command: "help.contents" }
                ]
                delegate: Button {
                    required property var modelData
                    objectName: modelData.name
                    width: side.width
                    height: Theme.tailleBouton
                    text: modelData.text
                    enabled: modelData.command === "" || root.commands.isAvailable(modelData.command)
                    onClicked: if (modelData.command !== "") root.run(modelData.command)
                }
            }
        }

        Column {
            x: 232
            y: Theme.espace4
            width: card.width - 232 - Theme.espace4
            spacing: Theme.espace3

            Label {
                text: qsTr("Paperboards")
                color: Theme.texte
                font.pixelSize: Theme.taillePoliceTitre
                font.bold: true
                height: 28
            }
            Row {
                spacing: Theme.espace2
                Repeater {
                    model: [
                        { name: "dashboardNew", text: qsTr("Créer"), command: "file.new" },
                        { name: "dashboardOpen", text: qsTr("Ouvrir…"), command: "file.open" },
                        { name: "dashboardImportPdf", text: qsTr("Importer un PDF…"), command: "file.import-pdf" }
                    ]
                    delegate: Button {
                        required property var modelData
                        objectName: modelData.name
                        height: Theme.tailleBouton
                        text: modelData.text
                        enabled: root.commands.isAvailable(modelData.command)
                        onClicked: root.run(modelData.command)
                    }
                }
            }
            Label {
                text: qsTr("Documents récents")
                color: Theme.texteDiscret
                height: 24
                verticalAlignment: Text.AlignVCenter
            }
            Label {
                visible: root.recentCount === 0
                text: qsTr("Aucun document récent")
                color: Theme.texteDesactive
            }
            Flow {
                id: grid
                width: parent.width
                spacing: Theme.espace3
                Repeater {
                    model: root.recents ? root.recents.entries : []
                    delegate: Rectangle {
                        id: cell
                        required property int index
                        required property var modelData
                        objectName: "recent" + index
                        width: 190
                        height: 130
                        radius: Theme.rayon
                        color: area.containsMouse ? Theme.surfaceSurvol : Theme.surface
                        border.color: Theme.contour
                        Image {
                            objectName: "recentThumbnail" + cell.index
                            visible: status === Image.Ready
                            anchors.top: parent.top
                            anchors.horizontalCenter: parent.horizontalCenter
                            anchors.topMargin: Theme.espace2
                            width: parent.width - 2 * Theme.espace2
                            height: 70
                            fillMode: Image.PreserveAspectFit
                            source: cell.modelData.thumbnail
                        }
                        Column {
                            anchors.bottom: parent.bottom
                            anchors.bottomMargin: Theme.espace2
                            width: parent.width - 2 * Theme.espace3
                            anchors.horizontalCenter: parent.horizontalCenter
                            spacing: Theme.espace2
                            Label {
                                width: parent.width
                                text: cell.modelData.name
                                color: Theme.texte
                                font.bold: true
                                elide: Text.ElideMiddle
                                horizontalAlignment: Text.AlignHCenter
                            }
                            Label {
                                width: parent.width
                                text: Qt.formatDateTime(cell.modelData.when, "dd/MM/yyyy HH:mm")
                                color: Theme.texteDiscret
                                horizontalAlignment: Text.AlignHCenter
                            }
                        }
                        MouseArea {
                            id: area
                            anchors.fill: parent
                            hoverEnabled: true
                            onClicked: root.run("file.open", cell.modelData.url)
                        }
                    }
                }
            }
        }

        CheckBox {
            objectName: "dashboardAtStartBox"
            x: 232
            y: card.height - height - Theme.espace4
            text: qsTr("Afficher l'accueil au démarrage")
            checked: root.preferences ? root.preferences.dashboardAtStart : true
            onToggled: if (root.preferences) root.preferences.dashboardAtStart = checked
        }
    }

    Component.onCompleted: {
        root.commands.setHandler("view.dashboard", function () { root.open() })
        if (root.showDashboard && (!root.preferences || root.preferences.dashboardAtStart))
            root.open()
    }
}
