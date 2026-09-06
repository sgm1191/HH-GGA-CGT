#pragma once

#include <fstream>
#include <string>

#include "../control/parameter_set.hpp"
#include "../ga/ga_parameters.hpp"

namespace ggacgt {

// Port of the config-header block in main() + WriteOutput(): one
// Solutions_GGA-CGT/GGA-CGT_(<configId>).txt per config, header written once at
// construction, one line appended per instance. The original reopens this file ("a")
// once per WriteOutput call (once per instance, not per generation, so this isn't the
// §7.5 "reopened every generation" problem -- that's Repeated_Fitness specifically) --
// kept open for the whole config run here anyway, since there's no reason not to.
class SummaryWriter {
public:
    SummaryWriter(const GaParameters& params, const ParameterSet& initialParams);

    void writeInstanceResult(const std::string& instancePath, long L2, long binsUsed, long generation,
                              double totalTimeSeconds, long deletedBins, long deletedItems,
                              long realizedCrossovers, long realizedMutations);

private:
    std::ofstream out_;
};

} // namespace ggacgt
