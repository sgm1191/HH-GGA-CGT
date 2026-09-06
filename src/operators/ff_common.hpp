#pragma once

#include "../core/bpp_instance.hpp"
#include "../core/chromosome.hpp"

namespace ggacgt {

// First-Fit item placement, shared by FFInitializer, GeneLevelCrossoverFFD's free-item
// reinsertion, and RearrangementByPairs's tail reinsertion -- exactly as the original's
// single `FF()` was called from `FF_n_`, `Gene_Level_Crossover_FFD`, and `RP`
// (REFACTORING_STRATEGY.md §2 groups these under one `operators/ff_common`).
//
// This is a near-literal port, NOT a simplified rewrite: `FF()` in the original folds
// the population's fitness accumulation (`sum of (fullness/capacity)^2` across all
// bins) into the placement scan itself, spread across many calls, so that a bin's
// contribution is added to `fitness` exactly once, at whichever call happens to "close"
// that bin off. Rewriting this as a separate post-pass would still compute the same
// mathematical total, but in a different floating-point summation ORDER -- floating
// point addition isn't associative, so that would risk breaking bit-exact agreement
// with the original binaries during golden testing (REFACTORING_STRATEGY.md §5). The
// accumulation mechanism is therefore preserved exactly; only the storage (vector-based
// Bin/Chromosome instead of raw arrays + linked lists) changes.
//
// `binI` is the original's global `bin_i`: a search-cursor optimization that, once a
// bin can no longer fit even the globally smallest item, is advanced past that bin so
// future calls don't rescan it. It is explicit per-call state here (the original resets
// it at the start of every placement pass: `bin_i = 0` in FF_n_/crossover, `bin_i = b`
// in RP), passed by reference to thread it across a whole placement loop.
//
// `beginning` is the first bin index to scan (original: FF's `beginning` param).
// `isLast` marks the final item of a placement pass -- when true, any bin fitness
// contribution not yet accumulated by an earlier call is swept in now (original: FF's
// `is_last` param and its two "finish the sum" branches).
void firstFitInsert(const BppInstance& instance, ItemId item, Chromosome& individual,
                     long& binI, long beginning, bool isLast);

} // namespace ggacgt
