import QtQuick

// Même contrat que WebFrameEngineView.qml et WebFrameSystemView.qml ; membres privés préfixés de « _ ».
Item {
    id: root
    objectName: "webFrameView"
    property string html: ""
    property url baseUrl: ""
    property url directUrl: ""
    property url contentFile: ""
    property int serial: 0
    property bool guardsEnabled: false
    readonly property bool overlaysScene: _overlays
    readonly property bool viewFocused: _focused

    signal viewReported(url pageUrl, bool canBack, bool canForward, string pageTitle, bool pageLoading)
    signal navigationTaken(url pageUrl)
    signal pageLoaded()

    property int _loads: 0
    property var _calls: []
    property bool _overlays: false
    property bool _focused: false

    function load() { _loads += 1 }
    function stop() { _calls.push("stop") }
    function goBack() { _calls.push("goBack") }
    function goForward() { _calls.push("goForward") }
    function reload() { _calls.push("reload") }
    function capture(callback) { callback(null) }
    function copySelection() { _calls.push("copySelection") }
}
