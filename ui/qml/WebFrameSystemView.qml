import QtQuick
import QtWebView

// Vue du cadre web sous Android (cadrage n° 2, 13, 33, 38) : la vue native passe au-dessus de tout le QML. Déclaré
// sous Android seulement : l'analyse des imports de l'APK voit ainsi QtWebView.
Item {
    id: root
    objectName: "webFrameView"
    property string html: ""
    property url baseUrl: ""
    property url directUrl: ""
    property url contentFile: ""
    property int serial: 0
    property bool guardsEnabled: false
    readonly property bool overlaysScene: true
    readonly property bool viewFocused: false

    signal viewReported(url pageUrl, bool canBack, bool canForward, string pageTitle, bool pageLoading)
    signal navigationTaken(url pageUrl)
    signal pageLoaded()

    // contentFile n'est pas lu. Base non vide : loadHtml passe à loadDataWithBaseURL, sans la limite de 2 Mo des
    // adresses, et un HTML lourd du lecteur YouTube garde l'identité (OB58). Base vide : loadData, adresse data:
    // limitée (docs/verifications/2026-11-13-youtube-erreur-153.md).
    function load() {
        if (directUrl.toString() !== "")
            web.url = directUrl
        else
            web.loadHtml(html, guardsEnabled ? "" : baseUrl)
    }
    function stop() { web.stop() }
    function goBack() { web.goBack() }
    function goForward() { web.goForward() }
    function reload() { web.reload() }
    function capture(callback) { callback(null) } // Android : capture par la fenêtre, n° 18
    function copySelection() {}
    function _report() {
        root.viewReported(web.url, web.canGoBack, web.canGoForward, web.title, web.loading)
    }

    // Magasin de cookies commun et durable : vidé à la création et à la destruction (H8-13).
    Component.onCompleted: web.deleteAllCookies()
    Component.onDestruction: web.deleteAllCookies()

    WebView {
        id: web
        anchors.fill: parent
        settings.javaScriptEnabled: !root.guardsEnabled
        settings.allowFileAccess: !root.guardsEnabled
        settings.localContentCanAccessFileUrls: !root.guardsEnabled
        settings.localStorageEnabled: !root.guardsEnabled
        onUrlChanged: {
            if (root.guardsEnabled)
                root.navigationTaken(url)
            root._report()
        }
        onTitleChanged: root._report()
        onLoadingChanged: function (request) {
            if (root.guardsEnabled && request.status === WebView.LoadStartedStatus)
                root.navigationTaken(request.url)
            if (request.status === WebView.LoadSucceededStatus)
                root.pageLoaded()
            root._report()
        }
    }
}
