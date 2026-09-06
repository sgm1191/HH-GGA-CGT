#pragma once

#include <fstream>
#include <string>

#include "../ga/generation_observer.hpp"

namespace ggacgt {

// Port of the inline Repeated_Fitness/gene_detail_(<configId>)_<baseName> writer.
// Opens the file once at construction (header row) and appends one row per generation
// via onGeneration() -- the original reopens this file with fopen/fclose every single
// generation (REFACTORING_STRATEGY.md §7.5); this keeps one std::ofstream open for the
// whole run instead.
//
// FOUND BUG (see hh/CHECKLIST.md Recommendations -- flagged prominently, it affects
// every historical Repeated_Fitness/*.txt file the original binaries have ever
// produced): the original's per-generation write has a misplaced parenthesis --
//   fprintf(output_gen, "...%d", ..., (int)dif_worst), (int)id_worst;
// The closing paren lands right after dif_worst; `, (int)id_worst` after it is the
// comma operator, not a fifth-from-last fprintf argument, so `id_worst` is computed but
// never actually passed. The format string still has 11 "%d"/"%f" conversions for only
// 10 supplied arguments, so the final column reads an argument that was never pushed:
// undefined behavior (whatever garbage happens to sit in that vararg slot). There is no
// deterministic "correct" garbage to reproduce, so this port fixes the call to actually
// pass id_worst, matching the format string's evident intent, rather than reproducing
// unreproducible UB.
class GenerationTraceLogger : public IGenerationObserver {
public:
    GenerationTraceLogger(int configId, const std::string& instanceBaseName);

    void onGeneration(const PopulationStats& stats, const ParameterSet& params,
                       const GenerationOperatorStats& operatorStats) override;

private:
    std::ofstream out_;
};

} // namespace ggacgt
