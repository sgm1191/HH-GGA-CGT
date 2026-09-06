#include "adaptive_trace_logger.hpp"

#include <iomanip>
#include <stdexcept>

#include "paths.hpp"

namespace ggacgt {

AdaptiveTraceLogger::AdaptiveTraceLogger(int configId, const std::string& instanceBaseName, long pSize)
    : pSize_(pSize),
      mutationReps_(paths::mutationRepsFile(configId, instanceBaseName)),
      ratiosBins_(paths::ratiosBinsFile(configId, instanceBaseName)) {
    if (!mutationReps_) throw std::runtime_error("Could not open Mutation_reps file for " + instanceBaseName);
    if (!ratiosBins_) throw std::runtime_error("Could not open Ratios_bins file for " + instanceBaseName);
    mutationReps_ << std::fixed << std::setprecision(6);
    ratiosBins_ << std::fixed << std::setprecision(6);

    mutationReps_ << "Gene\tComplete\tIncomplete\tDelete\tBinsSolPartialM\tItemsFreeM\tBinsSolPartialC\tItemsFreeC"
                     "\tSize_father_B  \tSize_father_prom \tComplete_Bins_M\tComplete_Bins_C \tP_c "
                     "\tProportion_bins_full_c\tP_m";
    ratiosBins_ << "Gene\tRatio_best_M\tRatio_worst_M\tRatio_prom_M\tRatio_best_C\tRatio_worst_C\tRatio_prom_C";
    mutationReps_.flush();
    ratiosBins_.flush();
}

void AdaptiveTraceLogger::onGeneration(const PopulationStats& stats, const ParameterSet& params,
                                        const GenerationOperatorStats& op) {
    const double pSizeReal = static_cast<double>(pSize_);
    const double mutationDenom = static_cast<double>(params.pM) * pSizeReal;
    const double crossDenom = static_cast<double>(params.pC) * pSizeReal;

    double promedioCompletos = 0, promedioIncompletos = 0, promedioEliminados = 0;
    double binsSolParcialMuta = 0, itemsEliminadosMuta = 0;
    if (mutationDenom != 0.0) {
        promedioCompletos = static_cast<double>(op.mutation.completeBins) / mutationDenom;
        promedioIncompletos = static_cast<double>(op.mutation.incompleteBins) / mutationDenom;
        promedioEliminados = static_cast<double>(op.mutation.eliminatedBins) / mutationDenom;
        binsSolParcialMuta = static_cast<double>(op.mutation.survivingBins) / mutationDenom;
        itemsEliminadosMuta = static_cast<double>(op.mutation.freeItems) / mutationDenom;
    }
    // arr_acul_muta[5] ("Complete_Bins_M"): the original reads uninitialized `m_bins`
    // here -- not reproduced (see operator_interfaces.hpp's MutationStats doc comment).
    const double completeBinsMuta = 0.0;

    double binsSolParcial = 0, itemsEliminados = 0, sizeFather = 0;
    double completeBinsCross = 0, sizeFatherC = 0, proportionBinsFull = 0;
    if (params.pC != 0.0f) {
        binsSolParcial = (static_cast<double>(op.crossover.partialBins) / crossDenom) * 2.0;
        itemsEliminados = (static_cast<double>(op.crossover.eliminatedItems) / crossDenom) * 2.0;
        sizeFather = (static_cast<double>(op.crossover.generatedPairSize) / crossDenom) * 2.0;
        completeBinsCross = ((static_cast<double>(op.crossover.parentsFullBins) / crossDenom) * 2.0) / 2.0;
        sizeFatherC = ((static_cast<double>(op.crossover.parentsBinsUsed) / crossDenom) * 2.0) / 2.0;
        proportionBinsFull = (op.crossover.fullBinRatio / crossDenom) * 2.0;
    }

    mutationReps_ << "\n"
                  << stats.generation << " \t " << promedioCompletos << " \t " << promedioIncompletos << " \t "
                  << promedioEliminados << " \t " << binsSolParcialMuta << " \t " << itemsEliminadosMuta << " \t "
                  << binsSolParcial << " \t " << itemsEliminados << " \t " << sizeFather << "\t " << sizeFatherC
                  << " \t " << completeBinsMuta << " \t " << completeBinsCross << " \t " << params.pC << " \t "
                  << proportionBinsFull << " \t " << params.pM;
    mutationReps_.flush();

    // best/worst/average ratio: the "_M" and "_C" columns are the SAME value in the
    // original (confirmed against the source -- see header comment), computed once.
    ratiosBins_ << "\n"
                << stats.generation << " \t " << stats.bestFullBinRatio << " \t " << stats.worstFullBinRatio
                << " \t " << stats.averageFullBinRatio << " \t " << stats.bestFullBinRatio << " \t "
                << stats.worstFullBinRatio << " \t " << stats.averageFullBinRatio << " ";
    ratiosBins_.flush();
}

} // namespace ggacgt
