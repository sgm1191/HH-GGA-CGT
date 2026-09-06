#pragma once

#include <vector>

#include "bin.hpp"
#include "types.hpp"

namespace ggacgt {

// One BPP solution (individual). Replaces the original's six fake "extra bins" hack --
// per-individual metadata smuggled into `population[i][number_items + 0..5].Bin_Fullness`
// (REFACTORING_STRATEGY.md §3) -- with named fields. Mapping from the original slots:
//
//   [number_items + 0] -> fitness
//   [number_items + 1] -> binsUsed          (== bins.size() once an operator finishes)
//   [number_items + 2] -> birthGeneration
//   [number_items + 3] -> fullBinCount
//   [number_items + 4] -> minBinFullness    (see note below)
//   [number_items + 5] -> origin
//
// Note on minBinFullness: the original literally stores the FULLNESS of the least-full
// bin seen so far (initialized to `bin_capacity`, lowered as bins are added/updated),
// and every read site computes `bin_capacity - that_value` to get the actual spare
// capacity. REFACTORING_STRATEGY.md §3 names the slot `maxSpareCapacity`, which
// describes the *derived* quantity at read sites, not what is actually stored -- this
// field is named for what it literally holds to avoid that mismatch; callers that want
// the derived spare capacity compute `instance.capacity() - minBinFullness`.
struct Chromosome {
    enum class Origin { Initial, Crossover, Mutation };

    std::vector<Bin> bins;
    Fitness fitness = 0.0;
    long binsUsed = 0;
    GenerationNumber birthGeneration = 0;
    long fullBinCount = 0;
    Weight minBinFullness = 0;
    Origin origin = Origin::Initial;

    void clear() {
        bins.clear();
        fitness = 0.0;
        binsUsed = 0;
        birthGeneration = 0;
        fullBinCount = 0;
        minBinFullness = 0;
        origin = Origin::Initial;
    }
};

} // namespace ggacgt
