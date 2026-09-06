#pragma once

#include <vector>

#include "operator_interfaces.hpp"

namespace ggacgt {

// First-Fit with n_ pre-allocated large items (FF-n_ in the original: FF_n_). Builds
// one new random BPP solution: the `largeItemCount` heaviest items (each > capacity/2)
// go one-per-bin up front, then the remaining items are shuffled and placed First-Fit.
class FFInitializer : public IInitializer {
public:
    FFInitializer(const BppInstance& instance, long largeItemCount);

    Chromosome generate(Rng& permutationRng, GenerationNumber birthGeneration) override;

private:
    const BppInstance& instance_;
    long largeItemCount_;

    // The original's shared `permutation` global: the small-items list (items beyond
    // the largeItemCount heaviest, in descending weight order), reshuffled via
    // Sort_Random on EVERY generate() call rather than rebuilt from scratch each time
    // -- i.e. generate() for the (k+1)-th individual shuffles whatever order the k-th
    // individual's shuffle left behind, not a fresh copy of the descending order. This
    // is load-bearing for reproducing the original's exact initial population for a
    // given seed, so it is instance state (persisted across calls), not a local.
    std::vector<long> smallItemsBuffer_;
};

} // namespace ggacgt
