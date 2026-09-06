#pragma once

#include <fstream>
#include <string>

#include "../ga/generation_observer.hpp"

namespace ggacgt {

// Port of the informacion variant's Mutation_reps/gene_detail_muta(<configId>)_<baseName>
// and Ratios_bins/gene_detail_ratio(<configId>)_<baseName> writers -- both opened once
// at construction (like GenerationTraceLogger, fixing the same §7.5 reopen-every-
// generation pattern) and written from the same onGeneration() call, since the
// original writes them back-to-back from the same per-generation state.
//
// Formulas (see hh/CHECKLIST.md Recommendations #17 for the full derivation from the
// source): both files' denominators are `p_m*P_size`/`p_c*P_size` -- an ESTIMATE of how
// many mutation/crossover calls happened this generation, NOT the actual count
// (`GenerationOperatorStats::mutationCalls`/`crossoverCalls` are computed but
// deliberately unused for these denominators, matching the original exactly).
//
// Two known-buggy original values are NOT reproduced (see operator_interfaces.hpp's
// CrossoverStats/MutationStats doc comments for why each is unrecoverable/fixed):
//   - Mutation_reps' "Complete_Bins_M" column is always 0 here (original:
//     arr_acul_muta[5]/m_bins, read uninitialized).
//   - Mutation_reps' "Proportion_bins_full_c" column uses the type-correct ratio sum
//     instead of the original's out-of-bounds, int-truncated arr_acul_cruza[5].
//
// Ratios_bins' three "_M" and three "_C" columns are pairwise numerically identical in
// the original (best_ratio_c/worst_ratio_c/average_ratio_c are computed via formulas
// that turn out identical to best_ratio_m/worst_ratio_m/average_ratio_m -- confirmed
// against the source, not assumed), so this port computes each pair once.
class AdaptiveTraceLogger : public IGenerationObserver {
public:
    AdaptiveTraceLogger(int configId, const std::string& instanceBaseName, long pSize);

    void onGeneration(const PopulationStats& stats, const ParameterSet& params,
                       const GenerationOperatorStats& operatorStats) override;

private:
    long pSize_;
    std::ofstream mutationReps_;
    std::ofstream ratiosBins_;
};

} // namespace ggacgt
