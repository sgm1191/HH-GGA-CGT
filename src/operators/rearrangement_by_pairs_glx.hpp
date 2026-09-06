#pragma once

#include <vector>

#include "../core/bpp_instance.hpp"
#include "../core/chromosome.hpp"
#include "../core/rng.hpp"

namespace ggacgt {

// Rearrangement by Pairs, crossover variant (original: RP_GLX, informacion-only).
// Reinserts a crossover child's leftover free items via the same pairwise item-swap
// search as RearrangementByPairs (RP), but: (a) has FOUR EXTRA swap cases beyond RP's
// three -- ALL FOUR combinations of {F[k], F[k2]} x {p, s}, where a single free item
// replaces just one of the pair (not only both-or-neither), checked in that exact
// order after RP's three cases; (b) scans free items in ascending-weight order from
// the start instead of RP's random shuffle; and (c) reorders the leftover free-item
// pool differently before the final First-Fit sweep (large items first,
// deterministically, only the tail shuffled) instead of RP's "small enough -> shuffle
// everything, else -> sort descending" branch. See rearrangement_by_pairs_glx.cpp for
// the exact diff against RP.
//
// The original's RP_GLX also reintroduces a `new`/`free` mismatch (`delete s` from RP
// became `free(s)` here -- undefined behavior, same bug class CHANGES.md already fixed
// for the base file). Not a deliberate reproduce-vs-fix choice: this port doesn't do
// manual node allocation at all (std::vector::erase), so the bug has no equivalent to
// reproduce or fix -- it simply doesn't exist in this representation.
//
// Precondition (matches the original: only ever called from crossover when there ARE
// free items): `freeItems` must be non-empty.
class RearrangementByPairsGLX {
public:
    explicit RearrangementByPairsGLX(const BppInstance& instance) : instance_(instance) {}

    // Returns the final bin count (== individual.bins.size() afterward).
    long rearrange(Chromosome& individual, long survivingBinCount, std::vector<ItemId> freeItems,
                    Rng& permutationRng);

private:
    const BppInstance& instance_;
};

} // namespace ggacgt
