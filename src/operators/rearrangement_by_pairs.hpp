#pragma once

#include <vector>

#include "../core/bpp_instance.hpp"
#include "../core/chromosome.hpp"
#include "../core/rng.hpp"

namespace ggacgt {

// Rearrangement by Pairs (original: RP). Given `individual`'s `survivingBinCount` bins
// (already compacted to indices [0, survivingBinCount) by the caller -- mirrors the
// original always running Adjust_Solution before RP) and a pool of items freed by
// mutation, tries, for every pair of items within every surviving bin, to find one or
// two free items that can replace them (shrinking the free pool, or at least tightening
// that bin), then First-Fit-reinserts whatever remains free.
//
// This is a near-literal, index-based port of the original's pointer-walking,
// in-place-array-mutating implementation -- see rearrangement_by_pairs.cpp for the
// reasoning; the short version is that several of its apparent "just simplify this"
// spots are load-bearing for reproducing the original's exact output for a given seed
// (REFACTORING_STRATEGY.md §4), so it is deliberately not rewritten into a cleaner
// shape.
class RearrangementByPairs {
public:
    explicit RearrangementByPairs(const BppInstance& instance) : instance_(instance) {}

    // Returns the final bin count (== individual.bins.size() afterward).
    long rearrange(Chromosome& individual, long survivingBinCount, std::vector<ItemId> freeItems,
                    Rng& permutationRng);

private:
    const BppInstance& instance_;
};

} // namespace ggacgt
