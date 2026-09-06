// AdaptiveParameterController tests, as pure functions of (generation, PopulationStats)
// where feasible. The expected values here are not invented -- they're the exact
// generation-0 values this session's golden-test debugging captured from the real
// `gga-cgt-info` binary (Hard28_BPP14.txt, P_size=15, B_size=0.2, max_gen=12, seed=555):
// p_m=0.14, k_cs=13.5, k_ncs=7.5, p_c=1.0 -- see hh/CHECKLIST.md Recommendations for
// the full derivation and the golden-test transcript this came from.
#include <cmath>
#include <cstdio>
#include <cstdlib>

#include "control/adaptive_parameter_controller.hpp"

using namespace ggacgt;

namespace {

void expectNear(float actual, float expected, float tol, const char* what) {
    if (std::fabs(actual - expected) > tol) {
        std::fprintf(stderr, "FAILED: %s (expected %f, got %f)\n", what, expected, actual);
        std::exit(1);
    }
}

void testGenerationZero() {
    AdaptiveParameterController controller(/*pSize=*/15, /*bSize=*/0.2f, /*maxGen=*/12);

    PopulationStats stats;
    stats.repeatedFitnessBeforeCrossoverSelection = 0;
    stats.bestFullBinRatio = 0.079365; // 5/63, the real run's generation-0 best individual

    ParameterSet ps = controller.next(/*generation=*/0, stats);

    expectNear(ps.kCs, 13.5f, 1e-4f, "k_cs at generation 0 == schedule start (13.5)");
    expectNear(ps.kNcs, 7.5f, 1e-4f, "k_ncs at generation 0 == schedule start (7.5)");
    expectNear(ps.pM, 0.14f, 1e-4f, "p_m at generation 0 with repeatedFitness=0");
    expectNear(ps.pC, 1.0f, 1e-4f, "p_c at generation 0 (clamped/rounded to 1.0)");
}

// p_m's formula is a pure function of (P_size, B_size, repeatedFitness, generation) via
// a running average -- when repeatedFitness is 0 every generation (as in the golden
// case above), the running average of a constant stays constant, so p_m should stay
// fixed across generations too. Verified against the real binary's output, which
// printed p_m=0.14 for every one of the 12 generations in that run.
void testPmStaysConstantWhenNeverRepeated() {
    AdaptiveParameterController controller(/*pSize=*/15, /*bSize=*/0.2f, /*maxGen=*/12);
    PopulationStats stats;
    stats.repeatedFitnessBeforeCrossoverSelection = 0;

    for (long generation = 0; generation < 5; ++generation) {
        stats.bestFullBinRatio = 0.3; // arbitrary, doesn't affect p_m
        ParameterSet ps = controller.next(generation, stats);
        expectNear(ps.pM, 0.14f, 1e-4f, "p_m stays constant across generations when repeatedFitness is always 0");
    }
}

// k_cs/k_ncs are pure functions of (generation, maxGen) alone -- same values regardless
// of population state, matching schedules.hpp's LinearDecaySchedule/DampedCosineSchedule
// directly (already unit-tested there; this just confirms the controller wires them
// through unchanged).
void testKScheduleIndependentOfStats() {
    AdaptiveParameterController controller(/*pSize=*/20, /*bSize=*/0.1f, /*maxGen=*/100);
    PopulationStats lowRatio;
    lowRatio.bestFullBinRatio = 0.0;
    PopulationStats highRatio;
    highRatio.bestFullBinRatio = 1.0;

    ParameterSet psLow = controller.next(50, lowRatio);
    // Reset accumulators via a fresh controller so the two calls are independent.
    AdaptiveParameterController controller2(20, 0.1f, 100);
    ParameterSet psHigh = controller2.next(50, highRatio);

    expectNear(psLow.kCs, psHigh.kCs, 1e-4f, "k_cs at the same generation is independent of population stats");
    expectNear(psLow.kNcs, psHigh.kNcs, 1e-4f, "k_ncs at the same generation is independent of population stats");
}

} // namespace

int main() {
    testGenerationZero();
    testPmStaysConstantWhenNeverRepeated();
    testKScheduleIndependentOfStats();
    std::printf("test_adaptive_parameter_controller: all checks passed\n");
    return 0;
}
