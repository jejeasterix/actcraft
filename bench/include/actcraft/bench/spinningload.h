#pragma once

#include <cstdint>

namespace actcraft::bench {

// L5-C14 : charge témoin de « export-fond-reference » (voir ScenarioPlan::spinsInBackground).
// Squelette : la tâche suivante le remplit.
class SpinningLoad
{
public:
    std::int64_t iterations() const noexcept { return 0; }
};

} // namespace actcraft::bench
