#pragma once

#include "parameter_controller.hpp"
#include "schedules.hpp"

namespace ggacgt {

// The "informacion" variant: feedback-driven p_m/p_c, plus k_cs/k_ncs from the fixed
// schedules in schedules.hpp. All four are recomputed once per generation, at the same
// single point in Generation() the base variant's constant values would have been read
// from (confirmed by reading the informacion source directly -- see hh/CHECKLIST.md
// Recommendations #17), which is exactly where GgaCgtEngine::generationStep() already
// calls IParameterController::next() from.
//
// Stateful across the whole run (two running accumulators, reset only by constructing
// a fresh instance per instance/config run -- matches the original's `not_reps_acumul`
// and `acumulador_out` globals, both explicitly zeroed once per instance in main()).
//
// Formulas (original names in parens):
//   not_rep_pob = (P_size - repeatedFitnessBeforeCrossoverSelection) / P_size   [^1 in
//                 the original is a literal no-op, not reproduced as a pow() call]
//   not_reps_acumul += not_rep_pob                                              (running sum)
//   umbral = not_reps_acumul / (generation + 1)                                (running average)
//   p_m = (int)roundTo002(umbral * (P_size - B_size)) / 100
//
//   k_cs  = LinearDecaySchedule(13.5, 4.5).valueAt(generation, max_gen)
//   k_ncs = DampedCosineSchedule(7.5, 1.5).valueAt(generation, max_gen)
//
//   ri = clamp01(1 - stats.bestFullBinRatio) ^ (1 / k_cs)     [note: uses THIS
//        generation's just-computed k_cs -- an ordering dependency the original has
//        too, since k_cs is computed earlier in the same Generation() call]
//   crossAccumulated += ri                                                     (running sum)
//   p_c = clamp01(roundTo002(crossAccumulated / (generation + 1)))
class AdaptiveParameterController : public IParameterController {
public:
    AdaptiveParameterController(long pSize, float bSize, long maxGen)
        : pSize_(pSize), bSize_(bSize), maxGen_(maxGen) {}

    ParameterSet next(GenerationNumber generation, const PopulationStats& stats) override;

private:
    long pSize_;
    float bSize_;
    long maxGen_;
    float notRepsAccumulated_ = 0.0f;
    float crossAccumulated_ = 0.0f;
    LinearDecaySchedule kCsSchedule_{13.5f, 4.5f};
    DampedCosineSchedule kNcsSchedule_{7.5f, 1.5f};
};

} // namespace ggacgt
