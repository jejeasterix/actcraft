import QtQuick
import QtWebEngine

// Vue du cadre web sur PC et Mac (cadrage DA1, n° 11, 12, 33). Même contrat que WebFrameSystemView.qml, comparé par
// tst_ui_webframelayer. Racine Item : les noms du contrat (goBack, reload…) sont déjà ceux de WebEngineView. L'import est
// écrit en clair pour qmlimportscanner ; le module ne se charge qu'au premier cadre (Loader de WebFrameLayer.qml).
Item {
    id: root
    objectName: "webFrameView"
    property string html: ""
    property url baseUrl: ""
    property url directUrl: ""
    property url contentFile: ""
    property int serial: 0
    // Fixé à la création (initial de Loader.setSource) : profil et JavaScript ne se changent pas à chaud.
    property bool guardsEnabled: false
    readonly property bool overlaysScene: false
    readonly property bool viewFocused: web.activeFocus

    signal viewReported(url pageUrl, bool canBack, bool canForward, string pageTitle, bool pageLoading)
    signal navigationTaken(url pageUrl)
    signal pageLoaded()

    function load() {
        if (directUrl.toString() !== "")
            web.url = directUrl
        else if (contentFile.toString() !== "")
            web.url = contentFile
        else
            web.loadHtml(html, guardsEnabled ? "" : baseUrl)
    }
    function stop() { web.stop() }
    function goBack() { web.goBack() }
    function goForward() { web.goForward() }
    function reload() { web.reload() }
    function capture(callback) {
        if (!web.grabToImage(function (result) { callback(result.image) }))
            callback(null)
    }
    function copySelection() { web.triggerWebAction(WebEngineView.Copy) }
    function _report() {
        root.viewReported(web.url, web.canGoBack, web.canGoForward, web.title, web.loading)
    }

    // Prototype sans storageName : profil éphémère neuf à chaque création (instance()), détruit avec cette vue.
    Component {
        id: guardedProfile
        WebEngineProfilePrototype {}
    }

    WebEngineView {
        id: web
        objectName: "webFrameEngine"
        anchors.fill: parent
        // Gardes allumées : profil neuf et éphémère par vue (H8-13) ; sinon le profil par défaut, éphémère lui aussi.
        profile: root.guardsEnabled ? guardedProfile.createObject(root).instance() : WebEngine.defaultProfile
        settings.javascriptEnabled: !root.guardsEnabled
        settings.localStorageEnabled: !root.guardsEnabled
        settings.localContentCanAccessFileUrls: !root.guardsEnabled
        settings.localContentCanAccessRemoteUrls: !root.guardsEnabled
        // E8 : une fenêtre surgissante reste dans le cadre ; les permissions de la page sont refusées.
        onNewWindowRequested: function (request) { web.url = request.requestedUrl }
        onPermissionRequested: function (permission) { permission.deny() }
        onUrlChanged: {
            if (root.guardsEnabled)
                root.navigationTaken(url)
            root._report()
        }
        onTitleChanged: root._report()
        onCanGoBackChanged: root._report()
        onCanGoForwardChanged: root._report()
        onLoadingChanged: function (info) {
            if (root.guardsEnabled && info.status === WebEngineView.LoadStartedStatus)
                root.navigationTaken(info.url)
            if (info.status === WebEngineView.LoadSucceededStatus)
                root.pageLoaded()
            root._report()
        }
    }
}
