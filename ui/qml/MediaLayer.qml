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
                    // Lecture/pause et arrêt réels (F-552, F-582). Sans lecteur encore, la demande passe par le modèle
                    // (qui extrait le fichier au besoin) et le lecteur créé joue aussitôt.
                    function togglePlay() {
                        if (!playerSlot.item) {
                            if (mediaLayer.items)
                                mediaLayer.items.play(slot.mediaId)
                        } else if (video.playing) {
                            playerSlot.item.pause()
                        } else {
                            playerSlot.item.play()
                        }
                    }
                    function stopPlayback() {
                        if (playerSlot.item)
                            playerSlot.item.stop()
                    }
                    // Commandes (masquées si controlsHidden). La barre est une couverture d'entrée de la vue (comme le
                    // plateau de glisser-déposer) : un clic dessus ne trace rien et ne déclenche aucune action de la page.
                    Rectangle {
                        id: controls
                        objectName: "mediaControls-" + slot.mediaId
                        visible: !slot.controlsHidden
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.bottom: parent.bottom
                        height: Math.min(24, parent.height / 4)
                        color: "#B0202428"
                        function syncCover() {
                            if (!mediaLayer.view)
                                return
                            if (controls.visible)
                                mediaLayer.view.addInputCover(controls)
                            else
                                mediaLayer.view.removeInputCover(controls)
                        }
                        Component.onCompleted: syncCover()
                        onVisibleChanged: syncCover()
                        Component.onDestruction: if (mediaLayer.view) mediaLayer.view.removeInputCover(controls)
                        Item {
                            id: toggleButton
                            objectName: "mediaPlayPause-" + slot.mediaId
                            width: controls.height
                            height: controls.height
                            Text {
                                anchors.centerIn: parent
                                text: video.playing ? "\u275A\u275A" : "\u25B6"
                                color: "#E8EAED"
                                font.pixelSize: Math.max(8, parent.height * 0.5)
                            }
                            MouseArea {
                                anchors.fill: parent
                                onClicked: video.togglePlay()
                            }
                        }
                        Item {
                            id: stopButton
                            objectName: "mediaStop-" + slot.mediaId
                            x: controls.height
                            width: controls.height
                            height: controls.height
                            Rectangle {
                                anchors.centerIn: parent
                                width: Math.max(6, parent.height * 0.4)
                                height: width
                                color: "#E8EAED"
                            }
                            MouseArea {
                                anchors.fill: parent
                                onClicked: video.stopPlayback()
                            }
                        }
                        Rectangle {
                            anchors.left: stopButton.right
                            anchors.leftMargin: 4
                            anchors.verticalCenter: parent.verticalCenter
                            height: 3
                            color: "#E8EAED"
                            width: playerSlot.item && playerSlot.item.duration > 0
                                   ? Math.max(0, controls.width - 2 * controls.height - 8) * playerSlot.item.position / playerSlot.item.duration
                                   : 0
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
                    // Lecture auto : le fichier est écrit hors du fil principal, autoPlay passe à vrai après coup.
                    Connections {
                        target: slot
                        function onAutoPlayChanged() {
                            if (slot.autoPlay)
                                video.armed = true
                        }
                    }
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
