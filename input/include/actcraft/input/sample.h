#pragma once

#include <cstdint>

namespace actcraft::input {

enum class DeviceKind : std::uint8_t
{
    Stylus,
    Mouse,
    Other,
    Touch, // un doigt (décision 25 du cadrage livraison-complete) ; ajouté en dernier : valeurs inchangées
};

enum class Phase : std::uint8_t
{
    Press,
    Move,
    Release,
};

// Échantillon d'entrée, en coordonnées de page (décision 12).
struct Sample
{
    double x = 0;
    double y = 0;
    float pressure = 0.5f;
    std::int64_t timestampNs = 0; // core::monotonicNowNs() à la réception
    DeviceKind device = DeviceKind::Other;
    Phase phase = Phase::Move;
    std::uint64_t seq = 0; // attribué par Pipeline, avant le filtre
    // Socle des lots 3 et 7 (DA-A1, L0-S7), à la fin : valeurs existantes inchangées.
    std::uint64_t pointer = 0; // stylet et souris : 0 ; doigt : identifiant du point + 1
    std::uint8_t user = 0;     // utilisateur du contact (DA-A1), rempli par le routeur
};

class SampleSink
{
public:
    virtual ~SampleSink() = default;
    virtual void onSample(const Sample& sample) = 0;
};

} // namespace actcraft::input
