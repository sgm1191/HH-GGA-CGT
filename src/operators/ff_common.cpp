#include "ff_common.hpp"

#include <cmath>

namespace ggacgt {

void firstFitInsert(const BppInstance& instance, ItemId item, Chromosome& individual,
                     long& binI, long beginning, bool isLast) {
    const Weight w = instance.weight(item);
    const BinCapacity capacity = instance.capacity();
    const double capacityD = static_cast<double>(capacity);
    long i;

    const long totalBins = static_cast<long>(individual.bins.size());

    if (!isLast && w > capacity - individual.minBinFullness) {
        i = totalBins;
    } else {
        for (i = beginning; i < static_cast<long>(individual.bins.size()); ++i) {
            Bin& bin = individual.bins[static_cast<std::size_t>(i)];
            if (bin.fullness() + w <= capacity) {
                bin.insert(item, w);
                if (bin.fullness() == capacity) ++individual.fullBinCount;

                if (isLast) {
                    for (; i < static_cast<long>(individual.bins.size()); ++i) {
                        Bin& b2 = individual.bins[static_cast<std::size_t>(i)];
                        individual.fitness += std::pow(static_cast<double>(b2.fullness()) / capacityD, 2);
                    }
                    return;
                }
                if (bin.fullness() + instance.smallestWeight() > capacity && i == binI) {
                    ++binI;
                    individual.fitness += std::pow(static_cast<double>(bin.fullness()) / capacityD, 2);
                }
                return;
            }
            if (isLast) {
                individual.fitness += std::pow(static_cast<double>(bin.fullness()) / capacityD, 2);
            }
        }
    }

    // New bin: neither existing bin scanned (or the fast-path skip) found room.
    individual.bins.emplace_back();
    Bin& newBin = individual.bins[static_cast<std::size_t>(i)];
    newBin.insert(item, w);
    if (newBin.fullness() < individual.minBinFullness) individual.minBinFullness = newBin.fullness();
    if (isLast) {
        individual.fitness += std::pow(static_cast<double>(newBin.fullness()) / capacityD, 2);
    }
}

} // namespace ggacgt
