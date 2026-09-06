#include "controlled_replacement.hpp"

namespace ggacgt {

void applyControlledReplacement(Population& population, const SelectionPools& pools,
                                 const std::vector<Chromosome>& children, long pSize, float pC) {
    long k = 0;
    for (long j = 0; static_cast<float>(j) < (pC / 2.0f * static_cast<float>(pSize) - 1.0f); ++j) {
        long targetRank = pools.randomIndividuals[static_cast<std::size_t>(k++)];
        long targetIndividual = population.orderedIndex(targetRank);
        population.at(targetIndividual) = children[static_cast<std::size_t>(j)];
    }
}

} // namespace ggacgt
