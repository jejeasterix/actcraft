import QtQuick
import QtMultimedia

// L8-T25 (S25, F-582, V57) : vidéos et GIF animés de la page affichée, en surcouche de pageOverlays, au-dessus du
// cache et jamais dedans : la page fixe ne peint que l'espace réservé (ou la première image du GIF) et rien ici
// ne lui demande de redessin. Placés par view.pageToScene ; aucune surcouche ne prend l'entrée : le stylet écrit
// à travers une vidéo qui joue, et le clic sur un média reste à l'exécuteur des actions (au relâché).
Item {
    id: mediaLayer
    objectName: "mediaLayer"
    anchors.fill: parent
    property var items: null // MediaItems de la fenêtre
    property var view: null  // PageView de la zone
    property int tick: 0     // relu au zoom, au défilement et au changement de page
    visible: items !== null && view !== null

    Connections {
        target: mediaLayer.view
        ignoreUnknownSignals: true
        function onZoomChanged() { mediaLayer.tick++ }
        function onPanChanged() { mediaLayer.tick++ }
        function onContentChanged() { mediaLayer.tick++ }
    }
    onWidthChanged: tick++
    onHeightChanged: tick++

    function frameOf(bounds) {
        mediaLayer.tick // dépendance de liaison
        if (!mediaLayer.view || !bounds)
            return Qt.rect(0, 0, 0, 0)
        const a = mediaLayer.mapFromItem(null, mediaLayer.view.pageToScene(Qt.point(bounds.x, bounds.y)))
        const b = mediaLayer.mapFromItem(null, mediaLayer.view.pageToScene(Qt.point(bounds.x + bounds.width, bounds.y + bounds.height)))
        return Qt.rect(a.x, a.y, b.x - a.x, b.y - a.y)
    }

    Repeater {
        model: mediaLayer.visible ? mediaLayer.items : null
        delegate: Loader {
            id: slot
            required property var mediaId
            required property string kind
            required property string mediaSource // pas « source » : masquerait Loader.source
            required property rect bounds
            required property bool controlsHidden
            required property bool loop
            required property bool aspectLocked
            required property bool clearBackground
            required property bool autoPlay
            readonly property rect frame: mediaLayer.frameOf(bounds)
            x: frame.x
            y: frame.y
            width: frame.width
            height: frame.height
            sourceComponent: kind === "video" ? videoFrame : (kind === "gif" ? gifFrame : null)

            Component {
                id: videoFrame
                Item {
                    id: video
                    objectName: "mediaFrame-" + slot.mediaId
                    // Le lecteur (MediaPlayer et sa sortie audio) n'existe qu'après la première demande de
                    // lecture : une page de vidéos n'en ouvre aucun à l'affichage.
                    property bool armed: false
                    readonly property var mediaPlayer: playerSlot.item
                    readonly property bool playing: playerSlot.item !== null &&
                                                    playerSlot.item.playbackState === MediaPlayer.PlayingState
                    // Fond noir pendant la lecture ; « fond transparent » (clearBackground) le retire.
                    Rectangle {
                        anchors.fill: parent
                        color: "black"
                        visible: video.playing && !slot.clearBackground
                    }
                    VideoOutput {
                        id: output
                        objectName: "mediaVideo-" + slot.mediaId
                        anchors.fill: parent
                        fillMode: slot.aspectLocked ? VideoOutput.PreserveAspectFit : VideoOutput.Stretch
                    }
                    Loader {
                        id: playerSlot
                        active: video.armed
                        sourceComponent: MediaPlayer {
                            source: slot.mediaSource
                            videoOutput: output
                            loops: slot.loop ? MediaPlayer.Infinite : 1
                            audioOutput: AudioOutput {}
                            // Vidéo illisible : repli annoncé vers le système (confirmation), jamais un échec muet.
                            onErrorOccurred: function(error, errorString) {
                                if (error !== MediaPlayer.NoError && mediaLayer.items)
                                    mediaLayer.items.videoFailed(slot.mediaId)
                            }
                        }
                        onLoaded: item.play()
                    }
                    // Commandes (F-582 : masquées si controlsHidden) : état et avancement, sans prise d'entrée.
                    Rectangle {
                        objectName: "mediaControls-" + slot.mediaId
                        visible: !slot.controlsHidden
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.bottom: parent.bottom
                        height: Math.min(24, parent.height / 4)
                        color: "#B0202428"
                        Rectangle {
                            anchors.left: parent.left
                            anchors.verticalCenter: parent.verticalCenter
                            height: 3
                            color: "#E8EAED"
                            width: playerSlot.item && playerSlot.item.duration > 0
                                   ? parent.width * playerSlot.item.position / playerSlot.item.duration : 0
                        }
                    }
                    Connections {
                        target: mediaLayer.items
                        function onVideoPlayRequested(id) {
                            if (id !== slot.mediaId)
                                return
                            if (playerSlot.item)
                                playerSlot.item.play()
                            else
                                video.armed = true // le lecteur créé joue aussitôt (onLoaded)
                        }
                    }
                    Component.onCompleted: if (slot.autoPlay) video.armed = true
                }
            }
            Component {
                id: gifFrame
                AnimatedImage {
                    objectName: "mediaGif-" + slot.mediaId
                    source: slot.mediaSource
                    playing: true
                    cache: false
                    fillMode: Image.Stretch
                }
            }
        }
    }
}
