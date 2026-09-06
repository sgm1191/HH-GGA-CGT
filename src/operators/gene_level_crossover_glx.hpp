#pragma once

#include "operator_interfaces.hpp"
#include "rearrangement_by_pairs_glx.hpp"

namespace ggacgt {

// The informacion variant's crossover (original: the informacion source's own
// Gene_Level_Crossover_FFD, which takes an extra `int *arr` parameter and reinserts
// leftover items via RP_GLX instead of plain First-Fit). Shares the bin-transmission
// loop's *shape* with the base variant's GeneLevelCrossoverFFD (same 50/50 coin flip,
// same shuffled-then-fullness/item-count-sorted bin order, same Used_Items
// duplicate-rejection) via operators/gene_level_crossover_common.hpp, but is a
// genuinely separate implementation, not a thin subclass: the informacion source
// tracks an extra `counter_full` (bins transmitted at exact capacity) inline in the
// transmission loop itself, and reinserts leftovers with RearrangementByPairsGLX
// instead of firstFitInsert. Reports CrossoverStats for the informacion loggers.
class GeneLevelCrossoverFFDGLX : public ICrossover {
public:
    explicit GeneLevelCrossoverFFDGLX(const BppInstance& instance) : instance_(instance), rpGlx_(instance) {}

    Chromosome cross(const Chromosome& parent1, const Chromosome& parent2, RngStreams& rng,
                      GenerationNumber birthGeneration, CrossoverCounters& counters) override;

    bool hasLastCallStats() const override { return true; }
    CrossoverStats lastCrossoverStats() const override { return lastStats_; }

private:
    const BppInstance& instance_;
    RearrangementByPairsGLX rpGlx_;
    CrossoverStats lastStats_;
};

} // namespace ggacgt
