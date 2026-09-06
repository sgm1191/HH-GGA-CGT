#pragma once

#include <vector>

#include "types.hpp"

namespace ggacgt {

// Faithful port of the original's portable Schrage pseudo-random number generator
// (L. Schrage, "A more Portable Fortran Random Number Generator", ACM TOMS 1979;
// original authorship credited in the source to Adriana Cesario de Faria Alvim).
// Bit-for-bit identical to `randp`/`get_rand`/`get_rand_ij` so that a given seed
// reproduces the exact sequence the original binaries produce — this is required for
// characterization/golden testing (REFACTORING_STRATEGY.md §5) and for not silently
// invalidating already-published experimental results (§4.2).
class Rng {
public:
    explicit Rng(int seed) : state_(seed) {}

    // randp(): advances the generator, returns a value in (0, 1).
    float next01();

    // get_rand(): advances the generator, returns an integer in [1, size].
    int uniform(int size);

    // get_rand_ij(): advances the generator, returns an integer in [i, j].
    int uniformRange(int i, int j);

    int state() const { return state_; }

private:
    int state_;
};

// Sort_Random(): shuffles the half-open range [k, n) of `arr` in place.
//
// IMPORTANT (REFACTORING_STRATEGY.md §4.2): this is deliberately NOT a textbook
// Fisher-Yates shuffle. The original draws `k + get_rand(seed, n - k) - 1` on every
// iteration with a FIXED range width `n - k` (not the shrinking `i - k + 1` a textbook
// implementation would use), so it produces a different permutation than Fisher-Yates
// for the same RNG stream. This function reproduces the original's exact index formula.
//
// Side effect preserved from the original: `lighterWeight` is updated to the minimum
// weight touched during the shuffle. `RearrangementByPairs` (RP in the original) reads
// this afterward; most other call sites ignore it, matching the original's usage.
void shuffleRange(Rng& rng, std::vector<long>& arr, long k, long n,
                   const std::vector<Weight>& weight, Weight& lighterWeight);

// Same shuffle, without the lighter-weight tracking. Two of the original's Sort_Random
// call sites (the persistent `random_individuals`/`best_individuals` selection pools in
// Generation()) shuffle arrays of INDIVIDUAL indices, not item ids -- `weight[]` isn't
// even sized for that (individual indices can exceed numberOfItems() for a small
// instance with a large P_size), and the original's `lighter_weight` side effect from
// those two calls is provably dead: RP() unconditionally overwrites the global
// `lighter_weight` at its own start before ever reading it, so nothing downstream ever
// observes whatever those two calls left in it. This overload reproduces the same
// index-shuffle (same RNG draws, same resulting permutation) without touching a weight
// array that isn't safe to index here. See hh/CHECKLIST.md Recommendations.
void shuffleRange(Rng& rng, std::vector<long>& arr, long k, long n);

// The original threads THREE independent RNG streams through a run, all seeded from the
// same config `seed` value but diverging as soon as each is drawn from -- confirmed by
// grepping every `&seed`/`&seed_permutation`/`&seed_emptybin` call site in the base file:
//   - `seed`             -> only `randp(&seed)`, the crossover 50/50 branch coin flip.
//   - `seed_permutation` -> only inside Sort_Random (shuffleRange here).
//   - `seed_emptybin`    -> only Adaptive_Mutation_RP's empty-bin-count draw.
// Bundled here so operator call sites can't accidentally cross the streams (which would
// silently desynchronize reproducibility for a given seed).
struct RngStreams {
    Rng& coin;
    Rng& permutation;
    Rng& emptyBin;
};

} // namespace ggacgt
