#include "controlled_selection.hpp"

namespace ggacgt {

void reshuffleSelectionPools(SelectionPools& pools, long pSize, float bSize, float pC, Rng& permutationRng) {
    long randomPoolSize = static_cast<long>(static_cast<int>(pSize - static_cast<int>(static_cast<float>(pSize) * bSize)));
    shuffleRange(permutationRng, pools.randomIndividuals, 0, randomPoolSize);

    long bestPoolStart = static_cast<long>((1.0f - pC) * static_cast<float>(pSize));
    shuffleRange(permutationRng, pools.bestIndividuals, bestPoolStart, pSize);
}

} // namespace ggacgt
