#include "bpp_instance.hpp"

#include <algorithm>
#include <numeric>

namespace ggacgt {

BppInstance::BppInstance(std::string name, std::vector<Weight> weights, BinCapacity capacity,
                          long bestKnownSolution)
    : name_(std::move(name)),
      weights_(std::move(weights)),
      capacity_(capacity),
      bestKnownSolution_(bestKnownSolution),
      totalWeight_(std::accumulate(weights_.begin(), weights_.end(), Weight{0})) {
    descendingWeightOrder_.resize(weights_.size());
    std::iota(descendingWeightOrder_.begin(), descendingWeightOrder_.end(), ItemId{0});
    std::stable_sort(descendingWeightOrder_.begin(), descendingWeightOrder_.end(),
                      [this](ItemId a, ItemId b) { return weights_[a] > weights_[b]; });
    smallestWeight_ = weights_[descendingWeightOrder_.back()];
}

} // namespace ggacgt
