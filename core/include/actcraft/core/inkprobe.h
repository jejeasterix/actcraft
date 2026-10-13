#pragma once

#include <cstdint>

namespace actcraft::core {

// Sonde de latence d'encre (DA4) : implémentée par bench/, appelée par input/ et render/.
// Sans sonde (pointeur nul), coût nul. L'implémentation protège ses données : deux fils l'appellent.
class InkProbe
{
public:
    virtual ~InkProbe() = default;
    // Fil principal : l'échantillon numéro `seq` vient d'entrer dans la chaîne d'entrée.
    virtual void sampleReceived(std::uint64_t seq, std::int64_t nowNs) = 0;
    // Fil de rendu, pendant la synchronisation : les échantillons jusqu'à `lastSeq` sont dans la géométrie
    // de l'image en préparation.
    virtual void samplesSynced(std::uint64_t lastSeq, std::int64_t nowNs) = 0;
};

} // namespace actcraft::core
