// Composition root for "gga-cgt-info" (the informacion/adaptive-parameter variant):
// wires GgaCgtEngine + AdaptiveParameterController + SummaryWriter + SolutionWriter +
// AdaptiveGenerationTraceLogger + AdaptiveTraceLogger + StagnationDumpObserver.
//
// Unlike main_base.cpp, this is NOT pure wiring on top of the exact same operators --
// per hh/CHECKLIST.md Recommendations #17, the informacion variant genuinely needs its
// own crossover (GeneLevelCrossoverFFDGLX, built on RearrangementByPairsGLX instead of
// plain First-Fit reinsertion). The mutation operator (AdaptiveMutationRP) IS shared
// with the base variant unchanged -- the informacion source's Adaptive_Mutation_RP
// still calls the same RP, just with extra stats bookkeeping that AdaptiveMutationRP
// already reports via IMutation::lastMutationStats() regardless of which composition
// root uses it.

#include <chrono>
#include <exception>
#include <iostream>
#include <memory>

#include "../control/adaptive_parameter_controller.hpp"
#include "../core/bpp_instance.hpp"
#include "../core/lower_bound.hpp"
#include "../ga/gga_cgt_engine.hpp"
#include "../io/adaptive_generation_trace_logger.hpp"
#include "../io/adaptive_trace_logger.hpp"
#include "../io/config_loader.hpp"
#include "../io/instance_list_loader.hpp"
#include "../io/instance_loader.hpp"
#include "../io/paths.hpp"
#include "../io/solution_writer.hpp"
#include "../io/stagnation_dump_observer.hpp"
#include "../io/summary_writer.hpp"
#include "../operators/adaptive_mutation.hpp"
#include "../operators/ff_initializer.hpp"
#include "../operators/gene_level_crossover_glx.hpp"

using namespace ggacgt;

int main() {
    try {
        paths::ensureOutputDirectories(
            {paths::outputDir("Solutions_GGA-CGT"), paths::outputDir("Details_GGA-CGT"),
             paths::outputDir("Repeated_Fitness"), paths::outputDir("Mutation_reps"), paths::outputDir("Ratios_bins")});

        auto configs = loadConfigurations("configurations.txt");
        auto instancePaths = loadInstanceList("instances.txt");

        for (const auto& config : configs) {
            SummaryWriter summaryWriter(config.gaParams, config.initialParams);

            for (std::size_t idx = 0; idx < instancePaths.size(); ++idx) {
                const std::string& instancePath = instancePaths[idx];
                std::cout << "[conf " << config.gaParams.configId << "] (" << (idx + 1) << "/"
                          << instancePaths.size() << ") " << instancePath << " ... " << std::flush;
                BppInstance instance = loadInstance(instancePath);
                LowerBoundResult lb = computeLowerBound(instance);

                auto initializer = std::make_unique<FFInitializer>(instance, lb.largeItemCount);
                auto crossover = std::make_unique<GeneLevelCrossoverFFDGLX>(instance);
                auto mutation = std::make_unique<AdaptiveMutationRP>(instance);
                auto controller = std::make_unique<AdaptiveParameterController>(config.gaParams.pSize,
                                                                                 config.gaParams.bSize,
                                                                                 config.gaParams.maxGen);

                GgaCgtEngine engine(instance, config.gaParams, std::move(initializer), std::move(crossover),
                                     std::move(mutation), std::move(controller));

                std::string baseName = paths::baseName(instancePath);
                engine.addObserver(
                    std::make_shared<AdaptiveGenerationTraceLogger>(config.gaParams.configId, baseName));
                engine.addObserver(
                    std::make_shared<AdaptiveTraceLogger>(config.gaParams.configId, baseName, config.gaParams.pSize));
                engine.addObserver(std::make_shared<StagnationDumpObserver>(config.gaParams.configId, baseName));

                auto start = std::chrono::steady_clock::now();
                RunResult result = engine.run();
                auto end = std::chrono::steady_clock::now();
                double totalTime = std::chrono::duration<double>(end - start).count();

                summaryWriter.writeInstanceResult(instancePath, lb.L2, result.bestSolution.binsUsed,
                                                   result.generationFound, totalTime, result.deletedBins,
                                                   result.deletedItems, result.realizedCrossovers,
                                                   result.realizedMutations);

                std::cout << "L2=" << lb.L2 << " bins=" << result.bestSolution.binsUsed
                          << " gen=" << result.generationFound << " time=" << totalTime << "s" << std::endl;

                if (config.gaParams.saveBestSolution) {
                    writeSolutionDetail(config.gaParams.configId, instancePath, instance, lb.L2,
                                         result.bestSolution);
                }
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "\nFatal error: " << e.what() << "\n";
        return 1;
    }

    std::cout << "\n\n\tEnd of process\n";
    return 0;
}
