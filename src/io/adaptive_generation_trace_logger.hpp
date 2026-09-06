#pragma once

#include <fstream>
#include <string>

#include "../ga/generation_observer.hpp"

namespace ggacgt {

// The informacion variant's own Repeated_Fitness writer -- a sibling of
// GenerationTraceLogger rather than a reuse of it, because the informacion source
// genuinely differs in two ways (confirmed against the source, not assumed):
//   1. It appends a trailing population-average-fitness column ("prom",
//      PopulationStats::averageFitness) the base file's header doesn't have.
//   2. Its "worst"/"gen_worst"/"dif_worst"/"id_worst" columns use
//      `ordered_population[repeated_fitness]`, not `ordered_population[0]` like the
//      base file -- see PopulationStats::worstFitnessNonDuplicate's doc comment.
// "best" uses the same `ordered_population[P_size-1]` convention as the base file in
// both variants.
//
// Unlike the base file's Repeated_Fitness write, the informacion source's version of
// this fprintf call does NOT have the base file's misplaced-parenthesis/UB bug
// (Recommendations #1) -- its `id_worst`/`apt_prom_gen` arguments are passed correctly
// in the source. Nothing to fix here, just to port.
class AdaptiveGenerationTraceLogger : public IGenerationObserver {
public:
    AdaptiveGenerationTraceLogger(int configId, const std::string& instanceBaseName);

    void onGeneration(const PopulationStats& stats, const ParameterSet& params,
                       const GenerationOperatorStats& operatorStats) override;

private:
    std::ofstream out_;
};

} // namespace ggacgt
