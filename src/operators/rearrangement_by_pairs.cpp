#include "rearrangement_by_pairs.hpp"

#include <algorithm>
#include <cmath>
#include <numeric>

#include "ff_common.hpp"

namespace ggacgt {

long RearrangementByPairs::rearrange(Chromosome& individual, long survivingBinCount,
                                      std::vector<ItemId> freeItems, Rng& permutationRng) {
    const long b = survivingBinCount;
    const long numberFreeItemsOriginal = static_cast<long>(freeItems.size());
    const BinCapacity capacity = instance_.capacity();
    const auto& W = instance_.weights();
    const double capacityD = static_cast<double>(capacity);

    // Original: `higher_weight = weight[F[0]]; lighter_weight = weight[F[0]];`, an
    // unconditional read of F[0]. F is a caller-owned, zero-initialized fixed array in
    // the original, so F[0] == item 0's weight whenever the free pool is (rarely, but
    // reachably -- see hh/CHECKLIST.md) empty. Reproduced via the same fallback rather
    // than reading out of bounds on an empty vector.
    Weight higherWeight = freeItems.empty() ? instance_.weight(0) : W[static_cast<std::size_t>(freeItems[0])];
    Weight lighterWeight = higherWeight;

    individual.fitness = 0.0;
    individual.fullBinCount = 0;
    individual.minBinFullness = capacity;

    std::vector<long> orderedBins(static_cast<std::size_t>(b));
    std::iota(orderedBins.begin(), orderedBins.end(), 0L);
    // NOTE: faithfully reproduces an apparent original quirk. Sort_Random's
    // lighter_weight side effect (see core/rng.hpp) looks up `weight[array[i]]` for
    // whatever array it's given -- here that's `ordered_BinFullness`, an array of BIN
    // indices, not item ids. The original calls it this way regardless, so
    // `lighterWeight` genuinely gets perturbed by treating bin indices as item ids. Not
    // "fixed" here; see hh/CHECKLIST.md Recommendations.
    shuffleRange(permutationRng, orderedBins, 0, b, W, lighterWeight);
    shuffleRange(permutationRng, freeItems, 0, numberFreeItemsOriginal, W, lighterWeight);

    long binI = b;

    for (long i = 0; i < b; ++i) {
        Bin& bin = individual.bins[static_cast<std::size_t>(orderedBins[static_cast<std::size_t>(i)])];
        long sum = bin.fullness();
        bool ban = false;

        for (long pIdx = 0; !ban && pIdx + 1 < static_cast<long>(bin.items().size()); ++pIdx) {
            for (long sIdx = pIdx + 1; !ban && sIdx < static_cast<long>(bin.items().size()); ++sIdx) {
                for (long k = 0; !ban && k < numberFreeItemsOriginal - 1; ++k) {
                    if (i == b - 1) {
                        Weight fkPeek = W[static_cast<std::size_t>(freeItems[static_cast<std::size_t>(k)])];
                        if (fkPeek > higherWeight) higherWeight = fkPeek;
                    }

                    for (long k2 = k + 1; !ban && k2 < numberFreeItemsOriginal; ++k2) {
                        // Re-read every value fresh each iteration -- a case (c) swap
                        // below (when it doesn't hit exact capacity) does NOT break,
                        // and the next k2 iteration must see the items it just swapped
                        // in, exactly like the original's pointer/array dereferences do.
                        ItemId pItem = bin.items()[static_cast<std::size_t>(pIdx)];
                        ItemId sItem = bin.items()[static_cast<std::size_t>(sIdx)];
                        Weight pW = W[static_cast<std::size_t>(pItem)];
                        Weight sW = W[static_cast<std::size_t>(sItem)];
                        Weight fk = W[static_cast<std::size_t>(freeItems[static_cast<std::size_t>(k)])];
                        Weight fk2 = W[static_cast<std::size_t>(freeItems[static_cast<std::size_t>(k2)])];
                        Weight pairWeight = pW + sW;

                        if (fk >= pairWeight && (sum - pairWeight + fk) <= capacity) {
                            sum = sum - pairWeight + fk;
                            ItemId evictedP = pItem;
                            ItemId evictedS = sItem;
                            bin.items()[static_cast<std::size_t>(pIdx)] = freeItems[static_cast<std::size_t>(k)];
                            bin.items().erase(bin.items().begin() + sIdx);
                            freeItems[static_cast<std::size_t>(k)] = evictedP;
                            freeItems.push_back(evictedS);
                            ban = true;
                            break;
                        }
                        if (fk2 >= pairWeight && (sum - pairWeight + fk2) <= capacity) {
                            sum = sum - pairWeight + fk2;
                            ItemId evictedP = pItem;
                            ItemId evictedS = sItem;
                            bin.items()[static_cast<std::size_t>(pIdx)] = freeItems[static_cast<std::size_t>(k2)];
                            bin.items().erase(bin.items().begin() + sIdx);
                            freeItems[static_cast<std::size_t>(k2)] = evictedP;
                            freeItems.push_back(evictedS);
                            ban = true;
                            break;
                        }
                        if ((fk + fk2 > pairWeight) || (fk + fk2 == pairWeight && !(fk == pW || fk == sW))) {
                            if (sum - pairWeight + (fk + fk2) > capacity) break; // exits k2 loop only, no match
                            sum = sum - pairWeight + (fk + fk2);
                            ItemId evictedP = pItem;
                            ItemId evictedS = sItem;
                            bin.items()[static_cast<std::size_t>(pIdx)] = freeItems[static_cast<std::size_t>(k)];
                            bin.items()[static_cast<std::size_t>(sIdx)] = freeItems[static_cast<std::size_t>(k2)];
                            freeItems[static_cast<std::size_t>(k)] = evictedP;
                            freeItems[static_cast<std::size_t>(k2)] = evictedS;
                            if (sum == capacity) {
                                ban = true;
                                break;
                            }
                            // else: no break -- keep scanning k2 with the swapped-in items
                        }
                    }
                }
            }
        }

        bin.setFullness(sum);
        if (bin.fullness() < individual.minBinFullness) individual.minBinFullness = bin.fullness();
        if (bin.fullness() == capacity) {
            ++individual.fullBinCount;
        } else if (bin.fullness() + instance_.smallestWeight() <= capacity) {
            if (orderedBins[static_cast<std::size_t>(i)] < binI) binI = orderedBins[static_cast<std::size_t>(i)];
        }
    }

    for (long i = 0; i < binI; ++i) {
        individual.fitness +=
            std::pow(static_cast<double>(individual.bins[static_cast<std::size_t>(i)].fullness()) / capacityD, 2);
    }

    if (static_cast<double>(higherWeight) < 0.5 * capacityD) {
        shuffleRange(permutationRng, freeItems, 0, static_cast<long>(freeItems.size()), W, lighterWeight);
    } else {
        std::stable_sort(freeItems.begin(), freeItems.end(), [&](ItemId a, ItemId b2) {
            return W[static_cast<std::size_t>(a)] > W[static_cast<std::size_t>(b2)];
        });
        if (!freeItems.empty()) lighterWeight = W[static_cast<std::size_t>(freeItems.back())];
    }

    if (lighterWeight > capacity - individual.minBinFullness) {
        for (long i = binI; i < b; ++i) {
            individual.fitness += std::pow(
                static_cast<double>(individual.bins[static_cast<std::size_t>(i)].fullness()) / capacityD, 2);
        }
        binI = b;
    }

    if (!freeItems.empty()) {
        for (std::size_t idx = 0; idx + 1 < freeItems.size(); ++idx) {
            firstFitInsert(instance_, freeItems[idx], individual, binI, binI, false);
        }
        firstFitInsert(instance_, freeItems.back(), individual, binI, binI, true);
    }

    individual.binsUsed = static_cast<long>(individual.bins.size());
    return individual.binsUsed;
}

} // namespace ggacgt
