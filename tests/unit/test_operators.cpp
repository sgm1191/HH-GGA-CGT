// Operator invariant checks, per REFACTORING_STRATEGY.md §5: crossover/mutation must
// never produce a bin with a duplicate item, never exceed bin_capacity, and every item
// must end up placed exactly once.
#include <cstdio>
#include <cstdlib>
#include <vector>

#include "core/bpp_instance.hpp"
#include "core/lower_bound.hpp"
#include "core/rng.hpp"
#include "operators/adaptive_mutation.hpp"
#include "operators/ff_initializer.hpp"
#include "operators/gene_level_crossover.hpp"

using namespace ggacgt;

namespace {

void expect(bool condition, const char* what) {
    if (!condition) {
        std::fprintf(stderr, "FAILED: %s\n", what);
        std::exit(1);
    }
}

BppInstance makeInstance() {
    std::vector<Weight> weights = {9, 8, 7, 6, 5, 5, 4, 4, 3, 3, 3, 2, 2, 1, 1};
    return BppInstance("synthetic", weights, /*capacity=*/10, /*bestKnownSolution=*/0);
}

void checkValidSolution(const BppInstance& instance, const Chromosome& c, const char* label) {
    std::vector<int> seen(static_cast<std::size_t>(instance.numberOfItems()), 0);
    for (const Bin& bin : c.bins) {
        Weight sum = 0;
        for (ItemId item : bin.items()) {
            expect(item >= 0 && item < instance.numberOfItems(), "item id in range");
            expect(seen[static_cast<std::size_t>(item)] == 0, "item placed exactly once (no duplicate across bins)");
            seen[static_cast<std::size_t>(item)] = 1;
            sum += instance.weight(item);
        }
        expect(sum == bin.fullness(), "bin's cached fullness matches sum of its items' weights");
        expect(sum <= instance.capacity(), "bin does not exceed capacity");
    }
    for (long i = 0; i < instance.numberOfItems(); ++i) {
        expect(seen[static_cast<std::size_t>(i)] == 1, "every item placed exactly once (no item lost)");
    }
    expect(c.binsUsed == static_cast<long>(c.bins.size()), "binsUsed matches bins.size()");
    std::printf("  %s: OK (%ld bins)\n", label, c.binsUsed);
}

void testCrossoverInvariants() {
    BppInstance instance = makeInstance();
    Rng permutationRng(111);
    FFInitializer initializer(instance, computeLowerBound(instance).largeItemCount);

    Chromosome parent1 = initializer.generate(permutationRng, 0);
    Chromosome parent2 = initializer.generate(permutationRng, 0);
    checkValidSolution(instance, parent1, "crossover parent1");
    checkValidSolution(instance, parent2, "crossover parent2");

    Rng coinRng(222), permRng2(333), emptyBinRng(444);
    RngStreams rngStreams{coinRng, permRng2, emptyBinRng};
    GeneLevelCrossoverFFD crossover(instance);
    CrossoverCounters counters;

    for (int trial = 0; trial < 5; ++trial) {
        Chromosome child = crossover.cross(parent1, parent2, rngStreams, /*birthGeneration=*/1, counters);
        checkValidSolution(instance, child, "crossover child");
    }
}

void testMutationInvariants() {
    BppInstance instance = makeInstance();
    Rng permutationRng(555);
    FFInitializer initializer(instance, computeLowerBound(instance).largeItemCount);
    Chromosome individual = initializer.generate(permutationRng, 0);
    checkValidSolution(instance, individual, "mutation source");

    Rng coinRng(666), permRng2(777), emptyBinRng(888);
    RngStreams rngStreams{coinRng, permRng2, emptyBinRng};
    AdaptiveMutationRP mutation(instance);

    for (int trial = 0; trial < 5; ++trial) {
        mutation.mutate(individual, /*k=*/4.0f, rngStreams);
        checkValidSolution(instance, individual, "mutated individual");
    }
}

} // namespace

int main() {
    testCrossoverInvariants();
    testMutationInvariants();
    std::printf("test_operators: all checks passed\n");
    return 0;
}
