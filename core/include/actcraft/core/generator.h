#pragma once

#include <actcraft/core/model.h>

#include <cstdint>
#include <random>

namespace actcraft::core {

inline constexpr std::uint32_t kDefaultSeed = 20261013u;
inline constexpr int kShapeCount = 150;
inline constexpr int kImageCount = 30;
inline constexpr int kStrokeCount = 120;

// Tirage portable (décision 3) : std::mt19937, dont la suite est fixée par la norme, et un rejet écrit par
// nous. Les distributions de la bibliothèque standard laissent leur algorithme à l'implémentation : la même
// graine donnerait une autre page sous MSVC et sous libc++. Un seul tirage par expression : l'ordre
// d'évaluation des arguments d'une fonction n'est pas fixé par la norme.
class PortableRng
{
public:
    explicit PortableRng(std::uint32_t seed) : m_engine(seed) {}

    std::uint32_t next();
    // Entier uniforme dans [0, n), n > 0 : x mod n après rejet des x >= 2^32 - (2^32 mod n).
    std::uint32_t below(std::uint32_t n);
    // Entier uniforme dans [lo, hi] (bornes incluses).
    std::int32_t between(std::int32_t lo, std::int32_t hi);

private:
    std::mt19937 m_engine;
};

// Trois objets témoins, de couleurs connues, au premier plan, à des positions connues (décision 3).
// Le banc contrôle leur couleur sur la capture « retour à 100 % » (décision 31).
struct Witnesses
{
    RectF shapeRect;
    Argb shapeColor;
    RectF imageRect;
    Argb imageCenterColor;
    RectF strokeRect;
    PointF strokeProbe;
    Argb strokeColor;
};

const Witnesses& witnesses() noexcept;

// Vrai si `p` tombe dans l'un des rectangles témoins agrandi de `margin` pixels de page.
bool hitsWitness(PointF p, double margin) noexcept;

// 300 objets : 150 formes (30 semi-transparentes), 30 images, 120 traits de 50 à 300 points (N1-5),
// dont les trois témoins, en dernier. Arithmétique entière : même résultat sous MSVC et Clang.
Page generatePage(std::uint32_t seed = kDefaultSeed);

} // namespace actcraft::core
