#pragma once

#include <memory>
#include <vector>

#include "../control/parameter_controller.hpp"
#include "../core/bpp_instance.hpp"
#include "../core/lower_bound.hpp"
#include "../core/population.hpp"
#include "../core/rng.hpp"
#include "../operators/controlled_selection.hpp"
#include "../operators/operator_interfaces.hpp"
#include "ga_parameters.hpp"
#include "generation_observer.hpp"

namespace ggacgt {

struct RunResult {
    Chromosome bestSolution;
    bool optimalFound = false;
    // Generation at which the optimal (== L2) solution was found, or params.maxGen if
    // the run completed without finding one (original: the loop-ending value of
    // `generation`, used verbatim in WriteOutput's summary line).
    GenerationNumber generationFound = 0;
    long deletedBins = 0;
    long deletedItems = 0;
    long realizedCrossovers = 0;
    long realizedMutations = 0;
};

// THE FRAMEWORK (REFACTORING_STRATEGY.md §2): owns the Population, runs the
// select->cross->replace->select->mutate->replace->check-optimal generation loop, and
// delegates every decision to injected operators/controller/observers. No file I/O, no
// globals -- `main_base.cpp` and `main_informacion.cpp` differ only in which
// IParameterController and which observers they attach.
class GgaCgtEngine {
public:
    GgaCgtEngine(const BppInstance& instance, GaParameters params, std::unique_ptr<IInitializer> initializer,
                 std::unique_ptr<ICrossover> crossover, std::unique_ptr<IMutation> mutation,
                 std::unique_ptr<IParameterController> controller);

    void addObserver(std::shared_ptr<IGenerationObserver> observer);

    // Runs until max_gen generations complete or an L2-matching solution is found.
    RunResult run();

private:
    // Returns a populated RunResult if this generation found the optimum (original:
    // Generation()'s five early-exit-on-optimal sites, unified into one check per
    // phase), std::nullopt otherwise.
    bool generationStep(GenerationNumber generation, RunResult& outResult);
    void findBestSolution(GenerationNumber generation);
    bool checkOptimalAndRecord(Chromosome& candidate, GenerationNumber generation, RunResult& outResult);

    const BppInstance& instance_;
    GaParameters params_;
    LowerBoundResult lowerBound_;

    std::unique_ptr<IInitializer> initializer_;
    std::unique_ptr<ICrossover> crossover_;
    std::unique_ptr<IMutation> mutation_;
    std::unique_ptr<IParameterController> controller_;
    std::vector<std::shared_ptr<IGenerationObserver>> observers_;

    // The original's three independent RNG streams (see core/rng.hpp RngStreams doc
    // comment), all seeded from params_.seed.
    Rng rngCoin_;
    Rng rngPermutation_;
    Rng rngEmptyBin_;

    Population population_;
    Chromosome globalBest_;
    SelectionPools pools_;

    long deletedBins_ = 0;
    long deletedItems_ = 0;
    long realizedCrossovers_ = 0;
    long realizedMutations_ = 0;
};

} // namespace ggacgt
