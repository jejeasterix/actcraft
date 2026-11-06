import QtQuick
import QtQuick.Controls.Basic
import ActCraft.Ui

// Cadres web de la page (S27, F-302) : une vue Qt WebView par cadre visible, posée sur son repère, avec sa
// barre (précédent, suivant, recharger, image fixe) et sa poignée (une étape au relâché).
//
// La vue ne reçoit jamais d'adresse : elle charge le HTML que WebFrameItems a lu et réécrit après la décision
// de la politique (L8-T29), par loadHtml ; une navigation qu'elle prendrait d'elle-même est rendue au modèle
// et arrêtée. JavaScript, accès aux fichiers et stockage local coupés. Hors de la scène que le cache
// photographie : miniatures, exports et page fixe montrent le repère neutre de paintPage. La vue native passe
// au-dessus de tout le QML : elle est masquée tant qu'une boîte de la fenêtre est ouverte.
Item {
    id: layer
    objectName: "webFrameLayer"
    anchors.fill: parent
    property var commands: null

    // Zone de page qui porte la couche ; la vue de page s'atteint par pageArea.view (plan 8a, l. 248).
    readonly property Item pageArea: findArea()
    readonly property var view: pageArea ? pageArea.view : null
    readonly property int count: appWebFrames.count
    readonly property int blockedCount: appWebFrames.blockedCount
    // Cadre visé par les commandes sans argument : le dernier touché, sinon le premier.
    property int currentRow: -1
    readonly property int targetRow: currentRow >= 0 && currentRow < count ? currentRow : (count > 0 ? 0 : -1)
    property var dialog: null
    readonly property bool covered: confirm.opened || (dialog !== null && dialog.opened)

    WebFrameItems {
        id: appWebFrames
        objectName: "appWebFrames"
        document: layer.pageArea ? layer.pageArea.documentController : null
    }

    function findArea() {
        for (let p = parent; p; p = p.parent) {
            if (p.objectName === "pageArea")
                return p
        }
        return null
    }
    function findDialog() {
        let top = layer
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
        const a = layer.mapFromItem(null, view.pageToScene(Qt.point(b.x, b.y)))
        const c = layer.mapFromItem(null, view.pageToScene(Qt.point(b.x + b.width, b.y + b.height)))
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

    onViewChanged: if (view) appWebFrames.useWindowCapture(view)

    Component.onCompleted: {
        dialog = findDialog()
        if (view)
            appWebFrames.useWindowCapture(view)
        if (!commands)
            return
        // Argument (scénarios) : { html, address } crée le cadre sans la boîte.
        commands.setHandler("insert.web-frame", function (arg) {
            if (arg && (arg.html || arg.address)) {
                appWebFrames.insertFrame(arg.html || "", arg.address || "")
            } else if (layer.dialog) {
                layer.dialog.open(appWebFrames)
            }
        })
        commands.setHandler("webframe.back", function (arg) { appWebFrames.goBack(layer.rowOf(arg)) })
        commands.setHandler("webframe.forward", function (arg) { appWebFrames.goForward(layer.rowOf(arg)) })
        commands.setHandler("webframe.reload", function (arg) { appWebFrames.reload(layer.rowOf(arg)) })
        commands.setHandler("webframe.snapshot", function (arg) {
            const ok = appWebFrames.snapshot(layer.rowOf(arg))
            console.info("ACTCRAFT IMAGE DU CADRE " + (ok ? "OK" : "ECHEC"))
        })
        // Déblocage : confirmation, puis cette page de ce document seulement.
        commands.setHandler("webframe.unblock", function (arg) {
            appWebFrames.requestUnblock(layer.rowOf(arg))
            if (appWebFrames.unblockPending)
                confirm.open()
        })
    }

    Repeater {
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
            required property string title
            required property bool canGoBack
            required property bool canGoForward
            required property int serial

            // Glisser de la poignée : aperçu sans étape ; l'étape part au relâché.
            property point resizeBy: Qt.point(0, 0)
            readonly property rect box: layer.frameRect(bounds)
            readonly property bool showsView: !blocked && !refused && content.length > 0
            x: box.x
            y: box.y
            width: Math.max(24, box.width + resizeBy.x)
            height: Math.max(24, box.height + resizeBy.y)

            Item {
                id: viewHost
                anchors.fill: parent
                clip: true
                visible: frame.showsView && !layer.covered
                property var web: null
                function show() {
                    if (!frame.showsView || !appWebFrames.engineBuilt)
                        return
                    if (!web) {
                        try {
                            web = Qt.createQmlObject("import QtQuick\nimport QtWebView\nWebView {\n"
                                                     + " anchors.fill: parent\n"
                                                     + " settings.javaScriptEnabled: false\n"
                                                     + " settings.allowFileAccess: false\n"
                                                     + " settings.localContentCanAccessFileUrls: false\n"
                                                     + " settings.localStorageEnabled: false\n}",
                                                     viewHost, "WebFrameView")
                        } catch (e) {
                            console.warn("Cadre web : vue indisponible (" + e + ")")
                            return
                        }
                        // Toute adresse prise par la vue repasse par le modèle (renvoi interne ou navigation
                        // du moteur) ; s'il la prend en charge, la vue s'arrête.
                        web.urlChanged.connect(function () {
                            if (appWebFrames.followLink(frame.index, web.url))
                                web.stop()
                        })
                        web.loadingChanged.connect(function (request) {
                            if (request.status === 0 && appWebFrames.followLink(frame.index, request.url))
                                web.stop()
                        })
                    }
                    web.loadHtml(frame.content, frame.baseUrl)
                }
            }
            onSerialChanged: viewHost.show()
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
                            layer.currentRow = frame.index
                            if (layer.commands)
                                layer.commands.trigger("webframe.unblock", frame.index)
                        }
                    }
                }
                // Couverture d'entrée tant qu'il est montré : ailleurs, la page reste aux outils.
                onVisibleChanged: visible ? layer.cover(blockedMark) : layer.uncover(blockedMark)
                Component.onCompleted: if (visible) layer.cover(blockedMark)
                Component.onDestruction: layer.uncover(blockedMark)
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
                        onClicked: if (layer.commands) layer.commands.trigger("webframe.back", frame.index)
                    }
                    ToolButton {
                        text: "›"
                        enabled: frame.canGoForward
                        height: parent.height
                        onClicked: if (layer.commands) layer.commands.trigger("webframe.forward", frame.index)
                    }
                    ToolButton {
                        text: "↻"
                        height: parent.height
                        onClicked: if (layer.commands) layer.commands.trigger("webframe.reload", frame.index)
                    }
                    ToolButton {
                        text: qsTr("Image fixe")
                        height: parent.height
                        enabled: frame.showsView
                        onClicked: if (layer.commands) layer.commands.trigger("webframe.snapshot", frame.index)
                    }
                    Label {
                        anchors.verticalCenter: parent.verticalCenter
                        width: Math.max(0, bar.width - x - 4)
                        elide: Text.ElideRight
                        text: frame.loading ? qsTr("Chargement…") : frame.title
                    }
                }
                TapHandler {
                    onTapped: layer.currentRow = frame.index
                }
                Component.onCompleted: layer.cover(bar)
                Component.onDestruction: layer.uncover(bar)
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
                        if (active || !layer.view)
                            return
                        const end = layer.mapToItem(null, frame.x + frame.width, frame.y + frame.height)
                        const corner = layer.view.sceneToPage(Qt.point(end.x, end.y))
                        frame.resizeBy = Qt.point(0, 0)
                        layer.currentRow = frame.index
                        appWebFrames.resizeFrame(frame.index, Qt.rect(frame.bounds.x, frame.bounds.y,
                                                                      Math.max(1, corner.x - frame.bounds.x),
                                                                      Math.max(1, corner.y - frame.bounds.y)))
                    }
                }
                Component.onCompleted: layer.cover(handle)
                Component.onDestruction: layer.uncover(handle)
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
    }
}
