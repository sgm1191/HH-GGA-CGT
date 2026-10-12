// Composition root for "gga-cgt" (the base/static-parameter variant): wires
// GgaCgtEngine + StaticParameterController + SummaryWriter + SolutionWriter +
// GenerationTraceLogger. No GA logic lives here -- this is wiring only
// (REFACTORING_STRATEGY.md §2, app/main_base.cpp).

#include <chrono>
#include <exception>
#include <iostream>
#include <memory>

#include "../control/static_parameter_controller.hpp"
#include "../core/bpp_instance.hpp"
#include "../core/lower_bound.hpp"
#include "../ga/gga_cgt_engine.hpp"
#include "../io/config_loader.hpp"
#include "../io/generation_trace_logger.hpp"
#include "../io/instance_list_loader.hpp"
#include "../io/instance_loader.hpp"
#include "../io/paths.hpp"
#include "../io/solution_writer.hpp"
#include "../io/summary_writer.hpp"
#include "../operators/adaptive_mutation.hpp"
#include "../operators/ff_initializer.hpp"
#include "../operators/gene_level_crossover.hpp"

using namespace ggacgt;

int main() {
    try {
        paths::ensureOutputDirectories({paths::outputDir("Solutions_GGA-CGT"), paths::outputDir("Details_GGA-CGT"),
                                        paths::outputDir("Repeated_Fitness")});

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
                auto crossover = std::make_unique<GeneLevelCrossoverFFD>(instance);
                auto mutation = std::make_unique<AdaptiveMutationRP>(instance);
                auto controller = std::make_unique<StaticParameterController>(config.initialParams);

                GgaCgtEngine engine(instance, config.gaParams, std::move(initializer), std::move(crossover),
                                     std::move(mutation), std::move(controller));

                std::string baseName = paths::baseName(instancePath);
                engine.addObserver(std::make_shared<GenerationTraceLogger>(config.gaParams.configId, baseName));

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
