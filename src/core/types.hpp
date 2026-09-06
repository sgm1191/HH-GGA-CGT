#pragma once

namespace ggacgt {

    // Strong-ish aliases replacing the original's bare `long int` / `unsigned long int`
    // globals (REFACTORING_STRATEGY.md §2, core/types.hpp). Kept as plain integer aliases
    // rather than wrapper classes: the original algorithm mixes these freely in arithmetic
    // (weight sums, capacity comparisons, fitness accumulation), and a zero-cost alias is
    // enough to document intent without fighting the arithmetic at every call site.
    using ItemId = long;          // index into a BppInstance's item arrays
    using Weight = long;          // item / bin weight (original: unsigned long int)
    using BinCapacity = long;     // bin capacity (original: unsigned long int)
    using Fitness = double;       // sum of squared bin-fullness ratios / bins used
    using GenerationNumber = long;

} // namespace ggacgt
