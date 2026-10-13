#pragma once

#include <actcraft/core/inkprobe.h>
#include <actcraft/render/inkbuffer.h>

#include <QQuickItem>

#include <cstdint>

namespace actcraft::render {

// Trait en cours, au-dessus du cache (décision 9). Exposé en QML sous le nom InkLayer (ui/, T12).
class InkItem : public QQuickItem
{
    Q_OBJECT

public:
    explicit InkItem(QQuickItem* parent = nullptr);

    void beginStroke(core::Argb color, double baseWidth);
    // `seq` : numéro de l'échantillon (input/), rendu à la sonde quand le point entre dans la géométrie
    // (DA4).
    void addPoint(const core::StrokePoint& point, std::uint64_t seq);
    void clear();
    // Sonde de latence (DA4), nulle par défaut : coût nul. Elle doit survivre à l'élément ou être retirée
    // avant.
    void setProbe(core::InkProbe* probe) noexcept;
    std::size_t pointCount() const noexcept;
    std::size_t blockCount() const noexcept;

    // Chemin RHI (DA5) : un nœud de géométrie par bloc de `buffer` sous `root`, alloué une fois à sa
    // capacité. Seuls les segments apparus depuis l'appel précédent sont écrits, dans les blocs sales.
    // `root` ne contient que des nœuds créés par cette fonction pour ce tampon ; après InkBuffer::clear(),
    // l'appelant le vide. Public pour les tests : hors écran, Qt prend toujours le rendu logiciel et ce
    // chemin n'y est jamais appelé.
    static void syncRhiBlocks(QSGNode* root, const InkBuffer& buffer);

protected:
    QSGNode* updatePaintNode(QSGNode* old, UpdatePaintNodeData* data) override;

private:
    void updateSoftware(QSGNode* root);

    InkBuffer m_buffer;
    core::InkProbe* m_probe = nullptr;
    std::uint64_t m_lastSeq = 0;
    bool m_seqPending = false;
};

} // namespace actcraft::render
