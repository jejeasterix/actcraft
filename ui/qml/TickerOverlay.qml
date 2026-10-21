import QtQuick
import ActCraft.Ui

// Message défilant (F-291, L7-T18) : une bande pleine largeur en haut, au centre ou en bas de la fenêtre
// (presentation.layer), où le texte passe de droite à gauche, une fois ou en boucle. Le texte bouge par un
// XAnimator, sur le fil de rendu : aucune minuterie ni liaison ne le déplace sur le fil de l'interface, qui reste
// au stylet. L'abscisse du texte côté QML n'est donc pas sa place à l'écran pendant le passage : rien ne la lit,
// et la couverture est la bande entière, immobile. Le cache n'est jamais redessiné. Texte toujours en texte brut
// (saisi par l'utilisateur ou lu d'un fichier). Entrée : la bande est une couverture de la vue
// (addInputCover de la racine, limitée à la bande par containmentMask) tant que le message défile ; elle ne prend
// que le clic droit, qui arrête le message. La fenêtre d'édition (TickerDialog) s'ouvre avec la commande
// tool.ticker. Vitesse, boucle et largeur du texte sont suivies en plein passage : la suite part de la place
// actuelle (retime).
Item {
    id: root
    anchors.fill: parent
    containmentMask: band
    property Commands commands: null
    property DocumentController documentController: null
    property Tools tools: null
    property Item pageArea: null
    property QtObject controller: tickerController
    // Vitesse lue par une propriété : un changement en cours de passage repart de la place actuelle (retime).
    readonly property real pixelsPerSecond: tickerController.pixelsPerSecond
    onPixelsPerSecondChanged: root.retime()
    // Boucle et largeur du texte (la fin du passage) lues de même : couper la boucle finit le passage en cours,
    // la reprendre le prolonge, un texte plus long ou plus court déplace la sortie.
    readonly property bool looping: tickerController.loop
    onLoopingChanged: root.retime()
    readonly property real textWidth: message.width
    onTextWidthChanged: root.retime()

    TickerController {
        id: tickerController
        objectName: "ticker"
        onRunningChanged: {
            if (running) {
                root.begin()
            } else {
                root.end()
            }
            root.syncCover()
        }
        onPausedChanged: {
            if (paused) {
                root.hold()
            } else if (running) {
                root.resume()
            }
        }
    }

    Rectangle {
        id: band
        objectName: "tickerBand"
        width: root.width
        height: Math.ceil(message.height + 2 * Theme.espace2)
        y: tickerController.position === TickerController.Bottom ? root.height - height
           : tickerController.position === TickerController.Center ? Math.round((root.height - height) / 2) : 0
        visible: tickerController.running
        color: tickerController.screenBackground ? "transparent" : tickerController.backgroundColor
        clip: true

        // Texte et ombre : déplacés ensemble par les animateurs, jamais par une liaison.
        Item {
            id: message
            objectName: "tickerMessage"
            y: Theme.espace2
            width: face.implicitWidth + tickerController.shadowOffset
            height: face.implicitHeight + tickerController.shadowOffset

            Text {
                objectName: "tickerShadow"
                x: tickerController.shadowOffset
                y: tickerController.shadowOffset
                visible: tickerController.shadowOffset > 0
                textFormat: Text.PlainText
                text: face.text
                font: face.font
                color: tickerController.shadowColor
            }
            Text {
                id: face
                objectName: "tickerText"
                textFormat: Text.PlainText
                text: tickerController.text
                font.family: tickerController.fontFamily.length > 0 ? tickerController.fontFamily
                                                                     : Qt.application.font.family
                font.pointSize: tickerController.fontPointSize
                color: tickerController.textColor
            }
        }

        // Passage complet, du bord droit de la bande jusqu'à la sortie du texte à gauche ; en boucle, sans fin.
        XAnimator {
            id: motion
            objectName: "tickerMotion"
            target: message
            from: band.width
            to: -message.width
            // Posée à chaque départ (setDuration), jamais liée : une durée changée en plein passage le finirait.
            duration: 1
            loops: tickerController.loop ? Animation.Infinite : 1
            onFinished: {
                if (root.finishedBy(motion)) {
                    tickerController.finished()
                }
            }
        }

        // Reprise après une pause : la fin du passage interrompu, à la même vitesse, puis la boucle.
        XAnimator {
            id: resumeMotion
            objectName: "tickerResume"
            target: message
            to: -message.width
            onFinished: {
                if (root.finishedBy(resumeMotion)) {
                    root.afterResume()
                }
            }
        }

        MouseArea {
            objectName: "tickerStopArea"
            anchors.fill: parent
            acceptedButtons: Qt.RightButton
            onClicked: tickerController.stop()
        }
    }

    // Départ de l'animateur en cours : sa place se calcule à la demande (pause, vitesse changée), jamais à
    // chaque image. L'abscisse qu'écrit un animateur arrêté ne suffit pas : elle vaut 0 s'il n'a pas encore
    // avancé d'une image.
    property double passStart: 0
    property real passFrom: 0
    property real passSpeed: 0
    property real passWidth: 0
    property bool passLoop: false
    property bool partialPass: false
    property QtObject runningMotion: null // animateur du passage en cours ; nul à l'arrêt et en pause

    function track(animator, from, partial) {
        root.runningMotion = animator
        root.passStart = Date.now()
        root.passFrom = from
        root.passSpeed = tickerController.pixelsPerSecond
        root.passWidth = message.width
        root.passLoop = tickerController.loop
        root.partialPass = partial
    }

    // Un animateur arrêté (pause, vitesse changée, arrêt) peut signaler sa fin : seule compte celle du passage
    // en cours, arrivé au bout de sa durée.
    function finishedBy(animator) {
        if (animator !== root.runningMotion || Date.now() - root.passStart < animator.duration - 250) {
            return false
        }
        root.runningMotion = null
        return true
    }

    function placeNow() {
        const elapsed = Date.now() - root.passStart
        if (root.partialPass) {
            return Math.max(-message.width, root.passFrom - root.passSpeed * elapsed / 1000)
        }
        return tickerController.positionAt(band.width, root.passWidth, root.passSpeed, root.passLoop, elapsed)
    }

    function begin() {
        root.runningMotion = null
        resumeMotion.stop()
        motion.stop()
        message.x = band.width
        root.setDuration()
        root.track(motion, band.width, false)
        motion.start()
    }

    // Durée d'un passage complet à la vitesse du moment (features::TickerRun).
    function setDuration() {
        motion.duration = tickerController.durationMs(band.width, message.width)
    }

    function end() {
        root.runningMotion = null
        resumeMotion.stop()
        motion.stop()
        message.x = band.width
    }

    // Pause : le texte reste à sa place, écrite dans message.x, d'où la reprise repart.
    function hold() {
        const x = root.placeNow()
        root.runningMotion = null
        resumeMotion.stop()
        motion.stop()
        message.x = x
    }

    function resume() {
        const left = message.x + message.width
        if (left <= 0) {
            root.afterResume()
            return
        }
        resumeMotion.from = message.x
        resumeMotion.duration = Math.max(1, Math.round(left / tickerController.pixelsPerSecond * 1000))
        root.track(resumeMotion, message.x, true)
        resumeMotion.start()
    }

    // Vitesse changée pendant le défilement : la suite du passage part de la place actuelle, à la nouvelle
    // vitesse (en pause, la reprise la prendra).
    function retime() {
        if (!tickerController.running || tickerController.paused) {
            return
        }
        root.hold()
        root.resume()
    }

    function afterResume() {
        if (tickerController.loop) {
            root.setDuration()
            root.track(motion, band.width, false)
            motion.start()
        } else {
            tickerController.finished()
        }
    }

    TickerDialog {
        id: tickerDialog
        objectName: "tickerDialog"
        parent: root.parent
        z: root.z + 1
        controller: tickerController
        pageArea: root.pageArea
    }

    function syncCover() {
        const view = root.pageArea ? root.pageArea.view : null
        if (!view) {
            return
        }
        if (tickerController.running) {
            view.addInputCover(root)
        } else {
            view.removeInputCover(root)
        }
    }

    function registerCommands() {
        if (!root.commands) {
            return
        }
        // Le message défile aussitôt avec les réglages courants, et la fenêtre d'édition s'ouvre.
        root.commands.setHandler("tool.ticker", function () {
            tickerController.start()
            tickerDialog.open()
        })
    }

    onPageAreaChanged: syncCover()
    onCommandsChanged: registerCommands()

    Component.onCompleted: {
        registerCommands()
        syncCover()
    }
}
