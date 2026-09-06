#pragma once

#include <vector>

#include "chromosome.hpp"

namespace ggacgt {

// A population of P_size chromosomes, sized to the actual configured P_size rather
// than the original's worst-case `SOLUTION population[P_size_MAX][ATTRIBUTES]`
// (REFACTORING_STRATEGY.md §7.2).
class Population {
public:
    explicit Population(long size) : individuals_(static_cast<std::size_t>(size)),
                                      orderedIndices_(static_cast<std::size_t>(size)) {
        for (long i = 0; i < size; ++i) orderedIndices_[static_cast<std::size_t>(i)] = i;
    }

    long size() const { return static_cast<long>(individuals_.size()); }

    Chromosome& at(long index) { return individuals_[static_cast<std::size_t>(index)]; }
    const Chromosome& at(long index) const { return individuals_[static_cast<std::size_t>(index)]; }

    // The original's `ordered_population[rank]`: index of the individual holding the
    // `rank`-th place in the current ascending-fitness order.
    long orderedIndex(long rank) const { return orderedIndices_[static_cast<std::size_t>(rank)]; }
    const std::vector<long>& orderedIndices() const { return orderedIndices_; }

    // Faithful port of Sort_Ascending_IndividualsFitness: sorts `orderedIndices_` by
    // ascending fitness AND, as an inseparable side effect, relocates every
    // exact-fitness duplicate to the front of the order as it's found
    // (REFACTORING_STRATEGY.md §4.1). This is NOT the same as a bare
    // `std::stable_sort` -- the duplicate relocation changes which individuals occupy
    // the low ranks, which the original's controlled selection depends on. Returns the
    // duplicate count (the original's `repeated_fitness`).
    long sortByFitnessAndCountDuplicates();

private:
    std::vector<Chromosome> individuals_;
    std::vector<long> orderedIndices_;
};

} // namespace ggacgt
