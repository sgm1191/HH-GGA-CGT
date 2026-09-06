#pragma once

#include "chromosome.hpp"
#include "types.hpp"

namespace ggacgt {

// Population-wide statistics for one generation, computed exactly once
// (REFACTORING_STRATEGY.md §7.4 -- the original recomputes an equivalent accumulation
// up to four times per generation at different call sites in Generation()). Passed by
// const& to the parameter controller and to every observer.
struct PopulationStats {
    GenerationNumber generation = 0;

    // Duplicate-fitness counts from Population::sortByFitnessAndCountDuplicates(), read
    // at the two points the original reads `repeated_fitness`: right after the
    // pre-crossover sort (`ind_rep_a_c`) and right after the pre-mutation sort
    // (`ind_rep_d_c`).
    long repeatedFitnessBeforeCrossoverSelection = 0;
    long repeatedFitnessAfterCrossoverSelection = 0;

    Fitness worstFitness = 0.0;
    GenerationNumber worstBirthGeneration = 0;
    Chromosome::Origin worstOrigin = Chromosome::Origin::Initial;

    Fitness bestFitness = 0.0;
    GenerationNumber bestBirthGeneration = 0;
    Chromosome::Origin bestOrigin = Chromosome::Origin::Initial;

    // Population-wide average fitness. Unused by the static ("base") variant; computed
    // unconditionally anyway since it's a single O(P_size) pass.
    Fitness averageFitness = 0.0;

    // "Full-bin ratio" (fullBinCount / binsUsed) for the best/worst/average individual
    // -- the informacion variant's `best_ratio_m`/`worst_ratio_m`/`average_ratio_m`.
    // Feeds both AdaptiveParameterController's p_c formula (best) and the
    // Ratios_bins/ trace (all three). Unused by the base variant, computed
    // unconditionally for the same reason as averageFitness above.
    //
    // IMPORTANT: `worstFullBinRatio` uses the SAME individual as
    // `worstFitnessNonDuplicate` below, NOT the same individual as `worstFitness`
    // above -- see that field's comment.
    double bestFullBinRatio = 0.0;
    double worstFullBinRatio = 0.0;
    double averageFullBinRatio = 0.0;

    // The informacion variant's Generation() computes its OWN "worst" using
    // `ordered_population[repeated_fitness]`, not `ordered_population[0]` like the
    // base variant (confirmed against both sources; the base variant's `worstFitness`
    // field above is golden-tested against `ordered_population[0]` and must not
    // change). `repeated_fitness` individuals with the exact-worst fitness get
    // relocated to the front by Population::sortByFitnessAndCountDuplicates(), so
    // `ordered_population[repeated_fitness]` is the first individual past that
    // duplicate block -- i.e. the informacion variant's Repeated_Fitness "worst"/
    // "id_worst"/"gen_worst"/"dif_worst" columns and Ratios_bins' "worst_ratio_m"
    // both skip the duplicate block that the base variant's do NOT. Used only by
    // the informacion-side loggers/controller; computed unconditionally like the
    // rest of this struct.
    Fitness worstFitnessNonDuplicate = 0.0;
    GenerationNumber worstBirthGenerationNonDuplicate = 0;
    Chromosome::Origin worstOriginNonDuplicate = Chromosome::Origin::Initial;
};

} // namespace ggacgt
