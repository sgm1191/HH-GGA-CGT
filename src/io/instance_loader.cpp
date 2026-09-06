#include "instance_loader.hpp"

#include <cmath>
#include <fstream>
#include <stdexcept>
#include <vector>

#include "paths.hpp"

namespace ggacgt {

namespace {
void skipLine(std::ifstream& in) {
    std::string discard;
    std::getline(in, discard);
}
} // namespace

BppInstance loadInstance(const std::string& path) {
    std::ifstream in(path);
    if (!in) throw std::runtime_error("There is no data file ==> [" + path + "]");

    skipLine(in); // "//Number of items"
    long numberItems = 0;
    in >> numberItems;

    skipLine(in); // rest of the numberItems line
    skipLine(in); // "//Bin Capacity"
    long double binCapacity1 = 0;
    in >> binCapacity1;

    skipLine(in);
    skipLine(in); // "//Optimal Solution"
    long bestSolution = 0;
    in >> bestSolution;

    skipLine(in);
    skipLine(in); // "//Weight of Item"

    std::vector<long double> weight1(static_cast<std::size_t>(numberItems));
    std::vector<Weight> weight(static_cast<std::size_t>(numberItems));
    long double totalAccumulatedAux = 0;
    bool fractional = false;

    for (long k = 0; k < numberItems; ++k) {
        in >> weight1[static_cast<std::size_t>(k)];
        weight[static_cast<std::size_t>(k)] = static_cast<Weight>(weight1[static_cast<std::size_t>(k)]);
        totalAccumulatedAux += weight1[static_cast<std::size_t>(k)];
        if (!fractional &&
            weight1[static_cast<std::size_t>(k)] / static_cast<long double>(weight[static_cast<std::size_t>(k)]) >
                1.0L) {
            fractional = true;
        }
    }

    long double totalAccumulatedWeight = 0;
    if (fractional) {
        // Some benchmark families store item weights as fractions of the bin capacity
        // rather than as integers; reconstruct integer weights by rescaling.
        for (long k = 0; k < numberItems; ++k) {
            weight[static_cast<std::size_t>(k)] =
                static_cast<Weight>(std::ceil(weight1[static_cast<std::size_t>(k)] * binCapacity1 - 0.5L));
            totalAccumulatedWeight += weight[static_cast<std::size_t>(k)];
        }
        binCapacity1 *= binCapacity1;
    } else {
        for (long k = 0; k < numberItems; ++k) totalAccumulatedWeight += weight[static_cast<std::size_t>(k)];
    }

    BinCapacity binCapacity = static_cast<BinCapacity>(binCapacity1);

    if (fractional) {
        long double expected =
            std::ceil(totalAccumulatedAux * std::sqrt(static_cast<long double>(binCapacity)) - 0.5L);
        if (static_cast<long long>(totalAccumulatedWeight) != static_cast<long long>(expected)) {
            throw std::runtime_error("Error loading weights for instance: " + path);
        }
    }

    return BppInstance(paths::baseName(path), weight, binCapacity, bestSolution);
}

} // namespace ggacgt
