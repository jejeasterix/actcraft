import QtQuick
import QtQuick.Controls.Basic
import ActCraft.Ui

// Navigateur de notes (F-830) : texte riche propre à la page affichée, écrit par SetPageNotesCommand (la frappe
// continue se fusionne en une étape). Gras, italique et souligné par les raccourcis du système ; la barre de
// mise en forme du texte (TextFormatBar) agit sur un objet texte de la page, pas sur les notes.
Item {
    id: root
    objectName: "notesBrowser"
    property Commands commands: null
    property DocumentController documentController: null
    property Tools tools: null
    property PageView view: null
    property var profile: null
    property var preferences: null
    property bool hasNotes: appNotes.hasNotes

    NotesController {
        id: appNotes
        objectName: "appNotes"
        document: root.documentController
        commands: root.commands
        contactActive: root.view ? root.view.contactActive : false
        // Le texte d'une page se recharge : l'éditeur se remplit sans écrire (garde « chargement »).
        onHtmlChanged: editor.load()
    }

    onVisibleChanged: if (visible) Qt.callLater(() => editor.forceActiveFocus())

    Rectangle {
        anchors.fill: parent
        color: Theme.surface
    }

    ScrollView {
        id: scroll
        anchors.fill: parent
        anchors.margins: Theme.espace2
        clip: true

        TextArea {
            id: editor
            objectName: "notesEditor"
            property bool loading: false
            // Notes de la page reçues assainies (NotesController, Bloquant 5) : sans ressource externe à charger.
            function load() {
                if (editor.text === appNotes.html)
                    return
                loading = true
                editor.text = appNotes.html
                loading = false
            }
            width: scroll.availableWidth
            wrapMode: TextEdit.Wrap
            textFormat: TextEdit.RichText
            readOnly: !appNotes.editable
            color: Theme.texte
            selectByMouse: true
            placeholderText: qsTr("Écrivez ici les notes de cette page…")
            placeholderTextColor: Theme.texteDesactive
            background: null
            Accessible.name: qsTr("Notes de la page")
            onTextChanged: if (!loading) appNotes.html = editor.text

            Shortcut {
                sequences: [StandardKey.Bold]
                enabled: editor.activeFocus
                onActivated: appNotes.toggleStyle(editor.textDocument, editor.selectionStart, editor.selectionEnd, "bold")
            }
            Shortcut {
                sequences: [StandardKey.Italic]
                enabled: editor.activeFocus
                onActivated: appNotes.toggleStyle(editor.textDocument, editor.selectionStart, editor.selectionEnd, "italic")
            }
            Shortcut {
                sequences: [StandardKey.Underline]
                enabled: editor.activeFocus
                onActivated: appNotes.toggleStyle(editor.textDocument, editor.selectionStart, editor.selectionEnd, "underline")
            }
        }
    }
}
