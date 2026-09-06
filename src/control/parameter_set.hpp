#pragma once

namespace ggacgt {

// The four per-generation GA parameters the original varies between "base" (constant,
// read once from configurations.txt) and "informacion" (adaptive, feedback-driven).
// Original globals: p_m, p_c, k_ncs, k_cs.
struct ParameterSet {
    float pM = 0.0f;
    float pC = 0.0f;
    float kCs = 0.0f;
    float kNcs = 0.0f;
};

} // namespace ggacgt
