#include "gene_level_crossover.hpp"

#include <algorithm>
#include <cmath>
#include <numeric>

#include "ff_common.hpp"
#include "gene_level_crossover_common.hpp"

namespace ggacgt {

using crossover_detail::markBinItemsIfUnused;
using crossover_detail::sortDescendingFullnessThenItems;

Chromosome GeneLevelCrossoverFFD::cross(const Chromosome& parent1, const Chromosome& parent2,
                                         RngStreams& rng, GenerationNumber birthGeneration,
                                         CrossoverCounters& counters) {
    Chromosome child;
    child.birthGeneration = birthGeneration;
    child.origin = Chromosome::Origin::Crossover;
    child.minBinFullness = instance_.capacity();

    const long bins1 = static_cast<long>(parent1.bins.size());
    const long bins2 = static_cast<long>(parent2.bins.size());
    const long counter = std::max(bins1, bins2);

    // Identity-filled to `counter` (== max parent bin count), matching the original's
    // `random_order1[k] = k` loop, but only [0, bins1) / [0, bins2) get shuffled below
    // -- the tail (if any) is never read, since the main loop below only runs bins1
    // iterations and guards every parent2 access by `order2[k] < bins2`.
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

    auto transmit = [&](const Chromosome& parent, long binIndex) {
        const Bin& bin = parent.bins[static_cast<std::size_t>(binIndex)];
        if (markBinItemsIfUnused(used, bin)) {
            child.bins.push_back(bin);
            if (child.bins.back().fullness() < child.minBinFullness)
                child.minBinFullness = child.bins.back().fullness();
        } else {
            ++counters.deletedBins;
            counters.deletedItems += bin.itemCount();
        }
    };

    for (long k = 0; k < bins1; ++k) {
        // Original: `float aleatorio = randp(&seed);` -- the run's "coin" stream.
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

    // Items not transmitted from either parent, in descending weight order (original:
    // scanning `ordered_weight[]` and collecting items with `items[item] == 0`).
    std::vector<ItemId> freeItems;
    freeItems.reserve(static_cast<std::size_t>(instance_.numberOfItems()));
    for (ItemId item : instance_.descendingWeightOrder()) {
        if (!used[static_cast<std::size_t>(item)]) freeItems.push_back(item);
    }

    if (!freeItems.empty()) {
        long binI = 0;
        for (std::size_t i = 0; i + 1 < freeItems.size(); ++i) {
            firstFitInsert(instance_, freeItems[i], child, binI, binI, false);
        }
        firstFitInsert(instance_, freeItems.back(), child, binI, binI, true);
    } else {
        // Original: every item was transmitted, so no FF pass runs -- the fitness sum
        // that FF's `is_last` sweep would otherwise have produced is computed directly.
        const double capacityD = static_cast<double>(instance_.capacity());
        for (const Bin& bin : child.bins) {
            child.fitness += std::pow(static_cast<double>(bin.fullness()) / capacityD, 2);
        }
    }

    child.binsUsed = static_cast<long>(child.bins.size());
    return child;
}

} // namespace ggacgt
