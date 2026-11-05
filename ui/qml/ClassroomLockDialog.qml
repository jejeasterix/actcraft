import QtQuick
import QtQuick.Controls.Basic
import ActCraft.Ui

// Mode classe protégé (F-298) : création du mot de passe au premier verrouillage (deux saisies),
// déverrouillage, changement (l'ancien d'abord). Le mot de passe ne quitte ces champs que vers ClassroomLock,
// qui n'en garde que l'empreinte ; les champs sont vidés dès la saisie lancée. Aucune voie d'ici ne lève le
// verrou sans lui : seul ClassroomLock appelle Commands::setLocked (C++).
Item {
    id: root
    objectName: "classroomLockDialog"
    property Commands commands: null
    property DocumentController documentController: null
    property Tools tools: null
    property PageView view: null
    property var profile: null
    property var preferences: null
    visible: false

    // « create » (puis verrouiller si lockAfter), « unlock », « change ».
    property string mode: "create"
    property bool lockAfter: false
    property string message: ""
    readonly property bool opened: popup.opened

    ClassroomLock {
        id: appClassroom
        objectName: "appClassroom"
        commands: root.commands
        preferences: root.preferences
        onLockedChanged: root.commands.setChecked("classroom.lock", appClassroom.locked)
        onPasswordCreated: ok => {
            if (!ok) {
                root.fail(qsTr("Le mot de passe n'a pas pu être créé."))
                return
            }
            if (root.lockAfter)
                appClassroom.lock()
            popup.close()
        }
        onUnlocked: ok => {
            if (ok)
                popup.close()
            else
                root.fail(qsTr("Mot de passe incorrect."))
        }
        onPasswordReplaced: ok => {
            if (ok)
                popup.close()
            else
                root.fail(qsTr("L'ancien mot de passe est incorrect."))
        }
    }

    function clearFields() {
        first.text = ""
        second.text = ""
        third.text = ""
    }
    function ask(newMode, thenLock) {
        root.mode = newMode
        root.lockAfter = thenLock
        root.message = ""
        root.clearFields()
        popup.open()
    }
    function fail(text) {
        root.message = text
        root.clearFields()
        first.forceActiveFocus()
    }
    function submit() {
        if (appClassroom.busy)
            return
        if (root.mode === "unlock") {
            appClassroom.unlock(first.text)
            root.clearFields()
            return
        }
        const fresh = root.mode === "create" ? first.text : second.text
        const again = root.mode === "create" ? second.text : third.text
        if (fresh.length === 0) {
            root.fail(qsTr("Saisissez un mot de passe."))
            return
        }
        if (fresh !== again) {
            root.fail(qsTr("Les deux saisies diffèrent."))
            return
        }
        if (root.mode === "create")
            appClassroom.createPassword(fresh, again)
        else
            appClassroom.changePassword(first.text, fresh)
        root.clearFields()
    }

    Component.onCompleted: {
        root.commands.setHandler("classroom.lock", function () {
            if (appClassroom.locked)
                root.ask("unlock", false)
            else if (!appClassroom.hasPassword)
                root.ask("create", true)
            else
                appClassroom.lock()
        })
        root.commands.setHandler("classroom.password", function () {
            root.ask(appClassroom.hasPassword ? "change" : "create", false)
        })
        root.commands.setChecked("classroom.lock", appClassroom.locked)
    }

    Popup {
        id: popup
        objectName: "classroomLockPopup"
        parent: Overlay.overlay
        anchors.centerIn: parent
        modal: true
        focus: true
        closePolicy: Popup.CloseOnEscape
        padding: Theme.espace2 * 2
        onOpened: first.forceActiveFocus()
        onClosed: root.clearFields()

        contentItem: Column {
            spacing: Theme.espace2
            width: 360

            Label {
                text: root.mode === "unlock" ? qsTr("Saisissez le mot de passe de la classe.")
                      : root.mode === "change" ? qsTr("Changez le mot de passe de la classe.")
                      : qsTr("Créez le mot de passe de la classe (deux fois).")
                wrapMode: Text.WordWrap
                width: parent.width
                font.pixelSize: Theme.taillePolice
                color: Theme.texte
            }
            TextField {
                id: first
                objectName: "classroomPasswordFirst"
                width: parent.width
                echoMode: TextInput.Password
                placeholderText: root.mode === "change" ? qsTr("Ancien mot de passe") : qsTr("Mot de passe")
                onAccepted: root.mode === "unlock" ? root.submit() : second.forceActiveFocus()
            }
            TextField {
                id: second
                objectName: "classroomPasswordSecond"
                visible: root.mode !== "unlock"
                width: parent.width
                echoMode: TextInput.Password
                placeholderText: root.mode === "change" ? qsTr("Nouveau mot de passe") : qsTr("Confirmation")
                onAccepted: root.mode === "create" ? root.submit() : third.forceActiveFocus()
            }
            TextField {
                id: third
                objectName: "classroomPasswordThird"
                visible: root.mode === "change"
                width: parent.width
                echoMode: TextInput.Password
                placeholderText: qsTr("Confirmation")
                onAccepted: root.submit()
            }
            Label {
                objectName: "classroomLockMessage"
                visible: text.length > 0
                text: root.message
                textFormat: Text.PlainText
                wrapMode: Text.WordWrap
                width: parent.width
                font.pixelSize: Theme.taillePolice
                color: Theme.texte
            }
            Row {
                spacing: Theme.espace2
                Button {
                    text: qsTr("Annuler")
                    onClicked: popup.close()
                }
                Button {
                    text: qsTr("Valider")
                    enabled: !appClassroom.busy
                    onClicked: root.submit()
                }
            }
        }
    }
}
