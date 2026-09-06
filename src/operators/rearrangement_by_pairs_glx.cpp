#include "rearrangement_by_pairs_glx.hpp"

#include <algorithm>
#include <cmath>
#include <numeric>

#include "ff_common.hpp"

namespace ggacgt {

long RearrangementByPairsGLX::rearrange(Chromosome& individual, long survivingBinCount,
                                         std::vector<ItemId> freeItems, Rng& permutationRng) {
    const long b = survivingBinCount;
    const long numberFreeItemsOriginal = static_cast<long>(freeItems.size());
    const BinCapacity capacity = instance_.capacity();
    const auto& W = instance_.weights();
    const double capacityD = static_cast<double>(capacity);

    // Precondition: RP_GLX is only ever called (from crossover) when there ARE free
    // items, so unlike RearrangementByPairs::rearrange, F[0] is always safe to read.
    Weight higherWeight = W[static_cast<std::size_t>(freeItems[0])];
    Weight lighterWeight = higherWeight;

    individual.fitness = 0.0;
    individual.fullBinCount = 0;
    individual.minBinFullness = capacity;

    std::vector<long> orderedBins(static_cast<std::size_t>(b));
    std::iota(orderedBins.begin(), orderedBins.end(), 0L);
    // Same reproduced quirk as RearrangementByPairs: this shuffle's lighter_weight
    // side effect looks up weight[bin index] as if it were an item id (see that file's
    // header comment) -- but unlike RP, RP_GLX does NOT shuffle F at all (see below),
    // so this bin-index-corrupted value survives all the way to the final
    // lighter_weight check unless the tail-shuffle near the end overwrites it.
    shuffleRange(permutationRng, orderedBins, 0, b, W, lighterWeight);
    // Original: `Sort_Ascending_Weights(F, number_free_items)` -- deterministic, no
    // RNG draw (unlike RP's `Sort_Random(F, 0, number_free_items)`). This is a genuine
    // RNG-consumption difference from RP: RP_GLX draws b times here, RP draws
    // b + numberFreeItemsOriginal times.
    std::stable_sort(freeItems.begin(), freeItems.begin() + numberFreeItemsOriginal,
                      [&](ItemId a, ItemId b2) { return W[static_cast<std::size_t>(a)] < W[static_cast<std::size_t>(b2)]; });

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
                        // Cases below deliberately re-read bin.items()[pIdx/sIdx] and
                        // freeItems[k/k2] FRESH before each case check (not cached
                        // once per k2 iteration): the original dereferences live
                        // pointers (`p->data`, `s->data`), and case 3's swap below can
                        // mutate the bin AND F in place WITHOUT breaking (only an
                        // exact-capacity match breaks) -- so cases 4 and 5, checked in
                        // the very same k2 iteration right after, must see whatever
                        // case 3 just wrote. Caching these once at the top of the
                        // iteration (as an earlier draft of this file did) is the one
                        // subtle deviation from RearrangementByPairs's "read everything
                        // fresh" discipline that actually mattered here, since RP has
                        // no case 4/5 fallthrough to expose it.

                        // Case 1: F[k] alone replaces both p and s.
                        {
                            ItemId pItem = bin.items()[static_cast<std::size_t>(pIdx)];
                            ItemId sItem = bin.items()[static_cast<std::size_t>(sIdx)];
                            Weight pW = W[static_cast<std::size_t>(pItem)];
                            Weight sW = W[static_cast<std::size_t>(sItem)];
                            Weight fk = W[static_cast<std::size_t>(freeItems[static_cast<std::size_t>(k)])];
                            Weight pairWeight = pW + sW;
                            if (fk >= pairWeight && (sum - pairWeight + fk) <= capacity) {
                                sum = sum - pairWeight + fk;
                                bin.items()[static_cast<std::size_t>(pIdx)] = freeItems[static_cast<std::size_t>(k)];
                                bin.items().erase(bin.items().begin() + sIdx);
                                freeItems[static_cast<std::size_t>(k)] = pItem;
                                freeItems.push_back(sItem);
                                ban = true;
                                break;
                            }
                        }
                        // Case 2: F[k2] alone replaces both p and s.
                        {
                            ItemId pItem = bin.items()[static_cast<std::size_t>(pIdx)];
                            ItemId sItem = bin.items()[static_cast<std::size_t>(sIdx)];
                            Weight pW = W[static_cast<std::size_t>(pItem)];
                            Weight sW = W[static_cast<std::size_t>(sItem)];
                            Weight fk2 = W[static_cast<std::size_t>(freeItems[static_cast<std::size_t>(k2)])];
                            Weight pairWeight = pW + sW;
                            if (fk2 >= pairWeight && (sum - pairWeight + fk2) <= capacity) {
                                sum = sum - pairWeight + fk2;
                                bin.items()[static_cast<std::size_t>(pIdx)] = freeItems[static_cast<std::size_t>(k2)];
                                bin.items().erase(bin.items().begin() + sIdx);
                                freeItems[static_cast<std::size_t>(k2)] = pItem;
                                freeItems.push_back(sItem);
                                ban = true;
                                break;
                            }
                        }
                        // Case 3: F[k]+F[k2] together improve on p+s (2-for-2 swap).
                        // May mutate the bin/F and fall through WITHOUT breaking.
                        {
                            ItemId pItem = bin.items()[static_cast<std::size_t>(pIdx)];
                            ItemId sItem = bin.items()[static_cast<std::size_t>(sIdx)];
                            Weight pW = W[static_cast<std::size_t>(pItem)];
                            Weight sW = W[static_cast<std::size_t>(sItem)];
                            Weight fk = W[static_cast<std::size_t>(freeItems[static_cast<std::size_t>(k)])];
                            Weight fk2 = W[static_cast<std::size_t>(freeItems[static_cast<std::size_t>(k2)])];
                            Weight pairWeight = pW + sW;
                            if ((fk + fk2 > pairWeight) || (fk + fk2 == pairWeight && !(fk == pW || fk == sW))) {
                                if (sum - pairWeight + (fk + fk2) > capacity) break; // exits k2 loop only, no match
                                sum = sum - pairWeight + (fk + fk2);
                                bin.items()[static_cast<std::size_t>(pIdx)] = freeItems[static_cast<std::size_t>(k)];
                                bin.items()[static_cast<std::size_t>(sIdx)] = freeItems[static_cast<std::size_t>(k2)];
                                freeItems[static_cast<std::size_t>(k)] = pItem;
                                freeItems[static_cast<std::size_t>(k2)] = sItem;
                                if (sum == capacity) {
                                    ban = true;
                                    break;
                                }
                                // else: no break -- fall through to cases 4/5 below,
                                // which must see the swap just applied.
                            }
                        }
                        // Cases 4-7 (GLX-only, not in RP): a single free item alone
                        // replaces just ONE of p/s (no partner needed, no free-pool
                        // growth) -- ALL FOUR combinations of {F[k], F[k2]} x {p, s},
                        // checked in exactly this order. An earlier draft of this file
                        // only implemented two of these four (F[k]->p and F[k2]->s),
                        // which was the actual root cause of a golden-test divergence
                        // (see hh/CHECKLIST.md Recommendations) -- found by bisecting
                        // down to a byte-for-byte re-comparison against the source
                        // after the "read fresh" fix alone wasn't enough to match.

                        // Case 4: F[k] alone replaces just p.
                        {
                            ItemId pItem = bin.items()[static_cast<std::size_t>(pIdx)];
                            Weight pW = W[static_cast<std::size_t>(pItem)];
                            Weight fk = W[static_cast<std::size_t>(freeItems[static_cast<std::size_t>(k)])];
                            if (fk > pW && (sum - pW + fk) <= capacity) {
                                sum = sum - pW + fk;
                                bin.items()[static_cast<std::size_t>(pIdx)] = freeItems[static_cast<std::size_t>(k)];
                                freeItems[static_cast<std::size_t>(k)] = pItem;
                                ban = true;
                                break;
                            }
                        }
                        // Case 5: F[k] alone replaces just s.
                        {
                            ItemId sItem = bin.items()[static_cast<std::size_t>(sIdx)];
                            Weight sW = W[static_cast<std::size_t>(sItem)];
                            Weight fk = W[static_cast<std::size_t>(freeItems[static_cast<std::size_t>(k)])];
                            if (fk > sW && (sum - sW + fk) <= capacity) {
                                sum = sum - sW + fk;
                                bin.items()[static_cast<std::size_t>(sIdx)] = freeItems[static_cast<std::size_t>(k)];
                                freeItems[static_cast<std::size_t>(k)] = sItem;
                                ban = true;
                                break;
                            }
                        }
                        // Case 6: F[k2] alone replaces just p.
                        {
                            ItemId pItem = bin.items()[static_cast<std::size_t>(pIdx)];
                            Weight pW = W[static_cast<std::size_t>(pItem)];
                            Weight fk2 = W[static_cast<std::size_t>(freeItems[static_cast<std::size_t>(k2)])];
                            if (fk2 > pW && (sum - pW + fk2) <= capacity) {
                                sum = sum - pW + fk2;
                                bin.items()[static_cast<std::size_t>(pIdx)] = freeItems[static_cast<std::size_t>(k2)];
                                freeItems[static_cast<std::size_t>(k2)] = pItem;
                                ban = true;
                                break;
                            }
                        }
                        // Case 7: F[k2] alone replaces just s.
                        {
                            ItemId sItem = bin.items()[static_cast<std::size_t>(sIdx)];
                            Weight sW = W[static_cast<std::size_t>(sItem)];
                            Weight fk2 = W[static_cast<std::size_t>(freeItems[static_cast<std::size_t>(k2)])];
                            if (fk2 > sW && (sum - sW + fk2) <= capacity) {
                                sum = sum - sW + fk2;
                                bin.items()[static_cast<std::size_t>(sIdx)] = freeItems[static_cast<std::size_t>(k2)];
                                freeItems[static_cast<std::size_t>(k2)] = sItem;
                                ban = true;
                                break;
                            }
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

    // Original: unconditional `Sort_Descending_Weights(F, number_free_items)` (unlike
    // RP's higher_weight-conditional branch), then large items (weight >= capacity/2)
    // are counted and left in place at the front, and only the TAIL is shuffled --
    // GLX's intent is evidently to seed the final FF pass with large items first,
    // deterministically, rather than RP's "shuffle everything or sort everything"
    // choice.
    std::stable_sort(freeItems.begin(), freeItems.end(),
                      [&](ItemId a, ItemId b2) { return W[static_cast<std::size_t>(a)] > W[static_cast<std::size_t>(b2)]; });

    long bigItem = 0;
    for (std::size_t i = 0; i < freeItems.size(); ++i) {
        if (W[static_cast<std::size_t>(freeItems[i])] < capacity / 2) break;
        ++bigItem;
    }
    shuffleRange(permutationRng, freeItems, bigItem, static_cast<long>(freeItems.size()), W, lighterWeight);

    if (lighterWeight > capacity - individual.minBinFullness) {
        for (long i = binI; i < b; ++i) {
            individual.fitness += std::pow(
                static_cast<double>(individual.bins[static_cast<std::size_t>(i)].fullness()) / capacityD, 2);
        }
        binI = b;
    }

    // Precondition guarantees freeItems is non-empty here (see header comment) --
    // matches the original, which unconditionally calls FF(F[i],...,is_last=1) with no
    // empty-pool guard in RP_GLX either.
    for (std::size_t idx = 0; idx + 1 < freeItems.size(); ++idx) {
        firstFitInsert(instance_, freeItems[idx], individual, binI, binI, false);
    }
    firstFitInsert(instance_, freeItems.back(), individual, binI, binI, true);

    individual.binsUsed = static_cast<long>(individual.bins.size());
    return individual.binsUsed;
}

} // namespace ggacgt
