import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Dialogs
import QtQuick.Layouts
import ActCraft.Ui

// Profils et réglages (F-882) : liste, importer, enregistrer, enregistrer sous, supprimer, revenir à
// l'origine, puis quatre onglets. Aucun accès disque : tout passe par les commandes profile.*.
Item {
    id: root
    objectName: "profileEditor"
    property Commands commands: null
    property DocumentController documentController: null
    property Tools tools: null
    property PageView view: null
    property var profile: null
    property var preferences: null
    readonly property bool opened: popup.opened
    visible: false

    readonly property var currentEntry: {
        const list = root.profile ? root.profile.profiles : []
        for (let i = 0; i < list.length; ++i)
            if (list[i].id === root.profile.currentId)
                return list[i]
        return ({ id: "", label: "", builtin: false, modified: false })
    }

    Popup {
        id: popup
        objectName: "profileEditorPopup"
        parent: Overlay.overlay
        anchors.centerIn: parent
        modal: true
        // La boîte prend le focus à l'ouverture : Tab parcourt ses contrôles, pas ceux de la fenêtre (L11-T24).
        focus: true
        padding: 12
        closePolicy: Popup.CloseOnEscape
        contentItem: ColumnLayout {
            spacing: 8
            RowLayout {
                spacing: 6
                ComboBox {
                    id: profileCombo
                    objectName: "profileCombo"
                    Layout.preferredWidth: 200
                    model: root.profile ? root.profile.profiles : []
                    textRole: "label"
                    valueRole: "id"
                    currentIndex: indexOfValue(root.profile ? root.profile.currentId : "")
                    onActivated: root.commands.trigger("profile.activate", currentValue)
                }
                Button { text: qsTr("Importer…"); onClicked: root.commands.trigger("profile.import") }
                Button { text: qsTr("Exporter…"); onClicked: root.commands.trigger("profile.export") }
                Button { objectName: "saveButton"; text: qsTr("Enregistrer"); onClicked: root.profile.saveNow() }
                Button { objectName: "saveAsButton"; text: qsTr("Enregistrer sous…"); onClicked: root.commands.trigger("profile.save-as") }
                Button {
                    objectName: "deleteButton"
                    text: qsTr("Supprimer")
                    enabled: !root.currentEntry.builtin
                    onClicked: root.commands.trigger("profile.delete")
                }
                Button {
                    objectName: "resetButton"
                    text: qsTr("Revenir à l'origine")
                    enabled: root.currentEntry.builtin && root.currentEntry.modified
                    onClicked: root.commands.trigger("profile.reset")
                }
            }
            TabBar {
                id: tabs
                objectName: "editorTabs"
                // Le style Basic peint les onglets non choisis en palette.dark (l'accent, ici) : onglets locaux,
                // le choisi garde le fond de la fenêtre, les autres la surface ; l'anneau de focus reste l'accent.
                palette.dark: Theme.surface
                palette.brightText: Theme.texte
                Layout.fillWidth: true
                TabButton {
                    text: qsTr("Format")
                    // Le style n'a pas d'anneau de focus pour un onglet (L11-T24).
                    Rectangle {
                        anchors.fill: parent
                        visible: parent.visualFocus
                        color: "transparent"
                        border.width: 2
                        border.color: Theme.surfaceActive
                    }
                }
                TabButton {
                    text: qsTr("Commandes")
                    // Le style n'a pas d'anneau de focus pour un onglet (L11-T24).
                    Rectangle {
                        anchors.fill: parent
                        visible: parent.visualFocus
                        color: "transparent"
                        border.width: 2
                        border.color: Theme.surfaceActive
                    }
                }
                TabButton {
                    text: qsTr("Raccourcis vers des programmes")
                    // Le style n'a pas d'anneau de focus pour un onglet (L11-T24).
                    Rectangle {
                        anchors.fill: parent
                        visible: parent.visualFocus
                        color: "transparent"
                        border.width: 2
                        border.color: Theme.surfaceActive
                    }
                }
                TabButton {
                    text: qsTr("Paramètres")
                    // Le style n'a pas d'anneau de focus pour un onglet (L11-T24).
                    Rectangle {
                        anchors.fill: parent
                        visible: parent.visualFocus
                        color: "transparent"
                        border.width: 2
                        border.color: Theme.surfaceActive
                    }
                }
            }
            StackLayout {
                currentIndex: tabs.currentIndex
                Layout.preferredWidth: 520
                Layout.preferredHeight: 460
                FormatPage {
                    commands: root.commands; documentController: root.documentController
                    profile: root.profile; preferences: root.preferences
                }
                CommandsPage {
                    commands: root.commands; documentController: root.documentController
                    profile: root.profile; preferences: root.preferences
                }
                LaunchersPage {
                    commands: root.commands; documentController: root.documentController
                    profile: root.profile; preferences: root.preferences
                }
                SettingsPage {
                    commands: root.commands; documentController: root.documentController
                    profile: root.profile; preferences: root.preferences
                }
            }
            Button {
                objectName: "doneButton"
                text: qsTr("Terminé")
                Layout.alignment: Qt.AlignRight
                onClicked: { root.profile.saveNow(); popup.close() }
            }
        }
    }

    Popup {
        id: namePopup
        objectName: "saveAsPopup"
        parent: Overlay.overlay
        anchors.centerIn: parent
        modal: true
        padding: 12
        closePolicy: Popup.CloseOnEscape
        onOpened: { nameField.text = ""; nameField.forceActiveFocus() }
        contentItem: RowLayout {
            spacing: 6
            Label { text: qsTr("Nom du profil") }
            TextField { id: nameField; objectName: "saveAsName"; Layout.preferredWidth: 220; onAccepted: okButton.clicked() }
            Button {
                id: okButton
                text: qsTr("OK")
                enabled: nameField.text.trim().length > 0
                onClicked: {
                    namePopup.close()
                    root.commands.trigger("profile.save-as", nameField.text.trim())
                }
            }
        }
    }

    FileDialog {
        id: importDialog
        objectName: "profileImportDialog"
        title: qsTr("Importer un profil")
        fileMode: FileDialog.OpenFile
        nameFilters: [qsTr("Profils ActCraft (*.json)"), qsTr("Tous les fichiers (*)")]
        onAccepted: root.commands.trigger("profile.import", selectedFile)
    }
    FileDialog {
        id: exportDialog
        objectName: "profileExportDialog"
        title: qsTr("Exporter le profil")
        fileMode: FileDialog.SaveFile
        defaultSuffix: "json"
        nameFilters: [qsTr("Profils ActCraft (*.json)")]
        onAccepted: root.commands.trigger("profile.export", selectedFile)
    }

    // Les gestionnaires natifs ne font rien sans argument : l'interface demande alors le nom ou le fichier.
    Connections {
        target: root.commands
        function onTriggered(id, argument) {
            const empty = argument === undefined || argument === null || String(argument) === ""
            if (!empty)
                return
            if (id === "profile.save-as") namePopup.open()
            else if (id === "profile.import") importDialog.open()
            else if (id === "profile.export") exportDialog.open()
        }
    }

    Component.onCompleted: {
        root.commands.setHandler("profile.edit", function () { popup.open() })
    }
}
