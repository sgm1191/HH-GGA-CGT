#include "gga_cgt_engine.hpp"

#include <utility>

#include "../operators/controlled_replacement.hpp"

namespace ggacgt {

GgaCgtEngine::GgaCgtEngine(const BppInstance& instance, GaParameters params,
                            std::unique_ptr<IInitializer> initializer, std::unique_ptr<ICrossover> crossover,
                            std::unique_ptr<IMutation> mutation, std::unique_ptr<IParameterController> controller)
    : instance_(instance),
      params_(params),
      lowerBound_(computeLowerBound(instance)),
      initializer_(std::move(initializer)),
      crossover_(std::move(crossover)),
      mutation_(std::move(mutation)),
      controller_(std::move(controller)),
      rngCoin_(params.seed),
      rngPermutation_(params.seed),
      rngEmptyBin_(params.seed),
      population_(params.pSize),
      pools_(params.pSize) {}

void GgaCgtEngine::addObserver(std::shared_ptr<IGenerationObserver> observer) {
    observers_.push_back(std::move(observer));
}

bool GgaCgtEngine::checkOptimalAndRecord(Chromosome& candidate, GenerationNumber generation, RunResult& outResult) {
    if (candidate.binsUsed != lowerBound_.L2) return false;
    globalBest_ = candidate;
    outResult.bestSolution = globalBest_;
    outResult.optimalFound = true;
    outResult.generationFound = generation;
    outResult.deletedBins = deletedBins_;
    outResult.deletedItems = deletedItems_;
    outResult.realizedCrossovers = realizedCrossovers_;
    outResult.realizedMutations = realizedMutations_;
    return true;
}

void GgaCgtEngine::findBestSolution(GenerationNumber generation) {
    // Original: Find_Best_Solution -- scans by RAW index (not fitness order), and only
    // requires strict improvement once generation > 0; the very first call (generation
    // == 0, i.e. right after generation 0's Generation() call) copies unconditionally.
    long bestIndividual = 0;
    for (long i = 1; i < params_.pSize; ++i) {
        if (population_.at(i).fitness > population_.at(bestIndividual).fitness) bestIndividual = i;
    }
    if (generation > 0) {
        if (population_.at(bestIndividual).fitness > globalBest_.fitness) {
            globalBest_ = population_.at(bestIndividual);
        }
    } else {
        globalBest_ = population_.at(bestIndividual);
    }
}

RunResult GgaCgtEngine::run() {
    RunResult result;

    // Generate_Initial_Population
    for (long i = 0; i < params_.pSize; ++i) {
        Chromosome individual = initializer_->generate(rngPermutation_, 0);
        individual.fitness /= static_cast<double>(individual.binsUsed);
        population_.at(i) = std::move(individual);
        if (checkOptimalAndRecord(population_.at(i), 0, result)) return result;
    }

    for (GenerationNumber generation = 0; generation < params_.maxGen; ++generation) {
        if (generationStep(generation, result)) return result;
        findBestSolution(generation);
    }

    result.optimalFound = false;
    result.bestSolution = globalBest_;
    result.generationFound = params_.maxGen;
    result.deletedBins = deletedBins_;
    result.deletedItems = deletedItems_;
    result.realizedCrossovers = realizedCrossovers_;
    result.realizedMutations = realizedMutations_;
    return result;
}

bool GgaCgtEngine::generationStep(GenerationNumber generation, RunResult& outResult) {
    RngStreams rngStreams{rngCoin_, rngPermutation_, rngEmptyBin_};

    // ---- Controlled selection for crossover ----
    PopulationStats stats;
    stats.generation = generation;
    stats.repeatedFitnessBeforeCrossoverSelection = population_.sortByFitnessAndCountDuplicates();

    long worstIdx = population_.orderedIndex(0);
    long bestIdx = population_.orderedIndex(params_.pSize - 1);
    stats.worstFitness = population_.at(worstIdx).fitness;
    stats.worstBirthGeneration = population_.at(worstIdx).birthGeneration;
    stats.worstOrigin = population_.at(worstIdx).origin;
    stats.bestFitness = population_.at(bestIdx).fitness;
    stats.bestBirthGeneration = population_.at(bestIdx).birthGeneration;
    stats.bestOrigin = population_.at(bestIdx).origin;
    {
        double sum = 0.0;
        double ratioSum = 0.0;
        for (long i = 0; i < params_.pSize; ++i) {
            sum += population_.at(i).fitness;
            const Chromosome& c = population_.at(i);
            ratioSum += static_cast<double>(c.fullBinCount) / static_cast<double>(c.binsUsed);
        }
        stats.averageFitness = sum / static_cast<double>(params_.pSize);
        stats.averageFullBinRatio = ratioSum / static_cast<double>(params_.pSize);
    }
    stats.bestFullBinRatio =
        static_cast<double>(population_.at(bestIdx).fullBinCount) / static_cast<double>(population_.at(bestIdx).binsUsed);

    // informacion-only "worst" (see PopulationStats::worstFitnessNonDuplicate doc
    // comment): `ordered_population[repeated_fitness]`, clamped in case every
    // individual shares the same fitness (repeated_fitness == pSize, which the
    // original does not guard against either).
    {
        long rank = stats.repeatedFitnessBeforeCrossoverSelection;
        if (rank >= params_.pSize) rank = params_.pSize - 1;
        long nonDupIdx = population_.orderedIndex(rank);
        stats.worstFitnessNonDuplicate = population_.at(nonDupIdx).fitness;
        stats.worstBirthGenerationNonDuplicate = population_.at(nonDupIdx).birthGeneration;
        stats.worstOriginNonDuplicate = population_.at(nonDupIdx).origin;
        stats.worstFullBinRatio = static_cast<double>(population_.at(nonDupIdx).fullBinCount) /
                                   static_cast<double>(population_.at(nonDupIdx).binsUsed);
    }

    // informacion-only stagnation trigger, checked BEFORE this generation's own
    // crossover/mutation touch the population -- see generation_observer.hpp.
    if (stats.bestFullBinRatio == 0.0) {
        for (auto& observer : observers_) {
            if (observer->wantsStagnationDump()) observer->onStagnationDetected(population_, generation);
        }
    }

    ParameterSet ps = controller_->next(generation, stats);

    reshuffleSelectionPools(pools_, params_.pSize, params_.bSize, ps.pC, rngPermutation_);

    // ---- Crossover ----
    std::vector<Chromosome> children;
    CrossoverCounters crossCounters;
    GenerationOperatorStats operatorStats;
    long k = 0;
    long h = params_.pSize - 1;

    for (long i = params_.pSize - 1, childIndex = 0;
         static_cast<float>(i) > (static_cast<float>(params_.pSize) - (ps.pC / 2.0f * static_cast<float>(params_.pSize)));
         --i, ++childIndex) {
        ++realizedCrossovers_;
        long f1 = pools_.bestIndividuals[static_cast<std::size_t>(h--)];
        long f2 = pools_.randomIndividuals[static_cast<std::size_t>(k++)];
        if (f2 == f1) {
            f1 = pools_.bestIndividuals[static_cast<std::size_t>(h--)];
        }
        long father1 = population_.orderedIndex(f1);
        long father2 = population_.orderedIndex(f2);

        Chromosome child = crossover_->cross(population_.at(father1), population_.at(father2), rngStreams,
                                              generation + 1, crossCounters);
        child.fitness /= static_cast<double>(child.binsUsed);
        children.push_back(std::move(child));

        // informacion-only: fold this call's CrossoverStats into the per-generation
        // total (original: arr_acul_cruza[k] += ..., summed across every crossover
        // call in Generation()). No-op for operators that don't implement this.
        if (crossover_->hasLastCallStats()) {
            CrossoverStats s = crossover_->lastCrossoverStats();
            operatorStats.crossover.partialBins += s.partialBins;
            operatorStats.crossover.eliminatedItems += s.eliminatedItems;
            operatorStats.crossover.generatedPairSize += s.generatedPairSize;
            operatorStats.crossover.parentsFullBins += s.parentsFullBins;
            operatorStats.crossover.parentsBinsUsed += s.parentsBinsUsed;
            operatorStats.crossover.fullBinRatio += s.fullBinRatio;
            ++operatorStats.crossoverCalls;
        }

        // Fold this crossover's deletions into the run-level totals BEFORE the
        // optimal check, since checkOptimalAndRecord snapshots deletedBins_/
        // deletedItems_ into outResult -- doing this after an early return would leave
        // the just-found winning child's own deletions uncounted.
        deletedBins_ += crossCounters.deletedBins;
        deletedItems_ += crossCounters.deletedItems;
        crossCounters = CrossoverCounters{};

        // NOTE: `generation`, not `generation + 1` -- the original's WriteOutput prints
        // the outer loop's raw `generation` value even though the winning
        // child/mutant's OWN birthGeneration metadata is stamped generation+1. This is
        // an asymmetry in the original, reproduced deliberately (see hh/CHECKLIST.md).
        if (checkOptimalAndRecord(children.back(), generation, outResult)) {
            return true;
        }
    }

    // ---- Controlled replacement for crossover ----
    applyControlledReplacement(population_, pools_, children, params_.pSize, ps.pC);

    // ---- Controlled selection for mutation ----
    stats.repeatedFitnessAfterCrossoverSelection = population_.sortByFitnessAndCountDuplicates();

    long cloneTargetRank = 0;
    for (long i = params_.pSize - 1;
         static_cast<float>(i) > (static_cast<float>(params_.pSize) - (ps.pM * static_cast<float>(params_.pSize)));
         --i) {
        ++realizedMutations_;
        long sourceIndividual = population_.orderedIndex(i);
        bool clonePath = (i != cloneTargetRank) &&
                          (cloneTargetRank < static_cast<long>(params_.pSize * params_.bSize)) &&
                          ((generation + 1 - population_.at(sourceIndividual).birthGeneration) < params_.lifeSpan);

        if (clonePath) {
            long targetIndividual = population_.orderedIndex(cloneTargetRank);
            population_.at(targetIndividual) = population_.at(sourceIndividual);
            mutation_->mutate(population_.at(targetIndividual), ps.kCs, rngStreams);
            if (mutation_->hasLastCallStats()) {
                MutationStats s = mutation_->lastMutationStats();
                operatorStats.mutation.completeBins += s.completeBins;
                operatorStats.mutation.incompleteBins += s.incompleteBins;
                operatorStats.mutation.eliminatedBins += s.eliminatedBins;
                operatorStats.mutation.survivingBins += s.survivingBins;
                operatorStats.mutation.freeItems += s.freeItems;
                ++operatorStats.mutationCalls;
            }
            population_.at(targetIndividual).birthGeneration = generation + 1;
            population_.at(targetIndividual).fitness /=
                static_cast<double>(population_.at(targetIndividual).binsUsed);
            population_.at(targetIndividual).origin = Chromosome::Origin::Mutation;
            if (checkOptimalAndRecord(population_.at(targetIndividual), generation, outResult)) return true;
            ++cloneTargetRank;
        } else {
            mutation_->mutate(population_.at(sourceIndividual), ps.kNcs, rngStreams);
            if (mutation_->hasLastCallStats()) {
                MutationStats s = mutation_->lastMutationStats();
                operatorStats.mutation.completeBins += s.completeBins;
                operatorStats.mutation.incompleteBins += s.incompleteBins;
                operatorStats.mutation.eliminatedBins += s.eliminatedBins;
                operatorStats.mutation.survivingBins += s.survivingBins;
                operatorStats.mutation.freeItems += s.freeItems;
                ++operatorStats.mutationCalls;
            }
            population_.at(sourceIndividual).birthGeneration = generation + 1;
            population_.at(sourceIndividual).fitness /=
                static_cast<double>(population_.at(sourceIndividual).binsUsed);
            population_.at(sourceIndividual).origin = Chromosome::Origin::Mutation;
            if (checkOptimalAndRecord(population_.at(sourceIndividual), generation, outResult)) return true;
        }
    }

    for (auto& observer : observers_) observer->onGeneration(stats, ps, operatorStats);

    return false;
}

} // namespace ggacgt
