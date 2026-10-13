#pragma once

#include <actcraft/core/model.h>

#include <cstddef>
#include <span>
#include <vector>

namespace actcraft::render {

// Trait en cours rangé en blocs de taille fixe (DA5). Le premier point du bloc b > 0 est le dernier du bloc
// b − 1 (jonction partagée) : les segments des blocs mis bout à bout sont ceux du trait, sans trou ni
// doublon. Commun aux chemins RHI et logiciel.
class InkBuffer
{
public:
    static constexpr std::size_t kBlockPoints = 256; // N2-3

    void begin(core::Argb color, double baseWidth);
    void add(const core::StrokePoint& point);
    void clear() noexcept;

    bool empty() const noexcept;
    // Points ajoutés (les jonctions ne comptent qu'une fois).
    std::size_t pointCount() const noexcept;
    std::size_t blockCount() const noexcept;
    std::span<const core::StrokePoint> block(std::size_t index) const;
    // Rectangle du bloc, agrandi de la demi-largeur maximale de ses points.
    core::RectF blockBounds(std::size_t index) const;
    // Plus petit indice de bloc modifié depuis markClean() ; blockCount() si aucun. Un ajout ne modifie que
    // le dernier bloc (ou celui qu'il ouvre) : les blocs pleins ne redeviennent jamais sales.
    std::size_t firstDirtyBlock() const noexcept;
    bool clearedSinceClean() const noexcept;
    void markClean() noexcept;
    core::Argb color() const noexcept;
    double baseWidth() const noexcept;

private:
    // Boîte des points d'un bloc et demi-largeur maximale : un segment prend la largeur de son premier point
    // et déborde de son second point d'autant (bout carré).
    struct Extent
    {
        double minX;
        double minY;
        double maxX;
        double maxY;
        double maxHalf;
    };

    std::vector<std::vector<core::StrokePoint>> m_blocks;
    std::vector<Extent> m_extents;
    std::size_t m_points = 0;
    std::size_t m_firstDirty = 0;
    bool m_cleared = false;
    core::Argb m_color = 0xFF000000u;
    double m_baseWidth = 3.0;
};

} // namespace actcraft::render
