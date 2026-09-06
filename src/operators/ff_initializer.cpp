#include "ff_initializer.hpp"

#include "ff_common.hpp"

namespace ggacgt {

FFInitializer::FFInitializer(const BppInstance& instance, long largeItemCount)
    : instance_(instance), largeItemCount_(largeItemCount) {
    // Matches main(): `for(i=0,j=n_;j<number_items;i++) permutation[i]=ordered_weight[j++];`
    const auto& order = instance_.descendingWeightOrder();
    smallItemsBuffer_.assign(order.begin() + largeItemCount_, order.end());
}

Chromosome FFInitializer::generate(Rng& permutationRng, GenerationNumber birthGeneration) {
    Chromosome individual;
    individual.birthGeneration = birthGeneration;
    individual.minBinFullness = instance_.capacity();
    individual.origin = Chromosome::Origin::Initial;

    for (long i = 0; i < largeItemCount_; ++i) {
        ItemId item = instance_.descendingWeightOrder()[static_cast<std::size_t>(i)];
        individual.bins.emplace_back();
        individual.bins.back().insert(item, instance_.weight(item));
        if (individual.bins.back().fullness() < individual.minBinFullness)
            individual.minBinFullness = individual.bins.back().fullness();
    }

    // Reshuffles the shared small-items buffer in place (see header: this is the
    // original's persistent, never-reset-to-identity `permutation` global).
    Weight lighterWeightSideEffect = instance_.smallestWeight(); // unused by this call site, mirrors the original discarding it here too
    const long count = static_cast<long>(smallItemsBuffer_.size());
    shuffleRange(permutationRng, smallItemsBuffer_, 0, count, instance_.weights(), lighterWeightSideEffect);

    // NOTE (deliberate deviation, documented in hh/CHECKLIST.md Recommendations): the
    // original unconditionally calls `FF(permutation[j], ..., is_last=1)` once after the
    // loop even when `count == 0` (every item was "large"), reading `permutation[0]`
    // uninitialized/stale in that case. That's a latent out-of-bounds/garbage-item bug
    // in the original for the "all items large" edge case, not an intentional
    // behavior -- it is not reproduced here; the trailing FF call is skipped when there
    // are no small items left to place.
    long binI = 0;
    if (count > 0) {
        for (long j = 0; j < count - 1; ++j) {
            firstFitInsert(instance_, smallItemsBuffer_[static_cast<std::size_t>(j)], individual, binI, binI, false);
        }
        firstFitInsert(instance_, smallItemsBuffer_[static_cast<std::size_t>(count - 1)], individual, binI, binI, true);
    }

    individual.binsUsed = static_cast<long>(individual.bins.size());
    return individual;
}

} // namespace ggacgt
