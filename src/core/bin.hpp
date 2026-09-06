#pragma once

#include <vector>

#include "types.hpp"

namespace ggacgt {

// A single bin's contents: a contiguous list of item ids plus its cached fullness.
//
// Replaces the original's per-bin `linked_list` (linked_list.h), which allocated one
// heap `node` per item and required node-by-node cloning/teardown for every crossover
// and mutation, on essentially the whole population every generation
// (REFACTORING_STRATEGY.md §7.1 -- the single biggest identified perf win). A
// `std::vector<ItemId>` clones with one `operator=`/copy, needs no per-item
// `new`/`delete`, and iterates cache-friendly instead of chasing `next` pointers.
class Bin {
public:
    void insert(ItemId item, Weight w) {
        items_.push_back(item);
        fullness_ += w;
    }

    void clear() {
        items_.clear();
        fullness_ = 0;
    }

    const std::vector<ItemId>& items() const { return items_; }
    std::vector<ItemId>& items() { return items_; }

    long itemCount() const { return static_cast<long>(items_.size()); }
    Weight fullness() const { return fullness_; }

    // RearrangementByPairs (port of RP) recomputes a bin's fullness explicitly as it
    // swaps items in/out rather than through insert(), so the setter is exposed rather
    // than re-deriving fullness by summing items() on every call.
    void setFullness(Weight f) { fullness_ = f; }

private:
    std::vector<ItemId> items_;
    Weight fullness_ = 0;
};

} // namespace ggacgt
