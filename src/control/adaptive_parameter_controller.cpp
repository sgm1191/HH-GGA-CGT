#include "adaptive_parameter_controller.hpp"

#include <cmath>

namespace ggacgt {

namespace {
float clamp01(float x) {
    if (x < 0.0f) return 0.0f;
    if (x > 1.0f) return 1.0f;
    return x;
}

// round_to_002: round to the nearest 0.02 via integer arithmetic (the original's
// comment: "evita precision y -lm" -- avoids precision issues and needing -lm, though
// this port links <cmath> anyway for pow/cos elsewhere).
float roundTo002(float x) {
    if (x < 0.0f) x = 0.0f;
    int steps = static_cast<int>(x * 50.0f + 0.5f);
    return static_cast<float>(steps) / 50.0f;
}
} // namespace

ParameterSet AdaptiveParameterController::next(GenerationNumber generation, const PopulationStats& stats) {
    ParameterSet ps;

    // p_m
    float notRepeat = static_cast<float>(pSize_ - stats.repeatedFitnessBeforeCrossoverSelection);
    float notRepPob = notRepeat / static_cast<float>(pSize_);
    notRepsAccumulated_ += notRepPob;
    float umbral = notRepsAccumulated_ / static_cast<float>(generation + 1);
    // Original: `p_m = (float)((int)round_to_002(umbral*(P_size-B_size)))/100;` -- the
    // (int) cast truncates round_to_002's result, which only matters because the
    // argument here is on the scale of P_size (not [0,1]) -- see the header comment.
    ps.pM = static_cast<float>(static_cast<int>(roundTo002(umbral * (static_cast<float>(pSize_) - bSize_)))) / 100.0f;

    // k_cs / k_ncs
    ps.kCs = kCsSchedule_.valueAt(generation, maxGen_);
    ps.kNcs = kNcsSchedule_.valueAt(generation, maxGen_);

    // p_c (depends on this generation's just-computed k_cs, and on the best
    // individual's full-bin ratio from BEFORE this generation's own crossover/mutation
    // -- both matching the original's read/compute order in Generation()).
    float proporcionInversa = 1.0f - static_cast<float>(stats.bestFullBinRatio);
    float ri = std::pow(clamp01(proporcionInversa), 1.0f / ps.kCs);
    crossAccumulated_ += ri;
    float pC = crossAccumulated_ / static_cast<float>(generation + 1);
    ps.pC = clamp01(roundTo002(pC));

    return ps;
}

} // namespace ggacgt
