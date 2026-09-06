#pragma once

#include <cmath>

namespace ggacgt {

// Reusable pieces of the "informacion" variant's k_cs/k_ncs adaptive schedules
// (REFACTORING_STRATEGY.md §2, control/schedules.hpp), extracted from the informacion
// source so they're unit-testable independently of the GA (unlike the original, where
// they're inline expressions recomputed every Generation() call). Confirmed against
// the informacion source (`GGA-CGT_cruzaFFDStepaphanie - informacion.CPP`):
//
//   k_cs  = r0 + (r1 - r0) * (generation / max_gen)                     [linear decay]
//   k_ncs = clamp((r0_ncs + (r1_ncs-r0_ncs)*(generation/max_gen))
//                    * cos(15*PI/max_gen * generation)^2,
//                 r1_ncs, r0_ncs)                                        [damped cosine]
//
// with the constants r0=13.5, r1=4.5, r0_ncs=7.5, r1_ncs=1.5 hardcoded in the original
// -- kept as defaulted constructor parameters here instead, so a caller can use the
// original's exact values without repeating the magic numbers, while still allowing
// them to be overridden/tested independently.

class LinearDecaySchedule {
public:
    LinearDecaySchedule(float start, float end) : start_(start), end_(end) {}

    float valueAt(long generation, long maxGen) const {
        return start_ + (end_ - start_) * (static_cast<float>(generation) / static_cast<float>(maxGen));
    }

private:
    float start_;
    float end_;
};

class DampedCosineSchedule {
public:
    DampedCosineSchedule(float start, float end) : start_(start), end_(end) {}

    float valueAt(long generation, long maxGen) const {
        constexpr double kPi = 3.14159265358979323846;
        const double maxGenD = static_cast<double>(maxGen);
        const double envelope =
            static_cast<double>(start_) +
            (static_cast<double>(end_) - static_cast<double>(start_)) * (static_cast<double>(generation) / maxGenD);
        const double angle = (15.0 * kPi / maxGenD) * static_cast<double>(generation);
        const double c = std::cos(angle);
        double value = envelope * c * c;
        // Original clamps to [end_, start_] (i.e. [r1_ncs, r0_ncs]) after the fact,
        // since the cosine envelope can overshoot both bounds.
        if (value > start_) value = start_;
        if (value < end_) value = end_;
        return static_cast<float>(value);
    }

private:
    float start_;
    float end_;
};

} // namespace ggacgt
