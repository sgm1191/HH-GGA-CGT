#pragma once

#include "operator_interfaces.hpp"

namespace ggacgt {

// Gene-Level Crossover with FFD reinsertion (original: Gene_Level_Crossover_FFD).
// For each of parent1's bins (in a randomized, then fullness/item-count-descending
// order), a 50/50 coin flip decides whether parent2's or parent1's corresponding bin is
// offered to the child first; a bin is only actually transmitted if none of its items
// are already in the child. Items left unplaced afterward are reinserted First-Fit in
// descending-weight order.
class GeneLevelCrossoverFFD : public ICrossover {
public:
    explicit GeneLevelCrossoverFFD(const BppInstance& instance) : instance_(instance) {}

    Chromosome cross(const Chromosome& parent1, const Chromosome& parent2, RngStreams& rng,
                      GenerationNumber birthGeneration, CrossoverCounters& counters) override;

private:
    const BppInstance& instance_;
};

} // namespace ggacgt
