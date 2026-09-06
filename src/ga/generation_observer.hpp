#pragma once

#include "../control/parameter_set.hpp"
#include "../core/population.hpp"
#include "../core/population_stats.hpp"
#include "../operators/operator_interfaces.hpp"

namespace ggacgt {

// Accumulated per-call operator diagnostics for one generation (original:
// `arr_acul_cruza`/`arr_acul_muta`, summed across every crossover/mutation call within
// one Generation() call -- see CrossoverStats/MutationStats in
// operators/operator_interfaces.hpp for the per-call field meanings; these are the
// same fields, field-wise-summed by the engine across the generation). Always
// zero-valued for operators that don't implement ICrossover::hasLastCallStats()/
// IMutation::hasLastCallStats() -- i.e. always zero for the base variant's operators,
// so this is a no-op addition for GenerationTraceLogger/the base composition root.
struct GenerationOperatorStats {
    CrossoverStats crossover;
    long crossoverCalls = 0;
    MutationStats mutation;
    long mutationCalls = 0;
};

// Replaces the original's inline `fopen`/`fprintf` calls scattered through Generation()
// (Repeated_Fitness/, and informacion-only Mutation_reps/ + Ratios_bins/). The engine
// notifies every attached observer once per generation with the same PopulationStats,
// ParameterSet, and GenerationOperatorStats it just used, no file I/O of its own.
class IGenerationObserver {
public:
    virtual ~IGenerationObserver() = default;
    virtual void onGeneration(const PopulationStats& stats, const ParameterSet& params,
                               const GenerationOperatorStats& operatorStats) = 0;

    // Optional (default no-op, so this is a non-invasive addition -- same rationale as
    // ICrossover/IMutation's optional stats methods): called once per generation,
    // BEFORE that generation's crossover/mutation run, only when the engine's
    // stagnation trigger fires (informacion's `best_ratio_m == 0` check -- the current
    // best individual has zero fully-packed bins). Takes the live Population by const&
    // rather than a copy, since this is rare and the engine shouldn't pay for a
    // population-wide snapshot on every generation just in case an observer wants one.
    virtual bool wantsStagnationDump() const { return false; }
    virtual void onStagnationDetected(const Population& population, GenerationNumber generation) { (void)population; (void)generation; }
};

} // namespace ggacgt
