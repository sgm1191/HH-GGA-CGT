#include "config_loader.hpp"

#include <fstream>
#include <stdexcept>

namespace ggacgt {

std::vector<ConfigEntry> loadConfigurations(const std::string& path) {
    std::ifstream in(path);
    if (!in) throw std::runtime_error("There is no data file ==> [" + path + "]");

    std::string headerLine;
    std::getline(in, headerLine);

    std::vector<ConfigEntry> entries;
    int confId;
    while (in >> confId) {
        ConfigEntry entry;
        entry.gaParams.configId = confId;

        int pSize = 0, maxGen = 0, lifeSpan = 0, seed = 0, saveBest = 0;
        float pM = 0, pC = 0, kNcs = 0, kCs = 0, bSize = 0;
        in >> pSize >> maxGen >> pM >> pC >> kNcs >> kCs >> bSize >> lifeSpan >> seed >> saveBest;

        entry.gaParams.pSize = pSize;
        entry.gaParams.maxGen = maxGen;
        entry.gaParams.bSize = bSize;
        entry.gaParams.lifeSpan = lifeSpan;
        entry.gaParams.seed = seed;
        entry.gaParams.saveBestSolution = (saveBest == 1);

        entry.initialParams.pM = pM;
        entry.initialParams.pC = pC;
        entry.initialParams.kNcs = kNcs;
        entry.initialParams.kCs = kCs;

        entries.push_back(entry);
    }
    return entries;
}

} // namespace ggacgt
