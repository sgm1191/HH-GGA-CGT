#include "population.hpp"

#include <utility>

namespace ggacgt {

long Population::sortByFitnessAndCountDuplicates() {
    // Verbatim port of Sort_Ascending_IndividualsFitness's bubble-sort-with-relocation:
    // a normal ascending bubble sort, except that whenever two adjacent individuals
    // (in the current order) have EXACTLY equal fitness, the later one is swapped back
    // to position `i2` (the front of the not-yet-relocated region) instead of being
    // left in place, and `i2` advances. `i2`'s final value is the duplicate count.
    long k = size() - 1;
    long i2 = 0;
    bool changed = true;
    while (changed) {
        changed = false;
        for (long i = i2; i < k; ++i) {
            Fitness a = individuals_[static_cast<std::size_t>(orderedIndices_[static_cast<std::size_t>(i)])].fitness;
            Fitness b = individuals_[static_cast<std::size_t>(orderedIndices_[static_cast<std::size_t>(i + 1)])].fitness;
            if (a > b) {
                std::swap(orderedIndices_[static_cast<std::size_t>(i)],
                          orderedIndices_[static_cast<std::size_t>(i + 1)]);
                changed = true;
            } else if (a == b) {
                std::swap(orderedIndices_[static_cast<std::size_t>(i)],
                          orderedIndices_[static_cast<std::size_t>(i2)]);
                ++i2;
            }
        }
        --k;
    }
    return i2;
}

} // namespace ggacgt
