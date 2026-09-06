#pragma once

#include <algorithm>
#include <vector>

#include "../core/chromosome.hpp"

namespace ggacgt {
namespace crossover_detail {

// Sort_Descending_BinFullness_Items: order the first `count` entries of `binIndices`
// (indices into `parent.bins`) by descending fullness, ties broken by ASCENDING item
// count -- the original's tie-break swap is `if(items1_bin > items2_bin) swap(m, m+1)`,
// which in a bubble sort pushes the higher-item-count bin to a LATER position, i.e.
// fewer items sorts first. `count` matters: the original bounds this sort by the
// PARENT's own bin count (`k = population[individual][number_items+1].Bin_Fullness - 1`),
// not by `binIndices`'s allocated size (which is `max(bins1, bins2)`, per the caller) --
// sorting past `count` would read out of `parent.bins`'s range whenever the two
// parents' bin counts differ. Both keys are pure comparisons with no RNG involved, so
// std::stable_sort converges to the exact same total order the original's bubble sort
// produced -- safe to upgrade per REFACTORING_STRATEGY.md §7.3 (only sorts whose
// *comparator* consumes RNG need a literal bubble-sort port; this one doesn't).
//
// Shared by GeneLevelCrossoverFFD (base) and GeneLevelCrossoverFFDGLX (informacion) --
// both original functions call the identical Sort_Descending_BinFullness_Items.
inline void sortDescendingFullnessThenItems(std::vector<long>& binIndices, long count, const Chromosome& parent) {
    std::stable_sort(binIndices.begin(), binIndices.begin() + count, [&](long a, long b) {
        const Bin& ba = parent.bins[static_cast<std::size_t>(a)];
        const Bin& bb = parent.bins[static_cast<std::size_t>(b)];
        if (ba.fullness() != bb.fullness()) return ba.fullness() > bb.fullness();
        return ba.itemCount() < bb.itemCount();
    });
}

// Used_Items(): if none of `bin`'s items are already marked in `used`, marks them all
// and returns true (bin can be transmitted); otherwise rolls back only the items this
// call itself marked (not any earlier-accepted bin's items) and returns false. Shared
// for the same reason as above.
inline bool markBinItemsIfUnused(std::vector<bool>& used, const Bin& bin) {
    std::vector<ItemId> marked;
    marked.reserve(static_cast<std::size_t>(bin.itemCount()));
    for (ItemId item : bin.items()) {
        if (!used[static_cast<std::size_t>(item)]) {
            used[static_cast<std::size_t>(item)] = true;
            marked.push_back(item);
        } else {
            for (ItemId m : marked) used[static_cast<std::size_t>(m)] = false;
            return false;
        }
    }
    return true;
}

} // namespace crossover_detail
} // namespace ggacgt
