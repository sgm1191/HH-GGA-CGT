#include "adaptive_mutation.hpp"

#include <algorithm>
#include <cmath>
#include <numeric>

namespace ggacgt {

namespace {
// Sort_Ascending_BinFullness: deterministic-key sort (no RNG in the comparator), safe
// to implement with std::stable_sort -- the bubble sort it replaces has no tie-break
// beyond "leave equal elements in place", which stable_sort reproduces exactly
// (REFACTORING_STRATEGY.md §7.3).
void sortAscendingFullness(std::vector<long>& binIndices, const Chromosome& individual) {
    std::stable_sort(binIndices.begin(), binIndices.end(), [&](long a, long b) {
        return individual.bins[static_cast<std::size_t>(a)].fullness() <
               individual.bins[static_cast<std::size_t>(b)].fullness();
    });
}
} // namespace

void AdaptiveMutationRP::mutate(Chromosome& individual, float k, RngStreams& rng) {
    const long binsUsed = static_cast<long>(individual.bins.size());
    const BinCapacity capacity = instance_.capacity();

    std::vector<long> orderedBinFullness(static_cast<std::size_t>(binsUsed));
    std::iota(orderedBinFullness.begin(), orderedBinFullness.end(), 0L);

    Weight lighterWeightUnused = instance_.smallestWeight();
    shuffleRange(rng.permutation, orderedBinFullness, 0, binsUsed, instance_.weights(), lighterWeightUnused);
    sortAscendingFullness(orderedBinFullness, individual);

    // Original: `i = 1; while(bin[ordered[i]].fullness < capacity && i < binsUsed) i++;`
    // -- note it starts at index 1, never inspecting index 0.
    long i = 1;
    while (i < binsUsed && individual.bins[static_cast<std::size_t>(orderedBinFullness[static_cast<std::size_t>(i)])].fullness() < capacity) {
        ++i;
    }
    // Informacion-only diagnostics (arr_acul_muta[0]/[1]: `completos`/`incompletos`).
    lastStats_.incompleteBins = i;
    lastStats_.completeBins = binsUsed - i;

    // Original: `_p_ = 1/(float)k;` then the number-of-bins-to-remove formula, which
    // draws exactly once from the run's "empty bin" RNG stream.
    long double pExponent = 1.0L / static_cast<long double>(k);
    long double powI = std::pow(static_cast<long double>(i), pExponent);
    long double term = static_cast<long double>(i) *
                        ((2.0L - static_cast<long double>(i) / static_cast<long double>(binsUsed)) / powI);
    long drawRange = static_cast<long>(std::ceil((1.0L / powI) * 100.0L));
    int draw = rng.emptyBin.uniform(static_cast<int>(drawRange));
    long double factor = 1.0L - (static_cast<long double>(draw) / 100.0L);
    long numberBinsToRemove = static_cast<long>(std::ceil(static_cast<double>(term * factor)));
    // arr_acul_muta[2]: `eliminados` (number_bins, the removal count).
    lastStats_.eliminatedBins = numberBinsToRemove;

    std::vector<ItemId> freeItems;
    long lightestBin = 0;
    for (long removed = 0; removed < numberBinsToRemove; ++removed) {
        Bin& bin = individual.bins[static_cast<std::size_t>(orderedBinFullness[static_cast<std::size_t>(lightestBin)])];
        for (ItemId item : bin.items()) freeItems.push_back(item);
        bin.clear();
        ++lightestBin;
    }

    // Adjust_Solution: compact away the now-empty bins (which may be scattered at
    // arbitrary indices, since ordered_BinFullness need not be a prefix of raw indices)
    // while preserving the surviving bins' relative order -- exactly what a stable
    // remove_if/erase does.
    individual.bins.erase(std::remove_if(individual.bins.begin(), individual.bins.end(),
                                          [](const Bin& bin) { return bin.itemCount() == 0; }),
                           individual.bins.end());

    const long survivingBinCount = static_cast<long>(individual.bins.size());
    // arr_acul_muta[3]/[4]: `number_bins` (post-removal, pre-reinsertion bin count) and
    // `number_free_items`.
    lastStats_.survivingBins = survivingBinCount;
    lastStats_.freeItems = static_cast<long>(freeItems.size());

    individual.binsUsed = rp_.rearrange(individual, survivingBinCount, freeItems, rng.permutation);
}

} // namespace ggacgt
