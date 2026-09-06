#pragma once

#include <vector>

#include "../core/rng.hpp"
#include "../core/types.hpp"

namespace ggacgt {

// The original's `random_individuals`/`best_individuals` globals: two persistent
// rank-index pools, reshuffled (not rebuilt) every generation -- see core/rng.hpp's
// shuffleRange doc comment on why "persistent, reshuffled in place" rather than "fresh
// each time" matters for reproducing the original's exact sequence for a given seed.
struct SelectionPools {
    std::vector<long> randomIndividuals;
    std::vector<long> bestIndividuals;

    explicit SelectionPools(long pSize)
        : randomIndividuals(static_cast<std::size_t>(pSize)),
          bestIndividuals(static_cast<std::size_t>(pSize)) {
        for (long i = 0; i < pSize; ++i) {
            randomIndividuals[static_cast<std::size_t>(i)] = i;
            bestIndividuals[static_cast<std::size_t>(i)] = i;
        }
    }
};

// Original: `Sort_Random(random_individuals,0,(int)(P_size-(int)(P_size*B_size)));` and
// `Sort_Random(best_individuals,(1-p_c)*P_size,P_size);`, in that order (both draw from
// the run's permutation RNG stream, so the order matters). The int-truncating casts and
// the float `(1-p_c)*P_size` start bound are reproduced exactly, including the implicit
// float/int truncation-toward-zero semantics.
void reshuffleSelectionPools(SelectionPools& pools, long pSize, float bSize, float pC, Rng& permutationRng);

} // namespace ggacgt
