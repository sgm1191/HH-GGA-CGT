#include "summary_writer.hpp"

#include <iomanip>
#include <stdexcept>

#include "paths.hpp"

namespace ggacgt {

SummaryWriter::SummaryWriter(const GaParameters& params, const ParameterSet& initialParams)
    : out_(paths::solutionsSummaryFile(params.configId)) {
    if (!out_) {
        throw std::runtime_error("Could not open solutions summary file for config " +
                                  std::to_string(params.configId));
    }
    out_ << std::fixed << std::setprecision(6);

    out_ << "CONF\t|P|\tmax_gen\tn_m\tn_c\tk1(non-cloned_solutions)\tk2(cloned_solutions)\t|B|\tlife_span\tseed";
    out_ << "\n"
         << params.configId << "\t" << params.pSize << "\t" << params.maxGen << "\t" << initialParams.pM << "\t"
         << initialParams.pC << "\t" << initialParams.kNcs << "\t" << initialParams.kCs << "\t" << params.bSize
         << "\t" << params.lifeSpan << "\t" << params.seed;
    out_ << "\nInstancias \t L2 \t Bins \t Gen \t Time \t DelBins \t DelItems \t Crosses \t Mutations";
    out_.flush();
}

void SummaryWriter::writeInstanceResult(const std::string& instancePath, long L2, long binsUsed, long generation,
                                         double totalTimeSeconds, long deletedBins, long deletedItems,
                                         long realizedCrossovers, long realizedMutations) {
    out_ << "\n"
         << instancePath << " \t " << L2 << " \t " << binsUsed << " \t " << generation << " \t " << totalTimeSeconds
         << " \t " << deletedBins << " \t " << deletedItems << " \t " << realizedCrossovers << " \t "
         << realizedMutations;
    out_.flush();
}

} // namespace ggacgt
