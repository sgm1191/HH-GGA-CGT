#pragma once

#include "bpp_instance.hpp"

namespace ggacgt {

struct LowerBoundResult {
    long L2;              // Martello-Toth L2 lower bound on the number of bins
    long largeItemCount;  // n_: items with weight > capacity/2, placed one per bin by FF-n_
};

// Faithful port of LowerBound() from the original. Deliberately deviates from the
// original in two spots where it relied on reading one element past the valid item
// range as a sentinel (`ordered_weight[number_items] = number_items; weight[number_items] = 0;`)
// to make an unguarded `while` condition safe -- here those loops get an explicit
// `index < numberOfItems` bounds check instead. This is behavior-preserving for any
// instance with strictly positive item weights (true for every BPP instance format this
// codebase reads) and avoids depending on the original's per-run global-array reuse.
LowerBoundResult computeLowerBound(const BppInstance& instance);

} // namespace ggacgt
