#pragma once

#include "operator_interfaces.hpp"
#include "rearrangement_by_pairs.hpp"

namespace ggacgt {

// Adaptive Mutation via Rearrangement by Pairs (original: Adaptive_Mutation_RP).
// Removes a data-driven number of the individual's lightest bins (more when `k` is
// small, i.e. k_cs for clones vs k_ncs for non-clones -- the caller picks which),
// frees their items, and hands everything to RearrangementByPairs to reinsert.
class AdaptiveMutationRP : public IMutation {
public:
    explicit AdaptiveMutationRP(const BppInstance& instance) : instance_(instance), rp_(instance) {}

    void mutate(Chromosome& individual, float k, RngStreams& rng) override;

    bool hasLastCallStats() const override { return true; }
    MutationStats lastMutationStats() const override { return lastStats_; }

private:
    const BppInstance& instance_;
    RearrangementByPairs rp_;
    MutationStats lastStats_;
};

} // namespace ggacgt
