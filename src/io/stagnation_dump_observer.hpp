#pragma once

#include <fstream>
#include <string>

#include "../ga/generation_observer.hpp"

namespace ggacgt {

// Port of the informacion variant's inline stagnation-dump block in Generation():
// triggered when the current best individual has zero fully-packed bins
// (`stats.bestFullBinRatio == 0.0`, checked by the engine BEFORE that generation's own
// crossover/mutation run -- see gga_cgt_engine.cpp), dumps the best individual's full
// per-bin item breakdown plus a whole-population "fitness + full-bin count" summary to
// Details_GGA-CGT/detalle_solucion_<instanceBaseNameWithoutExtension>_conf(<configId>).txt.
//
// The original opens this file in APPEND mode every single time it triggers (unlike
// every other writer in either variant, which write once); this port opens it once,
// lazily, on the FIRST trigger (so a run where stagnation never happens never creates
// the file, matching the original) and keeps writing to the same handle for the rest
// of the run -- same observable content, without the reopen-per-trigger overhead
// (REFACTORING_STRATEGY.md §7.5's spirit, applied here too).
//
// Error handling deviates from the original on purpose: the original's fopen failure
// path prints a console error and silently continues; this throws
// std::runtime_error instead, matching every other writer in hh/'s I/O layer.
//
// The original's filename is built from the FULL instance path (with directories,
// extension stripped) rather than the basename -- the exact bug class CHANGES.md
// documented and fixed for the base file's other writers, apparently reintroduced
// here (though this one has a NULL check on fopen, unlike Mutation_reps/Ratios_bins/
// Repeated_Fitness in the same variant, so it degrades to a skipped dump + console
// message instead of a segfault). io/paths.cpp's stagnationDumpFile() uses the safe
// basename-based construction instead, consistent with the rest of hh/.
class StagnationDumpObserver : public IGenerationObserver {
public:
    StagnationDumpObserver(int configId, std::string instanceBaseName);

    // No-op: this observer only cares about onStagnationDetected().
    void onGeneration(const PopulationStats& stats, const ParameterSet& params,
                       const GenerationOperatorStats& operatorStats) override;

    bool wantsStagnationDump() const override { return true; }
    void onStagnationDetected(const Population& population, GenerationNumber generation) override;

private:
    int configId_;
    std::string instanceBaseName_;
    std::ofstream out_;
};

} // namespace ggacgt
