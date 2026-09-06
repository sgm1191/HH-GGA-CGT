#include "paths.hpp"

namespace ggacgt::paths {

std::string baseName(const std::string& instancePath) {
    return std::filesystem::path(instancePath).filename().string();
}

std::filesystem::path solutionsSummaryFile(int configId) {
    return std::filesystem::path("Solutions_GGA-CGT") / ("GGA-CGT_(" + std::to_string(configId) + ").txt");
}

std::filesystem::path solutionDetailFile(int configId, const std::string& instanceBaseName) {
    return std::filesystem::path("Details_GGA-CGT") /
           ("GGA-CGT_S_(" + std::to_string(configId) + ")_" + instanceBaseName);
}

std::filesystem::path generationTraceFile(int configId, const std::string& instanceBaseName) {
    return std::filesystem::path("Repeated_Fitness") /
           ("gene_detail_(" + std::to_string(configId) + ")_" + instanceBaseName);
}

std::filesystem::path mutationRepsFile(int configId, const std::string& instanceBaseName) {
    return std::filesystem::path("Mutation_reps") /
           ("gene_detail_muta(" + std::to_string(configId) + ")_" + instanceBaseName);
}

std::filesystem::path ratiosBinsFile(int configId, const std::string& instanceBaseName) {
    return std::filesystem::path("Ratios_bins") /
           ("gene_detail_ratio(" + std::to_string(configId) + ")_" + instanceBaseName);
}

std::filesystem::path stagnationDumpFile(int configId, const std::string& instanceBaseName) {
    std::filesystem::path base(instanceBaseName);
    std::string stem = base.stem().string(); // extension stripped, matching the original's intent
    return std::filesystem::path("Details_GGA-CGT") /
           ("detalle_solucion_" + stem + "_conf(" + std::to_string(configId) + ").txt");
}

void ensureOutputDirectories(const std::vector<std::filesystem::path>& directories) {
    for (const auto& dir : directories) std::filesystem::create_directories(dir);
}

} // namespace ggacgt::paths
