// Schedule tests, per REFACTORING_STRATEGY.md §5: "LinearDecaySchedule(13.5, 4.5)" and
// "DampedCosineSchedule(7.5, 1.5, 15pi)" tested purely as numeric functions of
// (generation, max_gen), independent of the GA.
#include <cmath>
#include <cstdio>
#include <cstdlib>

#include "control/schedules.hpp"

using namespace ggacgt;

namespace {

void expectNear(float actual, float expected, float tol, const char* what) {
    if (std::fabs(actual - expected) > tol) {
        std::fprintf(stderr, "FAILED: %s (expected %f, got %f)\n", what, expected, actual);
        std::exit(1);
    }
}

void testLinearDecay() {
    LinearDecaySchedule kCs(13.5f, 4.5f);
    expectNear(kCs.valueAt(0, 100), 13.5f, 1e-4f, "k_cs at generation 0 == r0");
    expectNear(kCs.valueAt(100, 100), 4.5f, 1e-4f, "k_cs at generation == max_gen == r1");
    expectNear(kCs.valueAt(50, 100), 9.0f, 1e-4f, "k_cs at the midpoint == the arithmetic mean of r0,r1");
}

} // namespace

int main() {
    testLinearDecay();

    // DampedCosineSchedule(7.5, 1.5): at generation 0 the cosine envelope is cos(0)^2 == 1,
    // so the value equals the envelope's starting point r0_ncs == 7.5 exactly (clamped
    // at that ceiling, matching the original's post-hoc clamp).
    DampedCosineSchedule kNcs(7.5f, 1.5f);
    expectNear(kNcs.valueAt(0, 100), 7.5f, 1e-4f, "k_ncs at generation 0 == r0_ncs (cos^2(0) == 1)");

    // The schedule is clamp()ed into [end, start] = [1.5, 7.5] for every generation --
    // the cosine factor can drive it below r1_ncs or (via the envelope alone, near
    // max_gen) it could in principle exceed bounds; both ends must hold everywhere.
    for (long g = 0; g <= 100; g += 5) {
        float v = kNcs.valueAt(g, 100);
        if (v < 1.5f - 1e-4f || v > 7.5f + 1e-4f) {
            std::fprintf(stderr, "FAILED: k_ncs out of [1.5, 7.5] at generation %ld: %f\n", g, v);
            return 1;
        }
    }

    std::printf("test_schedules: all checks passed\n");
    return 0;
}
