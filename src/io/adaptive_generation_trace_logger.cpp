#include "adaptive_generation_trace_logger.hpp"

#include <iomanip>
#include <stdexcept>

#include "paths.hpp"

namespace ggacgt {

AdaptiveGenerationTraceLogger::AdaptiveGenerationTraceLogger(int configId, const std::string& instanceBaseName)
    : out_(paths::generationTraceFile(configId, instanceBaseName)) {
    if (!out_) throw std::runtime_error("Could not open generation trace file for " + instanceBaseName);
    out_ << std::fixed << std::setprecision(6);
    out_ << "GENE\tSol_rep_ac\tSol_rep_dc\tbest\tgen_best\tdif_best\tid_best\tworst\tgen_worst\tdif_worst\tid_worst "
            "\tprom ";
    out_.flush();
}

void AdaptiveGenerationTraceLogger::onGeneration(const PopulationStats& stats, const ParameterSet& /*params*/,
                                                  const GenerationOperatorStats& /*operatorStats*/) {
    long genBest = stats.bestBirthGeneration;
    long difBest = stats.generation - stats.bestBirthGeneration;
    long idBest = static_cast<long>(stats.bestOrigin);
    // informacion-only convention: worst uses ordered_population[repeated_fitness],
    // not ordered_population[0] -- see PopulationStats::worstFitnessNonDuplicate.
    long genWorst = stats.worstBirthGenerationNonDuplicate;
    long difWorst = stats.generation - stats.worstBirthGenerationNonDuplicate;
    long idWorst = static_cast<long>(stats.worstOriginNonDuplicate);

    out_ << "\n"
         << stats.generation << " \t " << stats.repeatedFitnessBeforeCrossoverSelection << " \t "
         << stats.repeatedFitnessAfterCrossoverSelection << " \t " << stats.bestFitness << " \t " << genBest
         << " \t " << difBest << " \t " << idBest << " \t " << stats.worstFitnessNonDuplicate << " \t " << genWorst
         << " \t " << difWorst << " \t " << idWorst << " \t " << stats.averageFitness;
    out_.flush();
}

} // namespace ggacgt
