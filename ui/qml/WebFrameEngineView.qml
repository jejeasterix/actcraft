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

    // OB52, erreur 153 : un HTML lourd (contentFile) de base https (identité du lecteur YouTube) ne se lit pas
    // en file:, qui n'envoie aucun Referer. Une page d'amorce prend la base, puis le HTML y est écrit
    // (document.write) : la page garde l'origine https. L'amorce porte un repère : tout chargement réussi qui la
    // montre (load, rechargement du moteur ou de son menu, retour) la réécrit ; aucune autre page ne l'est.
    // Écriture par morceaux, un par tour de boucle : d'un bloc, 25 Mo tiennent le fil 170 ms (Mac, OB52).
    readonly property string _primer: "<!DOCTYPE html><html data-actcraft-primer></html>"
    readonly property int _chunk: 524288
    property int _written: 0
    property string _token: ""
    property bool _writing: false
    // Chaque load() change de génération : un rappel de runJavaScript parti avant lui est ignoré.
    property int _generation: 0
    function _writesUnderBase() {
        return !guardsEnabled && contentFile.toString() !== "" && baseUrl.toString().startsWith("https:")
    }
    function _showsPrimer(pageUrl) {
        const page = pageUrl.toString()
        const hash = page.indexOf("#")
        return _writesUnderBase() && (hash < 0 ? page : page.slice(0, hash)) === baseUrl.toString()
    }
    // Jeton et position ne changent qu'à l'ouverture de l'amorce : une navigation dans le même document pendant
    // l'écriture (fragment, history) ne l'interrompt pas.
    function _writePrimer() {
        const generation = _generation
        const token = Math.random().toString(36).slice(2)
        web.runJavaScript("(function(){var r=document.documentElement;if(!r||!r.hasAttribute('data-actcraft-primer'))"
                          + "return false;document.open();window.actcraftPrimer=" + JSON.stringify(token)
                          + ";return true})()",
                          function (opened) {
                              if (generation !== root._generation)
                                  return
                              if (opened) {
                                  root._token = token
                                  root._written = 0
                                  root._writing = true
                                  writer.start()
                              } else if (!root._writing) {
                                  root.pageLoaded()
                              }
                          })
    }
    // Le jeton refuse d'écrire dans une page qui aurait remplacé l'amorce. Une paire de substitution n'est
    // jamais coupée : l'analyseur la laisserait en deux nœuds de texte (tst_ui_webengine).
    function _writeNext() {
        const generation = _generation
        let end = Math.min(html.length, _written + _chunk)
        const high = end < html.length ? html.charCodeAt(end - 1) : 0
        if (high >= 0xD800 && high <= 0xDBFF)
            end -= 1
        const last = end === html.length
        const piece = html.slice(_written, end)
        _written = end
        web.runJavaScript("(function(){if(window.actcraftPrimer!==" + JSON.stringify(_token) + ")return false;"
                          + "document.write(" + JSON.stringify(piece) + ");"
                          + (last ? "delete window.actcraftPrimer;document.close();" : "") + "return true})()",
                          function (written) {
                              if (written && last && generation === root._generation)
                                  root._settle() // un script externe bloquant retarde la fin de l'analyse
                          })
        if (!last)
            writer.start()
    }
    // pageLoaded après la fin de l'analyse du HTML écrit, comme le load d'une page chargée.
    function _settle() {
        const generation = _generation
        web.runJavaScript("document.readyState", function (state) {
            if (generation !== root._generation)
                return
            if (state === "complete") {
                root._writing = false
                root.pageLoaded()
            } else {
                settle.start()
            }
        })
    }

    function load() {
        ++_generation // l'écriture en cours visait l'ancien contenu
        writer.stop()
        settle.stop()
        _writing = false
        if (directUrl.toString() !== "")
            web.url = directUrl
        else if (_writesUnderBase())
            web.loadHtml(_primer, baseUrl) // non vide : loadHtml("", base) chargerait l'adresse de base
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

    Timer {
        id: writer
        interval: 0
        onTriggered: root._writeNext()
    }
    Timer {
        id: settle
        interval: 50
        onTriggered: root._settle()
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
        // Un fichier ou une adresse lâché sur la vue ne la fait pas naviguer gardes allumées : seul le modèle décide.
        settings.navigateOnDropEnabled: !root.guardsEnabled
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
            if (info.status === WebEngineView.LoadSucceededStatus && root._showsPrimer(info.url))
                root._writePrimer()
            else if (info.status === WebEngineView.LoadSucceededStatus)
                root.pageLoaded()
            root._report()
        }
    }
}
