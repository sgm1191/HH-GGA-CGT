#pragma once

#include "../core/population_stats.hpp"
#include "../core/types.hpp"
#include "parameter_set.hpp"

namespace ggacgt {

// This is the actual de-duplication point between "base" and "informacion"
// (REFACTORING_STRATEGY.md §2, §6 phase 4): GgaCgtEngine calls next() exactly once per
// generation and uses whatever ParameterSet comes back, without knowing or caring
// whether it's a constant or a feedback-driven computation.
class IParameterController {
public:
    virtual ~IParameterController() = default;
    virtual ParameterSet next(GenerationNumber generation, const PopulationStats& stats) = 0;
};

} // namespace ggacgt
