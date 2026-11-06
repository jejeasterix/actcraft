import QtQuick
import QtQuick.Dialogs
import ActCraft.Ui

// Commandes de fichier (F-700) : avec une URL, sans dialogue ; sans URL, le dialogue de fichier (natif ;
// content:// sous Android). Tout ce qui remplace un document modifié demande d'abord de l'enregistrer.
// Lot 5 (F-010) : Nouveau et Ouvrir ouvrent un onglet, sans rien demander (le document affiché est garé, pas
// remplacé) ; seul un « Sans titre » vierge et non modifié est remplacé par Ouvrir, dans son onglet (H5-23).
// Fermer ferme l'onglet affiché, comme sa croix (DocumentTabBar) ; le dernier devient un « Sans titre » vierge.
// Quitter demande l'enregistrement de chaque onglet modifié, affiché ou garé, l'un après l'autre (L5-C11).
Item {
    id: actions
    objectName: "fileActions"
    required property DocumentController document
    required property DocumentTabs tabs
    // Édition de texte en cours : validée avant de lire `modified` (sinon Fermer, Nouveau ou Quitter la perdraient).
    property TextEditing textEditing: null
    signal popupOpened()
    signal popupClosed()
    // « Enregistrer sous » abandonné : ce qui attendait l'enregistrement (fermer un onglet) est oublié.
    signal saveAbandoned()
    property var preferences: null // lié par L6-T23 (Main.qml) à appPreferences
    readonly property alias recents: recentList
    signal saved(url url) // enregistrement réussi ; Main.qml décide du résumé (L6-T23)
    property var pending: null    // action à reprendre après « enregistrer ? »
    property var afterSave: null  // action à reprendre après un enregistrement réussi
    property bool quitting: false
    // « Enregistrer » demandé pendant une lecture ou une écriture : relancé à sa fin, puis l'action en attente
    // (« Quitter » après « Enregistrer les modifications ? ») reprend (L2-C9). Jamais refusé en silence ; abandonné
    // si une ouverture réussie a remplacé le document visé.
    property bool saveWaiting: false
    // « Quitter » demandé pendant une lecture ou une écriture : reprend à cet onglet quand elle finit (-1 : rien).
    property int quitResume: -1

    function resumeQuit() {
        const from = actions.quitResume
        if (from < 0) return
        actions.quitResume = -1
        actions.quitFrom(from)
    }

    // « Quitter » demandé pendant une opération reprend une seule fois, quand plus rien n'est en cours ni en
    // attente (lecture, écriture, enregistrement ou action à suivre, boîte ouverte).
    function resumeWhenIdle() {
        if (actions.quitResume < 0 || actions.document.busy || actions.saveWaiting || actions.afterSave
            || actions.pending || saveChanges.visible)
            return
        actions.resumeQuit()
    }

    // Valide le texte en cours d'édition, s'il y en a un.
    function commitText() {
        if (actions.textEditing) actions.textEditing.commit()
    }

    function guard(action) {
        actions.commitText()
        if (!actions.document.modified) { action(); return }
        actions.pending = action
        saveChanges.documentTitle = actions.document.title
        saveChanges.open()
    }
    // Un onglet neuf, puis `fill` le met à la taille voulue. Refusé pendant une lecture ou une écriture (le
    // nombre d'onglets ne change pas) : rien n'est alors remplacé.
    function inNewTab(fill) {
        actions.commitText()
        const before = actions.tabs.count
        actions.tabs.newTab()
        if (actions.tabs.count > before) fill()
    }
    function newDocument(w, h) { inNewTab(function () { actions.document.newDocument(w, h) }) }
    function newDocumentWithFrames(w, h, across, down) {
        inNewTab(function () { actions.document.newDocumentWithFrames(w, h, across, down) })
    }
    function openUrl(url) {
        actions.commitText()
        if (!actions.document.modified && actions.document.fileUrl.toString() === "")
            actions.document.openUrl(url)
        else
            actions.tabs.openInNewTab(url)
    }
    function askSize() { sizeDialog.open() }
    function open(argument) {
        const url = actions.document.urlFromArgument(argument)
        if (url.toString() === "") {
            // Le dialogue crée le dossier par défaut en s'ouvrant (jamais ailleurs).
            openDialog.currentFolder = actions.document.defaultFolderUrl(true)
            openDialog.open()
        } else {
            actions.openUrl(url)
        }
    }
    function save() {
        if (actions.document.busy) { actions.saveWaiting = true; return }
        if (actions.document.fileUrl.toString() === "") saveAs(undefined)
        else actions.document.saveToUrl(actions.document.fileUrl)
    }
    function saveAs(argument) {
        const url = actions.document.urlFromArgument(argument)
        if (url.toString() !== "") { actions.document.saveToUrl(url); return }
        const folder = actions.document.defaultFolderUrl(true)
        saveDialog.currentFolder = folder
        saveDialog.selectedFile = folder + "/" + encodeURIComponent(actions.document.suggestedFileName)
        saveDialog.open()
    }
    function close() {
        actions.commitText()
        actions.tabs.requestClose(actions.tabs.currentIndex)
    }
    // Un onglet, affiché ou garé, a-t-il des modifications ? (fermeture de la fenêtre)
    function hasUnsavedTabs() {
        actions.commitText()
        for (let i = 0; i < actions.tabs.count; ++i)
            if (actions.tabs.isModifiedAt(i)) return true
        return false
    }
    function quit() {
        actions.commitText()
        actions.quitFrom(0)
    }
    // Demande l'enregistrement du premier onglet modifié à partir de `from` (il est montré d'abord), puis passe
    // au suivant ; « Annuler » ou un « Enregistrer sous » refusé abandonne la sortie. Quitte quand tous sont réglés.
    function quitFrom(from) {
        actions.quitResume = -1 // la chaîne vit : elle repose elle-même sa reprise si une bascule est refusée
        for (let i = from; i < actions.tabs.count; ++i) {
            if (!actions.tabs.isModifiedAt(i)) continue
            if (i !== actions.tabs.currentIndex) actions.tabs.activate(i)
            if (i !== actions.tabs.currentIndex) {
                // Lecture ou écriture en cours : la bascule est refusée ; la sortie reprend à sa fin.
                actions.quitResume = from
                return
            }
            actions.pending = function () { actions.quitFrom(i + 1) }
            saveChanges.documentTitle = actions.document.title
            saveChanges.open()
            return
        }
        actions.quitting = true
        Qt.quit()
    }

    FileDialog {
        id: openDialog
        objectName: "openDialog"
        title: qsTr("Ouvrir un paperboard")
        fileMode: FileDialog.OpenFile
        nameFilters: [qsTr("Paperboards (*.actcraft *.flipchart)"), qsTr("Tous les fichiers (*)")]
        onAccepted: actions.openUrl(selectedFile)
    }
    FileDialog {
        id: saveDialog
        objectName: "saveDialog"
        title: qsTr("Enregistrer le paperboard sous")
        fileMode: FileDialog.SaveFile
        defaultSuffix: "actcraft"
        nameFilters: [qsTr("Paperboards ActCraft (*.actcraft)")]
        onAccepted: actions.document.saveToUrl(selectedFile)
        // « Enregistrer sous » abandonné après « Enregistrer les modifications ? » : l'action en attente est oubliée.
        onRejected: { actions.afterSave = null; actions.quitResume = -1; actions.saveAbandoned() }
    }
    SaveChangesDialog {
        id: saveChanges
        onOpened: actions.popupOpened()
        onClosed: actions.popupClosed()
        onSaveRequested: { actions.afterSave = actions.pending; actions.pending = null; actions.save() }
        onDiscardRequested: { const a = actions.pending; actions.pending = null; if (a) a(); actions.resumeWhenIdle() }
        onRejected: { actions.pending = null; actions.quitResume = -1 }
    }
    PageSizeDialog {
        id: sizeDialog
        document: actions.document
        onOpened: actions.popupOpened()
        onClosed: actions.popupClosed()
        onSizeAccepted: (w, h, across, down) => actions.newDocumentWithFrames(w, h, across, down)
    }
    Connections {
        target: actions.document
        function onSaveFinished(ok, message) {
            // L'enregistrement en attente part d'abord, l'action qui le suit attend sa fin à lui ; sauf si
            // l'écriture finie a déjà mis l'écran dans le fichier (document propre) : l'action reprend aussitôt.
            if (actions.saveWaiting) {
                actions.saveWaiting = false
                if (!ok || actions.document.modified) { actions.save(); return }
            }
            const a = actions.afterSave
            actions.afterSave = null
            if (ok && a) a()
            actions.resumeWhenIdle()
        }
        function onLoadFinished(ok, message) {
            if (actions.saveWaiting) {
                actions.saveWaiting = false
                // Lecture ratée : le document est le même, l'enregistrement demandé part. Lecture réussie : il
                // visait le document remplacé, il est abandonné (jamais relancé sur le document ouvert) ; l'action
                // en attente est réévaluée sur le nouveau document (revue de L2-C9).
                if (!ok) { actions.save(); return }
                const a = actions.afterSave
                actions.afterSave = null
                if (a) actions.guard(a)
            }
            actions.resumeWhenIdle()
        }
        // Diffusion protégée (L8-T31) : le document ouvert est le même ; l'enregistrement demandé pendant
        // l'écriture de la copie part, puis l'action en attente (« Quitter ») reprend à sa fin à lui.
        function onPublishFinished(ok, message) {
            if (actions.saveWaiting) {
                actions.saveWaiting = false
                actions.save()
                return
            }
            actions.resumeWhenIdle()
        }
    }
    RecentDocuments {
        id: recentList
        objectName: "appRecents"
        preferences: actions.preferences
    }
    // Documents récents (L6-T24) : à côté de la Connections du lot 5, qui reste inchangée.
    Connections {
        target: actions.document
        function onSaveFinished(ok, message) {
            if (!ok)
                return
            recentList.note(actions.document.fileUrl)
            actions.saved(actions.document.fileUrl)
        }
        function onLoadFinished(ok, message) {
            if (ok)
                recentList.note(actions.document.fileUrl)
        }
    }
}
