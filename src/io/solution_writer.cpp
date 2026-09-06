#include "solution_writer.hpp"

#include <fstream>
#include <iomanip>
#include <stdexcept>
#include <vector>

#include "paths.hpp"

namespace ggacgt {

void writeSolutionDetail(int configId, const std::string& instancePath, const BppInstance& instance, long L2,
                          const Chromosome& best) {
    std::string baseName = paths::baseName(instancePath);
    std::ofstream out(paths::solutionDetailFile(configId, baseName));
    if (!out) throw std::runtime_error("Could not open solution detail file for " + baseName);
    out << std::fixed << std::setprecision(6);

    const long nBins = static_cast<long>(best.bins.size());

    out << "Instance:\t" << instancePath << "\n";
    out << "Number of items:\t" << instance.numberOfItems() << "\n";
    out << "Bin capacity:\t" << instance.capacity() << "\n";
    out << "L2:\t" << L2 << "\n";
    out << "\n****************************GGA-CGT global best solution******************************\n";
    out << "Number of bins:\n" << nBins << "\n";
    out << "Fitness:\n" << best.fitness << "\n";
    out << "Optimal order of the weights:\n";

    Weight accumulated = 0;
    bool overCapacity = false;
    std::vector<Weight> binFullness(static_cast<std::size_t>(nBins));
    for (long bin = 0; bin < nBins; ++bin) {
        Weight sum = 0;
        for (ItemId item : best.bins[static_cast<std::size_t>(bin)].items()) {
            sum += instance.weight(item);
            accumulated += instance.weight(item);
            out << instance.weight(item) << "\n";
        }
        binFullness[static_cast<std::size_t>(bin)] = sum;
        if (sum > instance.capacity()) overCapacity = true;
    }

    // Original: non-fatal console warning on mismatch (no exit) -- not reproduced as an
    // exception; see header comment.
    (void)accumulated;

    out << "\nDetailed solution:";
    for (long j = 0; j < nBins; ++j) {
        if (binFullness[static_cast<std::size_t>(j)] > instance.capacity()) {
            out << " \n ********************ERROR the capacity of the bin was exceeded******************";
        }
        out << "\n\nBIN " << (j + 1) << "\nFullness: " << binFullness[static_cast<std::size_t>(j)] << " Gap: "
            << (instance.capacity() - binFullness[static_cast<std::size_t>(j)]) << "\nStored items:\t ";
        for (ItemId item : best.bins[static_cast<std::size_t>(j)].items()) {
            out << "[Item: " << (item + 1) << ", Weight: " << instance.weight(item) << "]\t";
        }
    }

    out.close();

    if (overCapacity) {
        throw std::runtime_error("Bin capacity exceeded in solution for " + baseName);
    }
}

} // namespace ggacgt
