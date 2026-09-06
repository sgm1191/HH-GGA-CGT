#pragma once

namespace ggacgt {

// The run-level GA settings read once from one line of configurations.txt (original
// globals: P_size, max_gen, B_size, life_span, seed, save_bestSolution). p_m/p_c/k_cs/
// k_ncs are NOT here -- those come from IParameterController (ga/gga_cgt_engine.hpp).
struct GaParameters {
    long pSize = 0;
    long maxGen = 0;
    float bSize = 0.0f;
    long lifeSpan = 0;
    int seed = 0;
    bool saveBestSolution = false;
    int configId = 0;
};

} // namespace ggacgt
