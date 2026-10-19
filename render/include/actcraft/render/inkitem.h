#pragma once

#include <actcraft/core/inkprobe.h>
#include <actcraft/render/inkbuffer.h>
#include <actcraft/render/strokegeometry.h>

#include <QQuickItem>

#include <cstdint>

namespace actcraft::render {

// Trait en cours, au-dessus du cache (décision 9). Exposé en QML sous le nom InkLayer (ui/, T12).
class InkItem : public QQuickItem
{
    Q_OBJECT

public:
    explicit InkItem(QQuickItem* parent = nullptr);

    // Couleur translucide (alpha < 255, marqueur) : l'encre est dessinée opaque, dans une couche de l'élément
    // (propriété QML layer.enabled), et l'élément prend l'opacité de la couleur le temps du trait. La couche
    // est composée d'un seul tenant : le trait ne fonce pas là où ses segments se recouvrent ou se recoupent
    // (D3) ; une couleur à alpha sur les segments, ou une opacité sans couche, foncerait. clear() rend
    // l'élément opaque, sans couche. Forme selon `cap` (Stroke::cap) : ronde (D2, tout bout sauf le carré du
    // prototype) ou segments carrés du prototype.
    void beginStroke(core::Argb color, double baseWidth, core::PenCap cap = core::PenCap::Round);
    bool isTranslucent() const noexcept;
    // Couleur des segments (opaque pour un trait translucide).
    core::Argb drawnColor() const noexcept;
    // `seq` : numéro de l'échantillon (input/), rendu à la sonde quand le point entre dans la géométrie
    // (DA4).
    void addPoint(const core::StrokePoint& point, std::uint64_t seq);
    // Garde le numéro en attente : il est signalé à la synchronisation suivante (voir inkitem.cpp).
    void clear();
    // Sonde de latence (DA4), nulle par défaut : coût nul. Elle doit survivre à l'élément ou être retirée
    // avant.
    void setProbe(core::InkProbe* probe) noexcept;
    std::size_t pointCount() const noexcept;
    std::size_t blockCount() const noexcept;

    // Chemin RHI (DA5) : un nœud de géométrie par bloc de `buffer` sous `root`, alloué une fois à sa
    // capacité, avec un matériau propre au bloc (un lot par bloc). Seuls les segments apparus depuis l'appel
    // précédent sont écrits, dans les blocs sales ; un nœud créé (racine neuve) est écrit en entier. Vide
    // `root` si le tampon a été vidé depuis le dernier markClean(). `root` ne contient que des nœuds créés
    // par cette fonction pour ce tampon. Public pour les tests : hors écran, Qt prend toujours le rendu
    // logiciel et ce chemin n'y est jamais appelé.
    // Forme ronde (D2, L3-T11), largeur baseWidth : un bloc = disque de départ (kRoundInkCapVertices sommets,
    // tracé dans le bloc 0 seulement), un segment appendRoundInkSegment par paire de points
    // (kRoundInkVerticesPerSegment sommets), disque d'arrivée (sur le dernier point du trait, dans le dernier
    // bloc seulement). Le dernier segment du trait est provisoire (droit) : il est réécrit à l'arrivée du
    // point suivant, même quand ce point ouvre un bloc (le bloc plein reçoit alors son dernier segment et
    // perd son disque d'arrivée, une fois) ; les segments d'avant ne bougent plus. Prototype (bout carré) :
    // kVerticesPerSegment sommets par segment, appendStrokeTriangles.
    static constexpr std::size_t kRoundBlockVertices =
        2 * kRoundInkCapVertices + (InkBuffer::kBlockPoints - 1) * kRoundInkVerticesPerSegment;
    static void syncRhiBlocks(QSGNode* root, const InkBuffer& buffer, core::PenCap cap = core::PenCap::Round);

    // Segments d'encre peints par le chemin logiciel depuis le lancement, tous éléments confondus : borne le
    // travail d'une image (tests, banc).
    static std::uint64_t softwareSegmentsPainted() noexcept;

protected:
    QSGNode* updatePaintNode(QSGNode* old, UpdatePaintNodeData* data) override;

private:
    void updateSoftware(QSGNode* root);
    void setTranslucent(bool on);

    InkBuffer m_buffer;
    bool m_translucent = false;
    core::PenCap m_cap = core::PenCap::Round;
    core::InkProbe* m_probe = nullptr;
    std::uint64_t m_lastSeq = 0;
    bool m_seqPending = false;
};

} // namespace actcraft::render
