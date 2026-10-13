#pragma once

// Statistiques du banc : fonctions pures, sans Qt (décisions 24 et 25 du cadrage, seuils N1-1).

#include <cstddef>
#include <cstdint>
#include <vector>

namespace actcraft::bench {

inline constexpr double kOver25Ms = 25.0;
inline constexpr double kFreezeFactor = 2.5; // figement : au plus une image perdue (N1-1)

struct Percentiles
{
    double p50 = 0;
    double p95 = 0;
    double p99 = 0;
    double max = 0;
    std::size_t count = 0;
};

// Rang le plus proche : valeur triée d'indice ceil(p × n / 100) − 1 (médiane basse si n est pair). 0 si vide.
double percentile(std::vector<double> values, double p);
Percentiles summarize(const std::vector<double>& values);

// Séries d'un scénario, horloge core::monotonicNowNs().
struct FrameSeries
{
    std::vector<std::int64_t> swapNs;   // frameSwapped, croissants
    std::vector<std::int64_t> inputNs;  // instants PRÉVUS des entrées postées, croissants (décision 23)
    std::vector<std::int64_t> freezeNs; // figements de trait
    std::int64_t refreshPeriodNs = 16'666'667; // période d'écran MESURÉE : fixe les deux seuils relatifs
};

enum class IntervalClass : std::uint8_t
{
    Idle,
    Active,
    Freeze,
};

struct IntervalStats
{
    std::vector<IntervalClass> classes; // une par intervalle (swapNs.size() − 1)
    std::vector<double> activeMs;       // intervalles actifs hors figements
    std::vector<double> freezeMs;
    Percentiles active;
    double fps = 0;         // 1000 / médiane des intervalles actifs
    double over25Ratio = 0; // part des intervalles actifs > 25 ms
    double freezeMaxMs = 0;
    std::size_t freezeOverLimit = 0; // figements ≥ kFreezeFactor périodes d'écran
};

// Intervalle (swapNs[i − 1], swapNs[i]] :
// - Freeze s'il contient un figement ;
// - Active s'il contient une entrée et que la première arrive au plus 2 périodes après l'image de départ ;
// - Idle sinon (pause du stylo levé, attente du redessin : sa durée n'est pas celle d'une image).
IntervalStats classifyIntervals(const FrameSeries& series);

// Une synchronisation de l'encre et le frameSwapped de l'image qui la suit.
struct SyncRecord
{
    std::uint64_t lastSeq = 0;
    std::int64_t syncNs = 0;
    std::int64_t swapNs = 0;
};

struct LatencyStats
{
    std::vector<std::uint64_t> seq;
    std::vector<double> totalMs;
    std::vector<double> plannedToSyncMs;
    std::vector<double> syncToSwapMs;
    Percentiles total;
    Percentiles plannedToSync;
    Percentiles syncToSwap;
    std::size_t unmatched = 0; // échantillons mesurés jamais vus dans une image
};

// plannedNs[seq] : instant prévu de l'échantillon `seq`, ou −1 s'il n'est pas mesuré (relâché, molette).
// records : dans l'ordre des images, lastSeq croissant.
LatencyStats computeLatency(const std::vector<std::int64_t>& plannedNs,
                            const std::vector<SyncRecord>& records);

} // namespace actcraft::bench
