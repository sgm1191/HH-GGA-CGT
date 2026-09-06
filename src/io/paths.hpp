#pragma once

#include <filesystem>
#include <string>
#include <vector>

namespace ggacgt::paths {

// Replaces the original's fixed `char[512]` buffers + `sprintf`/`strcat` chains, which
// were the exact cause of the buffer-overflow and double-`data/`-prefix bugs recorded in
// CHANGES.md (REFACTORING_STRATEGY.md §7.6). `std::filesystem::path` composition can't
// overflow and doesn't need a `file_base`-style manual `strrchr` to strip directories.

// The filename component of an instance path (original: `file_base`, computed once per
// instance via `strrchr(file, '/')`).
std::string baseName(const std::string& instancePath);

// Solutions_GGA-CGT/GGA-CGT_(<configId>).txt -- one config's run summary (original:
// `nameC`, built once per config in main()).
std::filesystem::path solutionsSummaryFile(int configId);

// Details_GGA-CGT/GGA-CGT_S_(<configId>)_<instanceBaseName> -- one instance's best
// solution detail (original: `sendtofile`'s `fil`).
std::filesystem::path solutionDetailFile(int configId, const std::string& instanceBaseName);

// Repeated_Fitness/gene_detail_(<configId>)_<instanceBaseName> -- one instance's
// per-generation trace (original: main()'s `filename`).
std::filesystem::path generationTraceFile(int configId, const std::string& instanceBaseName);

// Mutation_reps/gene_detail_muta(<configId>)_<instanceBaseName> -- informacion-only.
std::filesystem::path mutationRepsFile(int configId, const std::string& instanceBaseName);

// Ratios_bins/gene_detail_ratio(<configId>)_<instanceBaseName> -- informacion-only.
std::filesystem::path ratiosBinsFile(int configId, const std::string& instanceBaseName);

// Details_GGA-CGT/detalle_solucion_<instanceBaseNameWithoutExtension>_conf(<configId>).txt
// -- the informacion variant's stagnation dump (original: sendtofile-adjacent code in
// Generation(), see io/stagnation_dump_observer.hpp).
std::filesystem::path stagnationDumpFile(int configId, const std::string& instanceBaseName);

// Creates every output directory the writers below need, once, up front (original:
// the `system("mkdir -p ...")` calls at the top of main()).
void ensureOutputDirectories(const std::vector<std::filesystem::path>& directories);

} // namespace ggacgt::paths
