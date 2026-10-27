import QtQuick
import ActCraft.Ui

// Séparations des cadres d'une page (D11 ; observation de l'original L4-O2) : fines, en pointillé (un pixel d'écran
// sur trois), aux limites des cadres, en édition comme en plein écran. Dans `pageOverlays`, hors de la scène que le
// cache photographie : les montrer ou les changer ne redessine pas le cache. Elles n'attirent rien (L4-O2) et ne
// prennent pas l'entrée : aucun gestionnaire de souris ni de stylet ici.
Item {
    id: overlay
    property string zones: "" // « <cadres en largeur>x<cadres en hauteur> » (appPageSettings.zones)
    property real zoom: 1     // view.zoom : un trait d'un pixel d'écran à tout zoom
    readonly property int across: overlay.zonePart(0)
    readonly property int down: overlay.zonePart(1)
    // n cadres : n − 1 séparations ; lu par le scénario page-allongee-zones.
    readonly property int separatorCount: Math.max(0, across - 1) + Math.max(0, down - 1)
    visible: separatorCount > 0

    function zonePart(i) {
        const parts = overlay.zones.split("x")
        return parts.length === 2 ? Math.max(0, parseInt(parts[i], 10) || 0) : 0
    }

    // Motif du pointillé : une image tuilée de 1 × 3 pixels (verticale) ou 3 × 1 (horizontale), un pixel de la
    // couleur du thème puis deux transparents. Une image tuilée se dessine aussi en rendu logiciel, où Canvas ne
    // dessine rien (constaté, tst_captures_page_allongee_zones) ; elle ne se repeint jamais au zoom : seule sa
    // longueur change. PNG sans compression construit ici, pour que la couleur reste celle du jeton du thème.
    function crc32(bytes) {
        let crc = 0xFFFFFFFF
        for (let i = 0; i < bytes.length; ++i) {
            crc ^= bytes[i]
            for (let k = 0; k < 8; ++k)
                crc = (crc >>> 1) ^ (0xEDB88320 & -(crc & 1))
        }
        return (crc ^ 0xFFFFFFFF) >>> 0
    }
    function be32(n) { return [(n >>> 24) & 255, (n >>> 16) & 255, (n >>> 8) & 255, n & 255] }
    function chunk(type, data) {
        const body = [type.charCodeAt(0), type.charCodeAt(1), type.charCodeAt(2), type.charCodeAt(3)].concat(data)
        return be32(data.length).concat(body, be32(overlay.crc32(body)))
    }
    function base64(bytes) {
        const table = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"
        let out = ""
        for (let i = 0; i < bytes.length; i += 3) {
            const n = (bytes[i] << 16) | ((bytes[i + 1] || 0) << 8) | (bytes[i + 2] || 0)
            out += table[(n >> 18) & 63] + table[(n >> 12) & 63]
            out += i + 1 < bytes.length ? table[(n >> 6) & 63] : "="
            out += i + 2 < bytes.length ? table[n & 63] : "="
        }
        return out
    }
    function dashImage(color, vertical) {
        const w = vertical ? 1 : 3
        const h = vertical ? 3 : 1
        const raw = []
        for (let y = 0; y < h; ++y) {
            raw.push(0) // filtre : aucun
            for (let x = 0; x < w; ++x) {
                if (x + y === 0)
                    raw.push(Math.round(color.r * 255), Math.round(color.g * 255), Math.round(color.b * 255), 255)
                else
                    raw.push(0, 0, 0, 0)
            }
        }
        let a = 1, b = 0
        for (let i = 0; i < raw.length; ++i) {
            a = (a + raw[i]) % 65521
            b = (b + a) % 65521
        }
        const n = raw.length
        const zlib = [0x78, 0x01, 0x01, n & 255, n >> 8, (~n) & 255, ((~n) >> 8) & 255].concat(raw, be32(((b << 16) | a) >>> 0))
        const png = [137, 80, 78, 71, 13, 10, 26, 10].concat(
            overlay.chunk("IHDR", be32(w).concat(be32(h), [8, 6, 0, 0, 0])),
            overlay.chunk("IDAT", zlib), overlay.chunk("IEND", []))
        return "data:image/png;base64," + overlay.base64(png)
    }
    readonly property string verticalDashes: dashImage(Theme.separationCadre, true)
    readonly property string horizontalDashes: dashImage(Theme.separationCadre, false)

    // Une séparation : une image tuilée d'un pixel d'écran d'épaisseur, à l'échelle de l'écran (scale 1 / zoom).
    component Separation: Image {
        property bool vertical: true
        property real at: 0 // position en coordonnées de page
        x: vertical ? at : 0
        y: vertical ? 0 : at
        width: vertical ? 1 : Math.max(1, Math.ceil(overlay.width * overlay.zoom))
        height: vertical ? Math.max(1, Math.ceil(overlay.height * overlay.zoom)) : 1
        scale: 1 / Math.max(0.01, overlay.zoom)
        transformOrigin: Item.TopLeft
        source: vertical ? overlay.verticalDashes : overlay.horizontalDashes
        fillMode: Image.Tile
        smooth: false // chaque point garde sa couleur, même à une position fractionnaire
        mipmap: false
    }

    // Le modèle ne dépend que du nombre de cadres, jamais du zoom ni du défilement.
    Repeater {
        model: Math.max(0, overlay.across - 1)
        delegate: Separation {
            required property int index
            vertical: true
            at: overlay.width * (index + 1) / overlay.across
        }
    }
    Repeater {
        model: Math.max(0, overlay.down - 1)
        delegate: Separation {
            required property int index
            vertical: false
            at: overlay.height * (index + 1) / overlay.down
        }
    }
}
