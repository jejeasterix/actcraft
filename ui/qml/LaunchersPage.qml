import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Dialogs
import QtQuick.Layouts
import ActCraft.Ui

// Raccourcis vers des programmes (F-920, F-142). Deux instances existent : celle de Main (invisible), qui pose
// les gestionnaires launcher.* et porte l'unique boîte de confirmation, et celle de l'onglet de l'éditeur de
// profils (`embedded`), qui n'est que la liste et le formulaire d'ajout.
// launcher.run : argument = identifiant du raccourci, ou son rang (« 0 » : launcherButton0).
// launcher.edit : sans argument, ouvre l'éditeur sur cet onglet ; {path, arguments, toolTip} ajoute ;
// {remove: rang ou identifiant} retire.
Item {
    id: root
    objectName: "launchersPage"
    property Commands commands: null
    property DocumentController documentController: null
    property Tools tools: null
    property PageView view: null
    property var profile: null
    property var preferences: null
    readonly property var host: root.parent
    readonly property bool embedded: root.host !== null && root.host !== undefined && root.host.currentIndex !== undefined
    readonly property alias launchers: launchers
    readonly property bool acceptsArguments: root.takesArguments(pathField.text)
    implicitWidth: 480
    implicitHeight: 320
    visible: false

    Launchers {
        id: launchers
        objectName: "launchers"
        profile: root.profile
    }

    // Un programme prend des paramètres (.exe ; sans suffixe hors Windows), un document jamais.
    function takesArguments(path) {
        const name = path.split("/").pop().split("\\").pop()
        const dot = name.lastIndexOf(".")
        if (dot < 0)
            return Qt.platform.os !== "windows"
        return name.slice(dot + 1).toLowerCase() === "exe"
    }

    // Découpe en paramètres, les guillemets groupent.
    function splitArguments(text) {
        const out = []
        const re = /"([^"]*)"|(\S+)/g
        let m
        while ((m = re.exec(text)) !== null)
            out.push(m[1] !== undefined ? m[1] : m[2])
        return out
    }

    function idOf(ref) {
        const text = String(ref)
        if (/^\d+$/.test(text)) {
            const entry = launchers.entries[parseInt(text)]
            return entry ? entry.id : ""
        }
        return text
    }

    function addFromFields() {
        const args = root.acceptsArguments ? root.splitArguments(argsField.text) : []
        if (launchers.add(pathField.text, args, tipField.text) !== "") {
            pathField.text = ""
            argsField.text = ""
            tipField.text = ""
        }
    }

    function selectMyTab() {
        const stack = root.host
        const bars = stack && stack.parent ? stack.parent.children : []
        for (let i = 0; i < bars.length; ++i) {
            if (bars[i].objectName === "editorTabs") {
                for (let j = 0; j < stack.children.length; ++j)
                    if (stack.children[j] === root)
                        bars[i].currentIndex = j
                return
            }
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 6
        ListView {
            id: list
            objectName: "launcherList"
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            model: launchers.entries
            delegate: RowLayout {
                id: row
                required property int index
                required property var modelData
                width: ListView.view.width
                Label {
                    Layout.fillWidth: true
                    elide: Text.ElideMiddle
                    text: (row.modelData.toolTip !== "" ? row.modelData.toolTip + " — " : "") + row.modelData.path
                }
                Button {
                    objectName: "launcherRemove" + row.index
                    text: qsTr("Retirer")
                    onClicked: launchers.remove(row.modelData.id)
                }
            }
        }
        RowLayout {
            TextField {
                id: pathField
                objectName: "launcherPathField"
                Layout.fillWidth: true
                placeholderText: qsTr("Programme ou document")
            }
            Button { text: qsTr("Parcourir…"); onClicked: fileDialog.open() }
        }
        TextField {
            id: argsField
            objectName: "launcherArgsField"
            Layout.fillWidth: true
            enabled: root.acceptsArguments
            placeholderText: qsTr("Paramètres")
        }
        Label {
            objectName: "launcherArgsNote"
            visible: pathField.text !== "" && !root.acceptsArguments
            text: qsTr("Un document ne prend aucun paramètre.")
        }
        TextField {
            id: tipField
            objectName: "launcherTipField"
            Layout.fillWidth: true
            placeholderText: qsTr("Info-bulle")
        }
        Button {
            objectName: "launcherAddButton"
            text: qsTr("Ajouter")
            enabled: pathField.text.trim() !== ""
            onClicked: root.addFromFields()
        }
    }

    FileDialog {
        id: fileDialog
        objectName: "launcherFileDialog"
        title: qsTr("Choisir un programme ou un document")
        onAccepted: pathField.text = launchers.localPath(selectedFile)
    }

    // Boîtes : une seule confirmation, une seule fenêtre de refus, dans la page de Main seulement.
    Loader {
        id: dialogsHost
        active: !root.embedded
        sourceComponent: Item {
            id: dialogs
            objectName: "launcherDialogs"

            function showConfirmation(id, path, args) {
                // Une seule demande à la fois : celle qui est affichée remplace la précédente.
                if (confirmation.shownId !== "" && confirmation.shownId !== id)
                    launchers.confirm(confirmation.shownId, false)
                confirmation.shownId = id
                confirmation.shownPath = path
                confirmation.shownArguments = args
                confirmation.answered = false
                confirmation.open()
            }
            function showFailure(message) {
                failure.message = message
                failure.open()
            }

            Popup {
                id: confirmation
                objectName: "launcherConfirmation"
                property string shownId: ""
                property string shownPath: ""
                property var shownArguments: []
                property bool answered: false
                readonly property bool argumentsVisible: shownArguments.length > 0
                function accept() {
                    answered = true
                    launchers.confirm(shownId, true)
                    shownId = ""
                    close()
                }
                function refuse() {
                    answered = true
                    launchers.confirm(shownId, false)
                    shownId = ""
                    close()
                }
                parent: Overlay.overlay
                anchors.centerIn: parent
                modal: true
                padding: 12
                closePolicy: Popup.CloseOnEscape
                onClosed: if (!answered && shownId !== "") refuse()
                contentItem: ColumnLayout {
                    spacing: 8
                    Label { text: qsTr("Lancer ce programme ? Il ne vient pas de ce poste."); font.bold: true }
                    Label { objectName: "launcherConfirmPath"; text: confirmation.shownPath }
                    Label {
                        objectName: "launcherConfirmArguments"
                        visible: confirmation.argumentsVisible
                        text: qsTr("Paramètres : %1").arg(confirmation.shownArguments.join(" "))
                    }
                    RowLayout {
                        Button { objectName: "launcherAccept"; text: qsTr("Lancer"); onClicked: confirmation.accept() }
                        Button { objectName: "launcherRefuse"; text: qsTr("Refuser"); onClicked: confirmation.refuse() }
                    }
                }
            }
            Popup {
                id: failure
                objectName: "launcherFailure"
                property string message: ""
                parent: Overlay.overlay
                anchors.centerIn: parent
                modal: true
                padding: 12
                contentItem: ColumnLayout {
                    Label { objectName: "launcherFailureText"; text: failure.message }
                    Button { text: qsTr("OK"); onClicked: failure.close() }
                }
            }
        }
    }

    Connections {
        target: launchers
        enabled: !root.embedded
        function onConfirmationNeeded(id, path, args) {
            dialogsLoader().showConfirmation(id, path, args)
        }
        function onFailed(message) {
            dialogsLoader().showFailure(message)
        }
    }

    function dialogsLoader() {
        return dialogsHost.item
    }

    Connections {
        target: root.commands
        enabled: root.embedded
        function onTriggered(id, argument) {
            if (id === "launcher.edit" && (argument === undefined || argument === null || typeof argument !== "object"))
                root.selectMyTab()
        }
    }

    Component.onCompleted: {
        if (root.embedded)
            return
        root.commands.setHandler("launcher.run", function (ref) {
            const id = root.idOf(ref === undefined || ref === null ? "" : ref)
            if (id !== "")
                launchers.run(id)
        })
        root.commands.setHandler("launcher.edit", function (spec) {
            if (spec !== undefined && spec !== null && typeof spec === "object") {
                if (spec.remove !== undefined)
                    launchers.remove(root.idOf(spec.remove))
                else if (spec.path)
                    launchers.add(String(spec.path), (spec.arguments || []).map(String), String(spec.toolTip || ""))
            } else {
                root.commands.trigger("profile.edit")
            }
        })
    }
}
