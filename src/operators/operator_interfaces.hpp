#pragma once

#include "../core/bpp_instance.hpp"
#include "../core/chromosome.hpp"
#include "../core/rng.hpp"

namespace ggacgt {

// GgaCgtEngine depends only on these interfaces (REFACTORING_STRATEGY.md §2,
// operators/operator_interfaces.hpp) -- no globals, no file I/O.

class IInitializer {
public:
    virtual ~IInitializer() = default;
    // Builds one new chromosome from scratch (original: FF_n_).
    virtual Chromosome generate(Rng& permutationRng, GenerationNumber birthGeneration) = 0;
};

// Original: COUNT_DELETE_BINS / COUNT_DELETE_ITEMS -- run-scoped counters of bins/items
// rejected by Used_Items() because they'd duplicate an item already transmitted to the
// child. Reported (accumulated across every crossover call for the whole run) in the
// Solutions_GGA-CGT/*.txt summary line; not used by any GA decision.
struct CrossoverCounters {
    long deletedBins = 0;
    long deletedItems = 0;
};

// The informacion variant's per-call diagnostics for one crossover, accumulated across
// a generation into `Mutation_reps/*.txt`'s "*_c" columns (original: `arr_acul_cruza`,
// filled via an `int *arr` out-parameter the informacion variant adds to
// Gene_Level_Crossover_FFD's signature -- confirmed by reading the informacion source
// directly, not the base file, so `GeneLevelCrossoverFFD` never touches this).
//
// Two fields deliberately DIVERGE from a literal port of `arr_acul_cruza` (documented
// here, not silently): the original's `int arr_acul_cruza[5]` is a 5-element array, but
// the informacion source WRITES to `arr[5]` too (`arr[5] = arr[5] + ratio_c;`) -- a
// stack buffer overflow (reads/writes one element past the array), so there is no
// "correct" value to reproduce for that slot; and `ratio_c` itself is a `float` ratio
// (`counter_full / k2`, fraction of transmitted bins that were exactly full) being
// stored into an `int*`, truncating it to 0 or 1 almost always. `fullBinRatio` below is
// typed and computed as the evidently-intended float ratio instead of reproducing
// either bug.
struct CrossoverStats {
    long partialBins = 0;        // arr[0]: k2, transmitted-bin count before reinsertion
    long eliminatedItems = 0;    // arr[1]: k, leftover/free item count
    long generatedPairSize = 0;  // arr[2]: max(parent1 bins, parent2 bins)
    long parentsFullBins = 0;    // arr[3]: sum of both parents' fullBinCount
    long parentsBinsUsed = 0;    // arr[4]: sum of both parents' binsUsed
    double fullBinRatio = 0.0;   // arr[5] (fixed, see above): fraction of transmitted bins at exact capacity
};

class ICrossover {
public:
    virtual ~ICrossover() = default;
    // Original: Gene_Level_Crossover_FFD.
    virtual Chromosome cross(const Chromosome& parent1, const Chromosome& parent2, RngStreams& rng,
                              GenerationNumber birthGeneration, CrossoverCounters& counters) = 0;

    // Optional per-call diagnostics for the informacion variant's Mutation_reps
    // logging (REFACTORING_STRATEGY.md-style: the base variant's GeneLevelCrossoverFFD
    // doesn't override these, so this is a zero-cost, non-invasive addition to the
    // interface rather than a breaking change to it). Valid only immediately after the
    // most recent cross() call.
    virtual bool hasLastCallStats() const { return false; }
    virtual CrossoverStats lastCrossoverStats() const { return {}; }
};

// The informacion variant's per-call diagnostics for one mutation (original:
// `arr_acul_muta`, filled via an `int *arr` out-parameter the informacion variant adds
// to Adaptive_Mutation_RP's signature).
//
// `arr[5]` (`m_bins`) is DELIBERATELY OMITTED here: the informacion source declares
// `long int m_bins;` with no initializer and never assigns it before
// `arr[5] = arr[5] + m_bins;` -- an uninitialized-read, not a computable quantity with
// any recoverable intent (unlike CrossoverStats::fullBinRatio above, where the
// original's intent was clear despite the bug). There is nothing to port for that slot.
struct MutationStats {
    long completeBins = 0;    // arr[0]: bins already at exact capacity before removal
    long incompleteBins = 0;  // arr[1]: bins not yet at capacity before removal
    long eliminatedBins = 0;  // arr[2]: number of bins removed this call
    long survivingBins = 0;   // arr[3]: bins remaining after removal (before RP reinsertion)
    long freeItems = 0;       // arr[4]: items freed by the removed bins
};

class IMutation {
public:
    virtual ~IMutation() = default;
    // Mutates `individual` in place, matching Adaptive_Mutation_RP's semantics.
    // `k` is k_cs or k_ncs depending on whether the individual is a clone (original:
    // the `is_cloned` flag only ever affected which `k` the caller passed in --
    // Adaptive_Mutation_RP's body doesn't otherwise branch on it -- so it is not part
    // of this interface).
    virtual void mutate(Chromosome& individual, float k, RngStreams& rng) = 0;

    // Optional per-call diagnostics, same rationale as ICrossover's above. Valid only
    // immediately after the most recent mutate() call.
    virtual bool hasLastCallStats() const { return false; }
    virtual MutationStats lastMutationStats() const { return {}; }
};

} // namespace ggacgt
