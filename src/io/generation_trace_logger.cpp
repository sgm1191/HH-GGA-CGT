#include "generation_trace_logger.hpp"

#include <iomanip>
#include <stdexcept>

#include "paths.hpp"

namespace ggacgt {

GenerationTraceLogger::GenerationTraceLogger(int configId, const std::string& instanceBaseName)
    : out_(paths::generationTraceFile(configId, instanceBaseName)) {
    if (!out_) throw std::runtime_error("Could not open generation trace file for " + instanceBaseName);
    out_ << std::fixed << std::setprecision(6);
    out_ << "GENE\tSol_rep_ac\tSol_rep_dc\tbest\tgen_best\tdif_best\tid_best\tworst\tgen_worst\tdif_worst\tid_worst";
    out_.flush();
}

void GenerationTraceLogger::onGeneration(const PopulationStats& stats, const ParameterSet& /*params*/,
                                          const GenerationOperatorStats& /*operatorStats*/) {
    long genBest = stats.bestBirthGeneration;
    long difBest = stats.generation - stats.bestBirthGeneration;
    long idBest = static_cast<long>(stats.bestOrigin);
    long genWorst = stats.worstBirthGeneration;
    long difWorst = stats.generation - stats.worstBirthGeneration;
    long idWorst = static_cast<long>(stats.worstOrigin);

    out_ << "\n"
         << stats.generation << " \t " << stats.repeatedFitnessBeforeCrossoverSelection << " \t "
         << stats.repeatedFitnessAfterCrossoverSelection << " \t " << stats.bestFitness << " \t " << genBest
         << " \t " << difBest << " \t " << idBest << " \t " << stats.worstFitness << " \t " << genWorst << " \t "
         << difWorst << " \t " << idWorst;
    out_.flush();
}

} // namespace ggacgt
