#pragma once

#include <QRandomGenerator>

#include <vector>

namespace actcraft::features {

// Dés (F-285) : 1 à 5 dés à six faces, lancer reproductible pour une graine donnée.
class Dice
{
public:
    static constexpr int kMinCount = 1;
    static constexpr int kMaxCount = 5;
    // E5, tranché par L7-T0 (docs/verifications/2026-10-18-lot7-principes.md) : 1 dé au premier lancement ;
    // ensuite le dernier nombre choisi, mémorisé dans les réglages par le contrôleur (phase 2). Vitesse par
    // défaut : 7 s.
    static constexpr int kDefaultCount = 1;
    static constexpr int kDefaultSeconds = 7;

    explicit Dice(quint32 seed);
    void setCount(int count) noexcept; // borné à [kMinCount, kMaxCount]
    int count() const noexcept;
    std::vector<int> roll(); // une face de 1 à 6 par dé
    const std::vector<int>& faces() const noexcept;
    int total() const noexcept;                   // somme du dernier lancer
    static int animationMs(int seconds) noexcept; // vitesse : 3 à 11 s

private:
    QRandomGenerator m_generator;
    int m_count = kDefaultCount;
    std::vector<int> m_faces;
};

} // namespace actcraft::features
