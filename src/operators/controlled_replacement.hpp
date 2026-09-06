#pragma once

#include <vector>

#include "../core/population.hpp"
#include "controlled_selection.hpp"

namespace ggacgt {

// Original: the "Controlled replacement for crossover" block in Generation(). Copies
// each child into the population slot of the next individual in
// `pools.randomIndividuals` (the SAME pool, walked again from index 0, that supplied
// crossover's `f2` parents -- this deliberately overwrites some of the very individuals
// that were just used as parents). The loop bound `j < p_c/2*P_size - 1` is a float
// expression in the original, reproduced with an explicit float comparison to match its
// truncation exactly; it is not necessarily equal to how many children were generated
// (that loop's bound has no `-1`), so the LAST generated child can be left uncopied --
// a latent quirk of the original, not "fixed" here (see hh/CHECKLIST.md Recommendations).
void applyControlledReplacement(Population& population, const SelectionPools& pools,
                                 const std::vector<Chromosome>& children, long pSize, float pC);

} // namespace ggacgt
