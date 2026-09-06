#include "gene_level_crossover_glx.hpp"

#include <algorithm>
#include <cmath>
#include <numeric>

#include "ff_common.hpp"
#include "gene_level_crossover_common.hpp"

namespace ggacgt {

using crossover_detail::markBinItemsIfUnused;
using crossover_detail::sortDescendingFullnessThenItems;

Chromosome GeneLevelCrossoverFFDGLX::cross(const Chromosome& parent1, const Chromosome& parent2,
                                            RngStreams& rng, GenerationNumber birthGeneration,
                                            CrossoverCounters& counters) {
    Chromosome child;
    child.birthGeneration = birthGeneration;
    child.origin = Chromosome::Origin::Crossover;
    child.minBinFullness = instance_.capacity();

    // arr[3]/arr[4]: both parents' fullBinCount/binsUsed, summed before anything else
    // runs (matches the original reading these first, from the untouched parents).
    const long parentsFullBins = parent1.fullBinCount + parent2.fullBinCount;
    const long parentsBinsUsed = parent1.binsUsed + parent2.binsUsed;

    const long bins1 = static_cast<long>(parent1.bins.size());
    const long bins2 = static_cast<long>(parent2.bins.size());
    const long counter = std::max(bins1, bins2); // arr[2]: gen_p

    std::vector<long> order1(static_cast<std::size_t>(counter));
    std::vector<long> order2(static_cast<std::size_t>(counter));
    std::iota(order1.begin(), order1.end(), 0L);
    std::iota(order2.begin(), order2.end(), 0L);

    Weight lighterWeightUnused = instance_.smallestWeight();
    shuffleRange(rng.permutation, order1, 0, bins1, instance_.weights(), lighterWeightUnused);
    shuffleRange(rng.permutation, order2, 0, bins2, instance_.weights(), lighterWeightUnused);
    sortDescendingFullnessThenItems(order1, bins1, parent1);
    sortDescendingFullnessThenItems(order2, bins2, parent2);

    std::vector<bool> used(static_cast<std::size_t>(instance_.numberOfItems()), false);
    const BinCapacity capacity = instance_.capacity();
    long counterFull = 0; // informacion-only: bins transmitted at exact capacity

    auto transmit = [&](const Chromosome& parent, long binIndex) {
        const Bin& bin = parent.bins[static_cast<std::size_t>(binIndex)];
        if (markBinItemsIfUnused(used, bin)) {
            if (bin.fullness() == capacity) ++counterFull;
            child.bins.push_back(bin);
            if (child.bins.back().fullness() < child.minBinFullness)
                child.minBinFullness = child.bins.back().fullness();
        } else {
            ++counters.deletedBins;
            counters.deletedItems += bin.itemCount();
        }
    };

    for (long k = 0; k < bins1; ++k) {
        float aleatorio = rng.coin.next01();

        if (aleatorio > 0.5f) {
            if (order2[static_cast<std::size_t>(k)] < bins2) {
                transmit(parent2, order2[static_cast<std::size_t>(k)]);
            }
            transmit(parent1, order1[static_cast<std::size_t>(k)]);
        } else {
            transmit(parent1, order1[static_cast<std::size_t>(k)]);
            if (order2[static_cast<std::size_t>(k)] < bins2) {
                transmit(parent2, order2[static_cast<std::size_t>(k)]);
            }
        }
    }

    std::vector<ItemId> freeItems;
    freeItems.reserve(static_cast<std::size_t>(instance_.numberOfItems()));
    for (ItemId item : instance_.descendingWeightOrder()) {
        if (!used[static_cast<std::size_t>(item)]) freeItems.push_back(item);
    }

    // arr[0]/arr[1]/arr[5]: k2 (bins transmitted so far), k (free item count), and the
    // full-bin ratio -- all captured now, matching the original's placement right
    // after the free-item scan and before reinsertion.
    const long partialBins = static_cast<long>(child.bins.size());
    lastStats_.partialBins = partialBins;
    lastStats_.eliminatedItems = static_cast<long>(freeItems.size());
    lastStats_.generatedPairSize = counter;
    lastStats_.parentsFullBins = parentsFullBins;
    lastStats_.parentsBinsUsed = parentsBinsUsed;
    lastStats_.fullBinRatio = partialBins > 0 ? static_cast<double>(counterFull) / static_cast<double>(partialBins) : 0.0;

    if (!freeItems.empty()) {
        rpGlx_.rearrange(child, partialBins, freeItems, rng.permutation);
    } else {
        const double capacityD = static_cast<double>(capacity);
        for (const Bin& bin : child.bins) {
            child.fitness += std::pow(static_cast<double>(bin.fullness()) / capacityD, 2);
        }
    }

    child.binsUsed = static_cast<long>(child.bins.size());
    return child;
}

} // namespace ggacgt
