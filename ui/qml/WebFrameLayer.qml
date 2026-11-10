import QtQuick
import QtQuick.Controls.Basic
import ActCraft.Ui

// Cadres web de la page (S27, F-302) : une vue par cadre visible, chargée par un Loader, posée sur son repère, avec sa
// barre (précédent, suivant, recharger, image fixe) et sa poignée (une étape au relâché).
//
// Gardes allumées : la vue ne reçoit jamais d'adresse, elle charge le HTML que WebFrameItems a lu et réécrit
// après la décision de la politique (L8-T29) ; une navigation qu'elle prendrait d'elle-même est rendue au
// modèle et arrêtée. JavaScript, accès aux fichiers et stockage local coupés. Hors de la scène que le cache
// photographie : miniatures, exports et page fixe montrent le repère neutre de paintPage. Sur Android, la vue
// native passe au-dessus de tout le QML ; sur PC et Mac, c'est un élément de la scène, couvert par la
// couverture ci-dessous. Dans les deux cas elle est masquée tant qu'une boîte de la fenêtre est ouverte.
Item {
    id: frameLayer
    objectName: "webFrameLayer"
    anchors.fill: parent
    property var commands: null
    property var profile: null

    // Zone de page qui porte la couche ; la vue de page s'atteint par pageArea.view (plan 8a, l. 248).
    readonly property Item pageArea: findArea()
    readonly property var view: pageArea ? pageArea.view : null
    readonly property int count: appWebFrames.count
    readonly property int blockedCount: appWebFrames.blockedCount
    // Cadre visé par les commandes sans argument : le dernier touché, sinon le premier.
    property int currentRow: -1
    readonly property int targetRow: currentRow >= 0 && currentRow < count ? currentRow : (count > 0 ? 0 : -1)
    property var dialog: null
    // Composant de la vue : celui du moteur construit ; un test en pose un autre avant le premier cadre.
    property url viewSource: appWebFrames.engineKind === "engine" ? Qt.resolvedUrl("WebFrameEngineView.qml")
                           : appWebFrames.engineKind === "system" ? Qt.resolvedUrl("WebFrameSystemView.qml") : ""
    property int loadedViews: 0   // vues dont une page a fini de charger (lu par les tests et le banc)
    property int snapshotCount: 0 // images fixes terminées
    // Vue masquée tant qu'une boîte de la fenêtre est ouverte (la vue native d'Android passe au-dessus de tout le QML), la
    // nôtre (ci-dessous, relayée) ou toute autre (menus compris : Main.openPopups, via PageArea.inputBlocked).
    readonly property bool covered: confirm.opened || (dialog !== null && dialog.opened) ||
                                    (pageArea !== null && pageArea.inputBlocked)

    WebFrameItems {
        id: appWebFrames
        objectName: "appWebFrames"
        profile: frameLayer.profile
        document: frameLayer.pageArea ? frameLayer.pageArea.documentController : null
    }

    // La boîte du cadre (Main.qml) recouvre la page comme les autres : relayée à la fenêtre par PageArea
    // (popupOpened/popupClosed vers Main.openPopups), pour que le stylet ne dessine pas derrière et que Échap
    // ne soit plus global tant qu'elle est ouverte.
    Connections {
        target: frameLayer.dialog
        ignoreUnknownSignals: true
        function onOpenedChanged() {
            if (!frameLayer.pageArea)
                return
            if (frameLayer.dialog.opened)
                frameLayer.pageArea.popupOpened()
            else
                frameLayer.pageArea.popupClosed()
        }
    }

    function findArea() {
        for (let p = parent; p; p = p.parent) {
            if (p.objectName === "pageArea")
                return p
        }
        return null
    }
    function findDialog() {
        let top = frameLayer
        while (top.parent)
            top = top.parent
        for (let i = 0; i < top.data.length; ++i) {
            if (top.data[i].objectName === "webFrameDialog")
                return top.data[i]
        }
        return null
    }
    function rowOf(arg) {
        return (typeof arg === "number" && arg >= 0 && arg < count) ? arg : targetRow
    }
    function frameRect(b) {
        if (!view)
            return Qt.rect(0, 0, 0, 0)
        // Dépendances : le cadre suit le zoom et le défilement de la vue.
        const zoom = view.zoom
        const pan = view.pan
        const a = frameLayer.mapFromItem(null, view.pageToScene(Qt.point(b.x, b.y)))
        const c = frameLayer.mapFromItem(null, view.pageToScene(Qt.point(b.x + b.width, b.y + b.height)))
        return Qt.rect(Math.min(a.x, c.x), Math.min(a.y, c.y), Math.abs(c.x - a.x), Math.abs(c.y - a.y))
    }
    function cover(item) {
        if (view)
            view.addInputCover(item)
    }
    function uncover(item) {
        if (view)
            view.removeInputCover(item)
    }

    // Image fixe du cadre (n° 18). Vue du moteur : grabToImage, répété jusqu'à deux images identiques, car
    // pageLoaded précède la peinture du texte. Vue native (Android) : capture par la fenêtre.
    property var pendingSnapshot: null
    function takeSnapshot(arg) {
        const row = rowOf(arg)
        const delegate = row >= 0 ? frames.itemAt(row) : null
        const webView = delegate ? delegate.webView : null
        if (webView && !webView.overlaysScene) {
            if (pendingSnapshot) {
                finishSnapshot(false)
                return
            }
            pendingSnapshot = { row: row, web: webView, first: true, tries: 20 }
            snapshotStep()
            return
        }
        finishSnapshot(appWebFrames.snapshot(row))
    }
    function snapshotStep() {
        const job = pendingSnapshot
        job.web.capture(function (image) {
            if (pendingSnapshot !== job)
                return
            if (!image) {
                pendingSnapshot = null
                finishSnapshot(false)
            } else if ((appWebFrames.sameAsPreviousCapture(image, job.first)) || --job.tries <= 0) {
                pendingSnapshot = null
                finishSnapshot(appWebFrames.insertSnapshot(job.row, image))
            } else {
                job.first = false
                snapshotTimer.restart()
            }
        })
    }
    function finishSnapshot(ok) {
        console.info("ACTCRAFT IMAGE DU CADRE " + (ok ? "OK" : "ECHEC"))
        snapshotCount += 1
    }
    Timer {
        id: snapshotTimer
        interval: 100
        onTriggered: if (frameLayer.pendingSnapshot) frameLayer.snapshotStep()
    }

    onViewChanged: if (view && Qt.platform.os === "android") appWebFrames.useWindowCapture(view)

    Component.onCompleted: {
        dialog = findDialog()
        if (view && Qt.platform.os === "android")
            appWebFrames.useWindowCapture(view)
        if (!commands)
            return
        // Argument (scénarios) : { html, address } crée le cadre sans la boîte.
        commands.setHandler("insert.web-frame", function (arg) {
            if (arg && (arg.html || arg.address)) {
                appWebFrames.insertFrame(arg.html || "", arg.address || "")
            } else if (frameLayer.dialog) {
                frameLayer.dialog.open(appWebFrames)
            }
        })
        commands.setHandler("webframe.back", function (arg) { appWebFrames.goBack(frameLayer.rowOf(arg)) })
        commands.setHandler("webframe.forward", function (arg) { appWebFrames.goForward(frameLayer.rowOf(arg)) })
        commands.setHandler("webframe.reload", function (arg) { appWebFrames.reload(frameLayer.rowOf(arg)) })
        commands.setHandler("webframe.snapshot", function (arg) {
            frameLayer.takeSnapshot(arg)
        })
        // Déblocage : confirmation, puis cette page de ce document seulement.
        commands.setHandler("webframe.unblock", function (arg) {
            appWebFrames.requestUnblock(frameLayer.rowOf(arg))
            if (appWebFrames.unblockPending)
                confirm.open()
        })
    }

    Repeater {
        id: frames
        model: appWebFrames
        delegate: Item {
            id: frame
            required property int index
            required property rect bounds
            required property bool blocked
            required property bool refused
            required property bool loading
            required property string content
            required property url baseUrl
            required property url directUrl
            required property url contentFile
            required property string title
            required property bool canGoBack
            required property bool canGoForward
            required property int serial

            // Glisser de la poignée : aperçu sans étape ; l'étape part au relâché.
            property point resizeBy: Qt.point(0, 0)
            readonly property rect box: frameLayer.frameRect(bounds)
            readonly property bool showsView: !blocked && !refused && (content.length > 0 || directUrl.toString().length > 0)
            readonly property var webView: viewHost.webView
            x: box.x
            y: box.y
            width: Math.max(24, box.width + resizeBy.x)
            height: Math.max(24, box.height + resizeBy.y)

            Item {
                id: viewHost
                objectName: "webFrameViewHost"
                anchors.fill: parent
                clip: true
                visible: frame.showsView && !(frameLayer.covered && loader.item !== null && loader.item.overlaysScene)
                property bool reported: false
                // Vue déclarée couverture d'entrée (n° 17) : sans cela le filtre de PageView prend le stylet.
                property var covering: null
                Component.onDestruction: if (covering) frameLayer.uncover(covering)
                // Réglage pour lequel la vue a été créée. Un autre document : le modèle est remis à zéro (rebuild), le
                // délégué et sa vue sont recréés, donc un profil neuf par document (H8-13).
                property bool madeGuards: false
                readonly property var webView: loader.status === Loader.Ready ? loader.item : null

                Loader {
                    id: loader
                    objectName: "webFrameLoader"
                    anchors.fill: parent
                    asynchronous: true // import QtWebEngine lu à la première vue (DA2)
                    onStatusChanged: {
                        if (status === Loader.Ready) {
                            if (!item.overlaysScene) {
                                viewHost.covering = item
                                frameLayer.cover(item)
                            }
                            viewHost.deliver()
                        } else if (status === Loader.Error) {
                            console.warn("Cadre web : vue indisponible (" + source + ")")
                        } else if (status === Loader.Null && viewHost.covering) {
                            frameLayer.uncover(viewHost.covering)
                            viewHost.covering = null
                        }
                    }
                }
                Connections {
                    target: loader.item
                    ignoreUnknownSignals: true
                    function onNavigationTaken(pageUrl) {
                        // Toute adresse prise par la vue repasse par le modèle ; s'il la prend en charge, la vue s'arrête.
                        if (appWebFrames.followLink(frame.index, pageUrl))
                            loader.item.stop()
                    }
                    function onPageLoaded() {
                        if (viewHost.reported)
                            return
                        viewHost.reported = true
                        frameLayer.loadedViews += 1
                        console.info("ACTCRAFT CADRE WEB CHARGE")
                    }
                    function onViewReported(pageUrl, canBack, canForward, pageTitle, pageLoading) {
                        appWebFrames.reportView(frame.index, pageUrl, canBack, canForward, pageTitle, pageLoading)
                    }
                }
                function deliver() {
                    if (loader.status !== Loader.Ready || !frame.showsView)
                        return
                    const view = loader.item
                    view.html = frame.content
                    view.baseUrl = frame.baseUrl
                    view.directUrl = frame.directUrl
                    view.contentFile = frame.contentFile
                    view.serial = frame.serial
                    view.load()
                }
                function show() {
                    const guards = appWebFrames.guardsEnabled
                    // Profil et JavaScript ne se changent pas à chaud (n° 10, 33) ; avant le test de showsView : une
                    // vue active ne survit pas, cachée, au passage d'un cadre au repère de blocage.
                    if (loader.status !== Loader.Null && madeGuards !== guards)
                        loader.source = ""
                    if (!frame.showsView || frameLayer.viewSource.toString() === "")
                        return
                    if (loader.status === Loader.Null) {
                        madeGuards = guards
                        reported = false
                        loader.setSource(frameLayer.viewSource, { guardsEnabled: guards })
                        return
                    }
                    deliver()
                }
            }
            Connections {
                target: appWebFrames
                function onViewCommand(row, action) {
                    if (row !== frame.index || !viewHost.webView)
                        return
                    if (action === "back")
                        viewHost.webView.goBack()
                    else if (action === "forward")
                        viewHost.webView.goForward()
                    else if (action === "reload")
                        viewHost.webView.reload()
                }
            }
            // Reporté : dataChanged sans rôles notifie les rôles un à un, dans un ordre non garanti ; quand `serial`
            // change, `content` peut être encore l'ancien (Loader jamais lancé, ou contenu périmé livré à la vue).
            onSerialChanged: Qt.callLater(viewHost.show)
            Component.onCompleted: viewHost.show()

            // Repère de blocage : contenu distant d'un document reçu, page non débloquée.
            Rectangle {
                id: blockedMark
                objectName: "webFrameBlocked"
                anchors.fill: parent
                visible: frame.blocked || frame.refused
                color: "#F2F3F5"
                border.color: "#8A8F98"
                Column {
                    anchors.centerIn: parent
                    width: parent.width - 24
                    spacing: 8
                    Label {
                        width: parent.width
                        horizontalAlignment: Text.AlignHCenter
                        wrapMode: Text.Wrap
                        text: frame.blocked ? qsTr("Contenu distant bloqué : %1").arg(frame.title)
                                            : qsTr("Adresse refusée par le cadre web.")
                    }
                    Button {
                        objectName: "webFrameUnblock"
                        anchors.horizontalCenter: parent.horizontalCenter
                        visible: frame.blocked
                        text: qsTr("Débloquer…")
                        onClicked: {
                            frameLayer.currentRow = frame.index
                            if (frameLayer.commands)
                                frameLayer.commands.trigger("webframe.unblock", frame.index)
                        }
                    }
                }
                // Couverture d'entrée tant qu'il est montré : ailleurs, la page reste aux outils.
                onVisibleChanged: visible ? frameLayer.cover(blockedMark) : frameLayer.uncover(blockedMark)
                Component.onCompleted: if (visible) frameLayer.cover(blockedMark)
                Component.onDestruction: frameLayer.uncover(blockedMark)
            }

            // Barre du cadre, au-dessus de son bord haut.
            Rectangle {
                id: bar
                objectName: "webFrameBar"
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.bottom: parent.top
                height: 26
                color: "#E6E8EB"
                border.color: "#8A8F98"
                Row {
                    anchors.fill: parent
                    anchors.margins: 1
                    spacing: 2
                    ToolButton {
                        text: "‹"
                        enabled: frame.canGoBack
                        height: parent.height
                        onClicked: if (frameLayer.commands) frameLayer.commands.trigger("webframe.back", frame.index)
                    }
                    ToolButton {
                        text: "›"
                        enabled: frame.canGoForward
                        height: parent.height
                        onClicked: if (frameLayer.commands) frameLayer.commands.trigger("webframe.forward", frame.index)
                    }
                    ToolButton {
                        text: "↻"
                        height: parent.height
                        onClicked: if (frameLayer.commands) frameLayer.commands.trigger("webframe.reload", frame.index)
                    }
                    ToolButton {
                        text: qsTr("Image fixe")
                        height: parent.height
                        enabled: frame.showsView
                        onClicked: if (frameLayer.commands) frameLayer.commands.trigger("webframe.snapshot", frame.index)
                    }
                    Label {
                        anchors.verticalCenter: parent.verticalCenter
                        width: Math.max(0, bar.width - x - 4)
                        elide: Text.ElideRight
                        text: frame.loading ? qsTr("Chargement…") : frame.title
                    }
                }
                TapHandler {
                    onTapped: frameLayer.currentRow = frame.index
                }
                Component.onCompleted: frameLayer.cover(bar)
                Component.onDestruction: frameLayer.uncover(bar)
            }

            // Poignée : coin bas droit.
            Rectangle {
                id: handle
                objectName: "webFrameHandle"
                width: 14
                height: 14
                x: parent.width - width / 2
                y: parent.height - height / 2
                color: "#FFFFFF"
                border.color: "#5A606B"
                DragHandler {
                    id: drag
                    target: null
                    onTranslationChanged: if (active) frame.resizeBy = Qt.point(translation.x, translation.y)
                    onActiveChanged: {
                        if (active || !frameLayer.view)
                            return
                        const end = frameLayer.mapToItem(null, frame.x + frame.width, frame.y + frame.height)
                        const corner = frameLayer.view.sceneToPage(Qt.point(end.x, end.y))
                        frame.resizeBy = Qt.point(0, 0)
                        frameLayer.currentRow = frame.index
                        appWebFrames.resizeFrame(frame.index, Qt.rect(frame.bounds.x, frame.bounds.y,
                                                                      Math.max(1, corner.x - frame.bounds.x),
                                                                      Math.max(1, corner.y - frame.bounds.y)))
                    }
                }
                Component.onCompleted: frameLayer.cover(handle)
                Component.onDestruction: frameLayer.uncover(handle)
            }
        }
    }

    // Confirmation du déblocage (webframe.unblock).
    Dialog {
        id: confirm
        objectName: "webFrameUnblockConfirm"
        parent: Overlay.overlay
        anchors.centerIn: parent
        modal: true
        title: qsTr("Débloquer le contenu distant")
        standardButtons: Dialog.Yes | Dialog.No
        Label {
            width: 380
            wrapMode: Text.Wrap
            text: qsTr("Le cadre web chargera le contenu distant de cette page, pour ce document seulement. JavaScript reste coupé.")
        }
        onAccepted: appWebFrames.answer(true)
        onRejected: appWebFrames.answer(false)
        onOpened: if (frameLayer.pageArea) frameLayer.pageArea.popupOpened()
        onClosed: if (frameLayer.pageArea) frameLayer.pageArea.popupClosed()
    }
}
