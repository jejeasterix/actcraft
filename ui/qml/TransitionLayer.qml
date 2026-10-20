import QtQuick
import ActCraft.Ui

// Effets de changement de page (F-640, L7-T7). page.next et page.previous sont enveloppés ici : sans effet à
// jouer, la page change aussitôt (corps d'origine de Main.qml) ; avec un effet, (1) l'ancienne page est figée
// dans des ShaderEffectSource (live: false) dont la source est la page entière (pageContent : page fixe, cache,
// couche d'attente, encre), (2) la page change à leur scheduledUpdateCompleted, (3) les morceaux bougent
// au-dessus de la nouvelle page par des Animator (fil de rendu ; opacité, translation, échelle, découpe), (4) ils
// sont retirés. Jamais grabToImage, jamais de redessin du cache : la vue redessine la nouvelle page, seule.
// Racine posée dans presentation.layer : la fenêtre entière ; aucune entrée prise.
Item {
    id: root
    anchors.fill: parent
    property Commands commands: null
    property DocumentController documentController: null
    property Tools tools: null
    property Item pageArea: null
    property QtObject controller: transitionController
    property QtObject curtain: null
    property QtObject spotlight: null
    // Instantanés pris depuis le chargement (crochet de test : aucun quand il n'y a pas d'effet à jouer).
    property int snapshotCount: 0
    // Morceaux de l'instantané en cours ; leur génération écarte le signal d'un instantané déjà retiré.
    property var pieces: []
    property int generation: 0
    property int frozenCount: 0
    property int doneCount: 0

    TransitionController {
        id: transitionController
        objectName: "transition"
        documentController: root.documentController
        blocked: (root.curtain ? root.curtain.active : false) || (root.spotlight ? root.spotlight.active : false)
        onCaptureRequested: effect => root.capture(effect)
        onPlayRequested: (effect, durationMs) => root.play(durationMs)
        onStopRequested: root.clear()
    }

    // Zone de la page, rognée : l'ancienne page bouge à l'intérieur, la nouvelle est dessous.
    Item {
        id: frame
        objectName: "transitionSnapshot"
        clip: true
    }

    Component {
        id: pieceComponent
        ShaderEffectSource {
            id: piece
            property int generation: 0
            property bool ready: false
            property real toX: 0
            property real toY: 0
            property real toScale: 1
            property real toOpacity: 1
            property real fraction: 1 // part de la durée
            property int durationMs: 1
            live: false
            hideSource: false
            onScheduledUpdateCompleted: {
                if (!piece.ready) {
                    piece.ready = true
                    root.pieceFrozen(piece.generation)
                }
            }

            ParallelAnimation {
                id: motion
                XAnimator {
                    target: piece; from: piece.x; to: piece.x + piece.toX; duration: piece.durationMs
                    easing.type: Easing.InOutQuad
                }
                YAnimator {
                    target: piece; from: piece.y; to: piece.y + piece.toY; duration: piece.durationMs
                    easing.type: Easing.InOutQuad
                }
                ScaleAnimator {
                    target: piece; from: 1; to: piece.toScale; duration: piece.durationMs
                    easing.type: Easing.InOutQuad
                }
                OpacityAnimator {
                    target: piece; from: 1; to: piece.toOpacity; duration: piece.durationMs
                    easing.type: Easing.InOutQuad
                }
                onFinished: root.pieceDone(piece.generation)
            }

            function play(totalMs) {
                piece.durationMs = Math.max(1, Math.round(totalMs * piece.fraction))
                motion.start()
            }
        }
    }

    // (1) Figer l'ancienne page : un morceau par pièce du mouvement, à la place exacte de la page affichée.
    function capture(effect) {
        root.clear()
        const area = root.pageArea
        const view = area ? area.view : null
        const page = view ? view.content : null
        const plan = transitionController.motion(effect)
        if (!page || plan.length === 0) {
            transitionController.frozen()
            return
        }
        const zone = root.mapFromItem(area, 0, 0, area.width, area.height)
        frame.x = zone.x
        frame.y = zone.y
        frame.width = zone.width
        frame.height = zone.height
        const top = frame.mapFromItem(page, 0, 0)
        const bottom = frame.mapFromItem(page, page.width, page.height)
        const w = bottom.x - top.x
        const h = bottom.y - top.y
        // Même résolution que le cache (zoom et densité de l'écran compris).
        const texture = view.cacheTextureSize
        const k = texture.width > 0 && page.width > 0 ? texture.width / page.width : 0
        root.generation += 1
        const made = []
        for (let i = 0; i < plan.length; ++i) {
            const p = plan[i]
            const source = Qt.rect(p.x * page.width, p.y * page.height, p.width * page.width,
                                   p.height * page.height)
            made.push(pieceComponent.createObject(frame, {
                generation: root.generation,
                sourceItem: page,
                sourceRect: source,
                textureSize: k > 0 ? Qt.size(Math.ceil(source.width * k), Math.ceil(source.height * k))
                                   : Qt.size(0, 0),
                x: top.x + p.x * w,
                y: top.y + p.y * h,
                width: p.width * w,
                height: p.height * h,
                transformOrigin: p.origin,
                toX: p.dx * w,
                toY: p.dy * h,
                toScale: p.scale,
                toOpacity: p.opacity,
                fraction: p.span
            }))
        }
        root.pieces = made
        root.frozenCount = 0
        root.snapshotCount += 1
        for (const piece of made) {
            piece.scheduleUpdate()
        }
    }

    // (2) Tous les morceaux sont figés : le contrôleur change de page.
    function pieceFrozen(generation) {
        if (generation !== root.generation) {
            return
        }
        root.frozenCount += 1
        if (root.frozenCount === root.pieces.length) {
            transitionController.frozen()
        }
    }

    // (3) L'effet, sur la nouvelle page déjà affichée.
    function play(durationMs) {
        root.doneCount = 0
        if (root.pieces.length === 0) {
            transitionController.finished()
            return
        }
        for (const piece of root.pieces) {
            piece.play(durationMs)
        }
    }

    function pieceDone(generation) {
        if (generation !== root.generation) {
            return
        }
        root.doneCount += 1
        if (root.doneCount === root.pieces.length) {
            transitionController.finished()
        }
    }

    // (4) Retirer l'instantané (fin de l'effet, ou effet interrompu par un nouvel appui).
    function clear() {
        root.generation += 1
        const old = root.pieces
        root.pieces = []
        root.frozenCount = 0
        root.doneCount = 0
        for (const piece of old) {
            piece.visible = false
            piece.destroy()
        }
    }

    function registerCommands() {
        if (!root.commands) {
            return
        }
        // Enveloppe : posée après les gestionnaires de Main.qml (Qt.callLater, ordre des onCompleted non garanti).
        root.commands.setHandler("page.next", function () { transitionController.navigate(0) })
        root.commands.setHandler("page.previous", function () { transitionController.navigate(1) })
        root.commands.setHandler("view.page-effect", function (id) {
            transitionController.globalEffect = String(id)
        })
        root.commands.setHandler("view.page-effect-duration", function (tenths) {
            transitionController.durationTenths = Math.round(Number(tenths))
        })
        root.commands.setHandler("page.effect", function (id) { transitionController.setPageEffect(String(id)) })
    }

    Component.onCompleted: Qt.callLater(root.registerCommands)
}
