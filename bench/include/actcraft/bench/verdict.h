#pragma once

#include <actcraft/bench/result.h>
#include <actcraft/bench/scenarios.h>

#include <QStringList>

#include <cstddef>
#include <cstdint>
#include <utility>
#include <vector>

namespace actcraft::bench {

// Seuils N1-1 et N1-7. La limite des figements et des reprises (kFreezeFactor périodes d'écran mesurées) est
// appliquée par classifyIntervals ; le verdict exige freezeOverLimit == 0 et resumeOverLimit == 0.
struct Budget
{
    double minFps = 58.0;
    double maxP95Ms = 18.5;
    double maxOver25Ratio = 0.01;
    double maxLatencyP50Ms = 20.0; // strict
    std::uint64_t maxMemoryBytes = 1ull << 30;
};

// Remplit criteria, withinBudget, informative, valid et invalidReasons (sans doublon ; les raisons déjà
// présentes, comme « error: … », sont gardées). Une période d'écran inconnue rend la mesure invalide
// (« invalid-refresh-period ») : ses statistiques nulles ne sont pas jugées.
void applyVerdict(BenchResult& result, const Budget& budget = {});

enum class ExitCode : int
{
    WithinBudget = 0,
    OverBudget = 1,
    Invalid = 2,
};

ExitCode exitCodeFor(const BenchResult& result) noexcept;
// « BUDGET TENU » | « BUDGET DEPASSE » | « MESURE INVALIDE »
QString verdictLine(const BenchResult& result);
// Résumé console ; la dernière ligne est verdictLine().
QStringList summaryLines(const BenchResult& result, const QString& path);

// --- Validité de la mesure : contrôles du déroulé (runner.cpp) ---

inline constexpr std::size_t kMinRefreshIntervals = 10;

// Période d'écran EFFECTIVE : médiane des intervalles entre images consécutives d'un rendu continu (chauffe).
// QScreen::refreshRate() n'est qu'une valeur nominale. 0 avec moins de kMinRefreshIntervals intervalles : pas
// de valeur par défaut plausible (stats.h), la mesure sera invalide.
std::int64_t measureRefreshPeriodNs(const std::vector<std::int64_t>& swapNs);

// Échantillons reçus par la vue : (numéro de séquence, instant de réception).
using ReceivedSamples = std::vector<std::pair<std::uint64_t, std::int64_t>>;

// La latence relie l'échantillon n° k à l'instant prévu de l'événement d'indice k du plan. Vrai si le k-ième
// échantillon reçu porte l'indice du k-ième événement de tablette du plan, et si tous sont reçus (rien pour
// un plan sans tablette). Faux si la numérotation n'a pas été remise à zéro au début du plan, si un événement
// est perdu, ou si le plan mêle molette et tablette (la chaîne d'entrée ne numérote que la tablette).
bool sequenceMatchesPlan(const std::vector<PlannedInput>& inputs, const ReceivedSamples& received);

// Traits du plan (appui … relâché, numéros = indices du plan) dont le relâché n'est couvert par aucune
// synchronisation avant l'appui suivant ALORS QU'une image a été synchronisée entre leurs réceptions (deux
// frameSwapped dans l'intervalle : avec la boucle à fil séparé, le premier peut finir une image synchronisée
// avant le relâché). Sans cette synchronisation, la fin du trait serait rattachée à la première image du
// trait suivant. Relâché et appui suivant reçus d'un bloc (fil principal bloqué) : la première image qui les
// contient est bien la même, ce n'est pas un défaut de mesure mais une lenteur, que le verdict juge.
std::size_t releasesNotSynced(const std::vector<PlannedInput>& inputs, const ReceivedSamples& received,
                              const std::vector<std::int64_t>& swapNs,
                              const std::vector<SyncRecord>& records);

} // namespace actcraft::bench
