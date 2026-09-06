#pragma once

#include <string>
#include <vector>

#include "types.hpp"

namespace ggacgt {

// A Bin Packing Problem instance: item weights, bin capacity, and the derived data
// (descending weight order, total weight) that the original computes once per instance
// and reuses for the whole run (REFACTORING_STRATEGY.md §2, core/bpp_instance.hpp).
// Immutable after construction -- replaces the file-scope `weight[]`, `bin_capacity`,
// `number_items`, `ordered_weight[]`, `total_accumulated_weight` globals.
class BppInstance {
public:
    BppInstance(std::string name, std::vector<Weight> weights, BinCapacity capacity,
                long bestKnownSolution);

    long numberOfItems() const { return static_cast<long>(weights_.size()); }
    Weight weight(ItemId item) const { return weights_[item]; }
    const std::vector<Weight>& weights() const { return weights_; }
    BinCapacity capacity() const { return capacity_; }
    long bestKnownSolution() const { return bestKnownSolution_; }
    Weight totalWeight() const { return totalWeight_; }
    // Weight of the globally lightest item (== weight of the last entry in
    // descendingWeightOrder()). Used by the First-Fit placement logic to decide when a
    // bin can no longer fit *any* remaining item (original: `weight[ordered_weight[number_items-1]]`).
    Weight smallestWeight() const { return smallestWeight_; }
    const std::string& name() const { return name_; }

    // Items ordered by descending weight, ties broken by ascending original item index
    // (a stable sort reproduces this since the original's bubble sort has no secondary
    // tie-break -- REFACTORING_STRATEGY.md §7.3). Matches
    // `Sort_Descending_Weights(ordered_weight, number_items)` from the base file.
    const std::vector<ItemId>& descendingWeightOrder() const { return descendingWeightOrder_; }

private:
    std::string name_;
    std::vector<Weight> weights_;
    BinCapacity capacity_;
    long bestKnownSolution_;
    Weight totalWeight_;
    Weight smallestWeight_;
    std::vector<ItemId> descendingWeightOrder_;
};

} // namespace ggacgt
