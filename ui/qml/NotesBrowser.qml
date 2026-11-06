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
    // Diffusion protégée (L8-T32, H8-16) : les notes font partie du contenu protégé. La porte filtre Copier et
    // Couper du clavier de l'éditeur ; son menu contextuel perd Copier et Couper quand la copie est interdite.
    property BroadcastGate broadcast: null
    property bool hasNotes: appNotes.hasNotes

    Binding {
        target: root.broadcast
        property: "notesEditor"
        value: editor
        when: root.broadcast !== null
    }

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

            // Copie interdite : notre menu remplace celui de Qt, rendu dès que la copie est permise.
            Binding {
                target: editor.ContextMenu
                property: "menu"
                value: restrictedMenu
                when: root.broadcast !== null && root.broadcast.copyRestricted
            }

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
    // Menu de l'éditeur sans Copier ni Couper (diffusion protégée).
    Menu {
        id: restrictedMenu
        objectName: "notesRestrictedMenu"
        MenuItem { text: qsTr("Annuler"); enabled: editor.canUndo; onTriggered: editor.undo() }
        MenuItem { text: qsTr("Rétablir"); enabled: editor.canRedo; onTriggered: editor.redo() }
        MenuSeparator {}
        MenuItem { text: qsTr("Coller"); enabled: editor.canPaste; onTriggered: editor.paste() }
        MenuItem {
            text: qsTr("Supprimer")
            enabled: !editor.readOnly && editor.selectedText !== ""
            onTriggered: editor.remove(editor.selectionStart, editor.selectionEnd)
        }
        MenuSeparator {}
        MenuItem { text: qsTr("Tout sélectionner"); enabled: editor.length > 0; onTriggered: editor.selectAll() }
    }
}
