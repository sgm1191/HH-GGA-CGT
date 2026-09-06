# Refactoring checklist — `hh/`

Tracks application of `../REFACTORING_STRATEGY.md` to a new, independent codebase in
this directory. **The original `.CPP`/`.h` files at the repo root are never modified** —
`hh/` is a from-scratch reimplementation that targets behavioral equivalence with them.

Legend: `[ ]` not started · `[~]` in progress / partial · `[x]` done

## Status at a glance

**Both binaries (`gga-cgt-hh` and `gga-cgt-info-hh`) are implemented, build with zero
warnings under `-Wall -Wextra`, and are golden-tested against the real `gga-cgt`/
`gga-cgt-info` binaries: 5 independent (instance, config, seed) cases, all matching
exactly** on every reproducible field of `Solutions_GGA-CGT/*.txt` (see Phase 9). Along
the way this found and fixed 5 real bugs in `hh/`'s own port (Recommendations #10-12,
#19-20) and documented 8 real bugs in the ORIGINAL codebase, several severe enough that
the original `gga-cgt-info` binary cannot process this repository's own instance data
without crashing (Recommendations #1-4, #18).

Open items: `tests/golden/*.sh` aren't wired into CTest (#16); the stagnation-dump
observer's exact output format is implemented but not golden-verified (no test case
triggered it); §7.8 parallelization is deliberately deferred until now (§7.8 itself
says to do this last); §7.9 copy-on-write bins is optional/deferred pending profiling.

---

## Phase 0 — Groundwork

- [x] Read `REFACTORING_STRATEGY.md` in full.
- [x] Read `GGA-CGT_cruzaFFDStepaphanie.CPP` (base) in full — algorithm, globals, I/O formats.
- [x] Read `linked_list.h` in full.
- [x] Extract the "informacion" (adaptive) variant's delta vs. base (adaptive-parameter
      formulas, extra loggers, stagnation dump, RNG call sites) without duplicating the
      full 3124-line read.
- [x] Confirm build tooling available (`g++` 11.4, `cmake`, `make`).
- [x] Identify dead code in the original: `Sort_Descending_RepeatedBinFullness`,
      `Sort_Descending_BinFullness`, `Sort_Ascending_Items`, `Sort_Ascending_Items_Fullness`,
      `Sort_Descending_BinFullness_Items_Weight` are declared/defined but **never called**
      in either `.CPP` file — not ported (see Recommendations).

## Phase 1 — §2 module layout skeleton

- [x] `hh/src/core/`, `hh/src/operators/`, `hh/src/control/`, `hh/src/ga/`, `hh/src/io/`,
      `hh/src/app/`, `hh/tests/unit/`, `hh/tests/golden/` directories created.
- [x] `hh/CHECKLIST.md` (this file) kept current after every phase — including going
      back to fix items that were checked inaccurately (Phase 6's I/O checkboxes were
      stale `[ ]` despite those files existing and being exercised by every golden run;
      corrected once flagged).

## Phase 2 — §3 core value types (replace the 6-fake-bins hack)

- [x] `core/types.hpp` — `ItemId`, `Weight`, `BinCapacity`, `Fitness` aliases.
- [x] `core/rng.hpp/.cpp` — `Rng` (Schrage `randp`/`get_rand`/`get_rand_ij`, faithful port),
      `shuffleRange` (= `Sort_Random`, including its `lighter_weight` tracking side effect,
      plus a no-tracking overload for the two call sites where that side effect is
      provably dead — see Recommendations), `RngStreams` bundling the run's three
      independent seed streams (verified by grep against the original: `seed` only feeds
      the crossover coin flip, `seed_permutation` only feeds `Sort_Random`, `seed_emptybin`
      only feeds the mutation bin-count draw).
- [x] `core/bpp_instance.hpp/.cpp` — `BppInstance` (items, weights, capacity, descending
      weight order, smallest-weight cache). `n_`/L2 live in `lower_bound.hpp` instead
      (they're LowerBound()'s output, not instance-loading output).
- [x] `core/bin.hpp` — `Bin` as `std::vector<ItemId>` + cached fullness (§7.1: no
      per-item heap nodes). Header-only, no `.cpp` needed.
- [x] `core/chromosome.hpp` — `Chromosome` struct: `bins`, `fitness`, `binsUsed`,
      `birthGeneration`, `fullBinCount`, `minBinFullness`, `Origin` enum — replaces
      `population[i][number_items + 0..5].Bin_Fullness` (field renamed from the doc's
      `maxSpareCapacity` to `minBinFullness` — see Recommendations for why). Header-only.
- [x] `core/population.hpp/.cpp` — `Population`: `std::vector<Chromosome>` sized to the
      *actual* `P_size` (§7.2, no `500×6000` worst-case arrays); dynamically sized bins.
- [x] `core/population.hpp/.cpp` — `sortByFitnessAndCountDuplicates()`: faithful port of
      `Sort_Ascending_IndividualsFitness`'s sort+relocate-duplicates-to-front dual
      behavior (§4.1) — **not** a bare `std::sort`.
- [x] `core/population_stats.hpp` — `PopulationStats` value type, computed once/gen (§7.4).
- [x] `core/lower_bound.hpp/.cpp` — Martello–Toth L2 + `n_` (port of `LowerBound()`),
      with two of the original's unguarded sentinel-array reads replaced by explicit
      bounds checks (documented deviation, see Recommendations).

## Phase 3 — §2 operators

- [x] `operators/operator_interfaces.hpp` — `IInitializer`, `ICrossover` (+
      `CrossoverCounters` for the deleted-bins/items run counters), `IMutation`.
- [x] `operators/ff_common.hpp/.cpp` — shared First-Fit placement (`firstFitInsert`, port
      of `FF`), used by the initializer, crossover's free-item reinsertion, and RP's tail
      reinsertion. Deliberately a near-literal port (not simplified) to preserve the
      original's exact floating-point fitness-summation order — see file header comment.
- [x] `operators/ff_initializer.hpp/.cpp` — `FFInitializer : IInitializer` (port of
      `FF_n_`). Found and documented a reproducibility-critical detail: the original's
      `permutation` buffer is shared/reshuffled across individuals, not rebuilt fresh —
      modeled as persistent instance state, not a per-call local.
- [x] `operators/gene_level_crossover.hpp/.cpp` — `GeneLevelCrossoverFFD : ICrossover`
      (port of `Gene_Level_Crossover_FFD`) — preserves the 50/50 `randp` branch order
      and the descending-fullness/item-count bin ordering.
- [x] `operators/rearrangement_by_pairs.hpp/.cpp` — `RearrangementByPairs` (port of `RP`)
      — the most intricate routine in the codebase (pairwise item-swap search with
      cascading non-breaking matches). Ported near-literally with values re-read fresh
      on every inner-loop iteration (not cached) because a matched-but-not-exact-capacity
      swap does NOT break the search and the next comparison must see the just-swapped
      items — caching would have silently changed behavior. Preserves the
      `higher_weight`/`lighter_weight` bookkeeping, including an apparent original quirk
      (see Recommendations).
- [x] `operators/adaptive_mutation.hpp/.cpp` — `AdaptiveMutationRP : IMutation` (port of
      `Adaptive_Mutation_RP`), built on `RearrangementByPairs`.
- [x] `operators/rearrangement_by_pairs_glx.hpp/.cpp` — **implemented and golden-tested.**
      Informacion-only, not in REFACTORING_STRATEGY.md's original inventory (see
      Recommendations #17's closing note on why). The informacion variant's crossover
      calls a SEPARATE function `RP_GLX` (not `RP`) on the child after transmission, not
      just First-Fit reinsertion like the base variant's crossover. `RP_GLX` operates on
      `children[]` instead of `population[]`, sorts free items ascending by weight
      unconditionally (no random/descending branch), has FOUR extra swap cases beyond
      `RP`'s three (all four combinations of {F[k], F[k2]} x {p, s}, a single free item
      replacing just one of the pair), and uses `free(s)` on a `new`-allocated node (a
      `new`/`free` mismatch bug CHANGES.md already fixed for the base file's `RP`,
      apparently reintroduced here — moot for this port, which never does manual node
      allocation). Two real bugs were found and fixed in THIS PORT during golden
      testing, both in this file — see Recommendations #19-20.
- [x] `operators/controlled_selection.hpp/.cpp` — `SelectionPools` (persistent
      `random_individuals`/`best_individuals`) + `reshuffleSelectionPools` (the two
      `Sort_Random` calls at the top of `Generation()`'s crossover-selection block).
- [x] `operators/controlled_replacement.hpp/.cpp` — `applyControlledReplacement`, the
      "Controlled replacement for crossover" copy loop. The mutation-side
      replacement/clone loop stays in `ga/gga_cgt_engine.cpp` (see Recommendations: it's
      interleaved with the mutation call and the per-individual L2 check closely enough
      that factoring it out added indirection without adding a real seam).

## Phase 4 — §2 control (parameter variation — the actual de-duplication point)

- [x] `control/parameter_set.hpp` — POD `{p_m, p_c, k_cs, k_ncs}`.
- [x] `control/parameter_controller.hpp` — `IParameterController::next(gen, stats)`.
- [x] `control/static_parameter_controller.hpp` — "base" variant: constant from config.
      Header-only.
- [x] `control/schedules.hpp` — `LinearDecaySchedule`/`DampedCosineSchedule`, confirmed
      against the informacion source directly (`git diff`-style comparison against the
      base file, read myself after a spec-extraction fork did not return within this
      session — see Recommendations #17): `k_cs` is exactly
      `LinearDecaySchedule(13.5, 4.5)` and `k_ncs` is exactly
      `DampedCosineSchedule(7.5, 1.5)` with the `15π/max_gen` angular frequency — these
      are the literal example values REFACTORING_STRATEGY.md §5 names, confirming both
      the doc's prior knowledge and this extraction. Unit-tested (`tests/unit/test_schedules.cpp`).
- [x] `control/adaptive_parameter_controller.hpp/.cpp` — **implemented and golden-tested.**
      `p_m`/`p_c` are NOT simple schedules like `k_cs`/`k_ncs` — see Recommendations
      #17 for the full formula derivation. Ported: `not_rep_pob`/`not_reps_acumul`/
      `umbral` running-average state for `p_m`; `calcular_prob_cruza_2`'s
      running-accumulator formula (with its `k_cs`-ordering dependency preserved) for
      `p_c`. Unit-tested against real values captured from the actual binary
      (`tests/unit/test_adaptive_parameter_controller.cpp`) and confirmed via two
      independent golden runs matching the original's `Solutions_GGA-CGT` output
      exactly (see Phase 9).

## Phase 5 — §2 GA engine (the framework itself)

- [x] `ga/ga_parameters.hpp` — POD `{P_size, max_gen, B_size, life_span, seed, …}`.
- [x] `ga/generation_observer.hpp` — `IGenerationObserver::onGeneration(stats, params)`.
- [x] `ga/gga_cgt_engine.hpp/.cpp` — owns `Population`, runs
      select→cross→replace→select→mutate→replace→check-optimal→notify, delegating every
      decision to injected operators/controller. **No file I/O, no globals** (§4.4: the
      five early-exit-on-optimal sites become one `checkOptimalAndRecord()` call).
      The single-call-site design for `IParameterController::next()` (called once,
      right after the pre-crossover sort) is now **confirmed correct** for the adaptive
      variant too: the extracted informacion spec (Recommendations #17) shows `p_m`,
      `p_c`, `k_cs`, and `k_ncs` are all recomputed at that exact same point in
      `Generation()`, nowhere else — resolves what was an open question
      (ex-Recommendation #13) when only the base variant had been ported.

## Phase 6 — §2 I/O (own translation units, no more inline `fopen`/`fprintf`)

- [x] `io/paths.hpp/.cpp` — `std::filesystem`-based path helpers (permanently fixes the
      `CHANGES.md` bug class: buffer overflow, double `data/` prefix, missing dirs).
      Compiled into `ggacgt_core`, exercised by every golden run (Phase 9) — every
      output file the base variant writes goes through this.
- [x] `io/instance_loader.hpp/.cpp` — port of `LoadData` → `loadInstance(path)`.
      Verified indirectly but thoroughly: every golden run loads a real `.txt` instance
      through this and reproduces the original's exact L2/behavior, which is only
      possible if weights/capacity were parsed correctly.
- [x] `io/config_loader.hpp/.cpp` — port of the `configurations.txt` parsing loop in
      `main()`. Verified the same way — every golden run's `P_size`/`max_gen`/etc came
      through this parser and matched the original's behavior exactly.
- [x] `io/instance_list_loader.hpp/.cpp` — port of the `instances.txt` parsing loop.
      Verified the same way, plus explicitly confirmed to NOT have the original's
      `while(!feof())` double-processing bug (Recommendations #2) by observing exactly
      one summary row per instance in every golden run vs. the original's two.
- [x] `io/summary_writer.hpp/.cpp` — port of `WriteOutput` (`Solutions_GGA-CGT/*.txt`),
      handle opened once per config run, not per call. Every field of its output
      (instance, L2, bins, generation, deleted-bins/items, crossovers, mutations) was
      diffed byte-for-byte against the original across all three golden runs.
- [x] `io/solution_writer.hpp/.cpp` — port of `sendtofile` (`Details_GGA-CGT/*.txt`).
      Confirmed to run without throwing (no over-capacity bin, i.e. every golden run's
      best solution is valid) across all golden runs; NOT diffed byte-for-byte against
      the original's detail file content, because the original's own duplicate-run bug
      (Recommendations #2) means its detail file reflects a DIFFERENT (second, RNG-
      drifted) run than the one whose summary line is comparable — there is nothing
      correct to diff against for this specific file with a single-config
      `configurations.txt`. This was root-caused during golden testing (see the base
      session's transcript), not merely assumed.
- [x] `io/generation_trace_logger.hpp/.cpp` — port of the inline `Repeated_Fitness/*`
      writer as an `IGenerationObserver`; **file opened once per run, not reopened every
      generation** (§7.5 fix). Header/row shape confirmed against the base file's exact
      fprintf strings; the `id_worst` UB (Recommendations #1) is fixed rather than
      reproduced. **Diffed field-by-field against a real `Repeated_Fitness/*.txt`** for
      the Hard28 golden run: row 0's `best`/`worst` fitness values didn't match at
      first, which turned out to be a THIRD file hit by Recommendations #2's
      duplicate-run bug (not previously identified) — `Repeated_Fitness/*.txt` is
      `fopen(...,"w+")`-truncated once per instance in `main()`, same as
      `Details_GGA-CGT/*.txt`, so with a single-config `configurations.txt` its content
      reflects the SECOND (duplicate, RNG-drifted) pass, not the first. Confirmed by
      instrumenting a scratch copy of the original to print the raw initial-population
      fitness of both passes: the original's row-0 "worst"/"best" values matched
      individuals 2 and 3 of the SECOND pass's initial population exactly, not anything
      in the first. `hh`'s single-pass trace is correct; there is no real divergence.
- [x] `io/adaptive_trace_logger.hpp/.cpp` — **implemented.** Writes both
      `Mutation_reps/*` and `Ratios_bins/*` from the same `onGeneration()` call (as the
      original does). Required extending `ICrossover`/`IMutation` with optional
      `hasLastCallStats()`/`lastCrossoverStats()`/`lastMutationStats()` methods (default
      no-op, so non-invasive to the base variant) and `IGenerationObserver` with a new
      `GenerationOperatorStats` parameter the engine fills by summing each call's stats
      across the generation — exactly the "real interface change" this was scoped as,
      not just a new observer. Two known-buggy original values are deliberately not
      reproduced: `Mutation_reps`' "Complete_Bins_M" column (original reads an
      uninitialized `m_bins`) and its "Proportion_bins_full_c" column (original writes
      an out-of-bounds `arr_acul_cruza[5]` as a truncated int) — see
      `operator_interfaces.hpp`'s `CrossoverStats`/`MutationStats` doc comments.
- [x] `io/adaptive_generation_trace_logger.hpp/.cpp` — **implemented**, a sibling of
      `GenerationTraceLogger` rather than a shared/flagged class (the informacion
      source's Repeated_Fitness write genuinely differs: an extra trailing "prom"
      column, and a different "worst" convention — see `PopulationStats::worstFitnessNonDuplicate`).
      Diffed field-by-field against the real `Repeated_Fitness/*.txt` output during
      this session's debugging (see Recommendations #19) — matches.
- [x] `io/stagnation_dump_observer.hpp/.cpp` — **implemented.** Triggered when
      `best_ratio_m == 0` (the best individual has zero full bins) — checked by the
      engine BEFORE that generation's own crossover/mutation run, via a new optional
      `IGenerationObserver::wantsStagnationDump()`/`onStagnationDetected(const
      Population&, GenerationNumber)` pair (the engine hands over the live `Population`
      by reference only when a registered observer actually wants it, so this doesn't
      cost anything on generations where nothing triggers). Dumps every population
      member's per-bin item lists plus a "bins llenos" summary to
      `Details_GGA-CGT/detalle_solucion_<instance>_conf(<n>).txt`, opened once and kept
      open across multiple triggers within a run (§7.5-style fix) rather than reopened
      in append mode every time as the original does — same observable file content.
      Not exercised by this session's golden runs (none of the tested cases happened to
      trigger the condition), so its exact dump *content* format is transcribed from
      the source but not independently diffed against a real triggered dump.

## Phase 7 — §2 composition roots

- [x] `app/main_base.cpp` — wires `GgaCgtEngine` + `StaticParameterController` +
      `SummaryWriter` + `SolutionWriter` + `GenerationTraceLogger`. ~70 lines, wiring
      only — confirmed no duplicated algorithmic logic by construction (it only calls
      into `ggacgt_core`, never touches a `Chromosome`/`Bin` field directly).
- [x] `app/main_informacion.cpp` — **implemented.** Wires `GgaCgtEngine` +
      `AdaptiveParameterController` + `AdaptiveGenerationTraceLogger` +
      `AdaptiveTraceLogger` + `StagnationDumpObserver` + `SummaryWriter` +
      `SolutionWriter`. NOT pure wiring on the exact same operators as
      `main_base.cpp` — it uses `GeneLevelCrossoverFFDGLX` (built on
      `RearrangementByPairsGLX`) instead of `GeneLevelCrossoverFFD`, per
      Recommendations #17's finding that the two variants differ at the operator level,
      not just the control/observer level. `AdaptiveMutationRP` (mutation) IS the exact
      same class the base variant uses — the informacion source's
      `Adaptive_Mutation_RP` still calls the same `RP`, unchanged.
- [x] Both composition roots confirmed working end-to-end via golden testing (Phase 9).

## Phase 8 — §8 build system

- [x] `hh/CMakeLists.txt` (per §8's own suggestion to move to CMake once the module
      count grows past a handful of files — it already has): `ggacgt_core` static
      library (core/operators/control/ga/io); `gga-cgt-hh` links against it.
      `ggacgt_adaptive` (the informacion-only pieces: `AdaptiveParameterController`,
      both extra loggers, `StagnationDumpObserver`, `GeneLevelCrossoverFFDGLX`,
      `RearrangementByPairsGLX`) links against `ggacgt_core` and backs
      `gga-cgt-info-hh` — kept as a separate library rather than folded into
      `ggacgt_core` so the base binary's link graph stays exactly what §2 describes
      (the base variant never needs RP_GLX or the adaptive controller compiled in).
- [x] Builds cleanly with `g++ 11.4`/`cmake` in this environment — verified with both
      default flags and `-Wall -Wextra`: **zero warnings** across `ggacgt_core`,
      `ggacgt_adaptive`, `gga-cgt-hh`, and `gga-cgt-info-hh`.
- [x] `tests/` wired in as a CMake subdirectory + CTest (`enable_testing()` / `add_test`
      in `hh/CMakeLists.txt` and `hh/tests/CMakeLists.txt`).
- [ ] Optional: `-fsanitize=address,undefined` debug target (replaces `gga-cgt-asan*`).
      Not added — straightforward to add (`add_compile_options`/`add_link_options`
      behind a `SANITIZE` cache option) whenever it's wanted; skipped for now since
      nothing in this session's testing needed it (no crashes/UB found in `hh/` itself).

## Phase 9 — §5 testing strategy

- [x] `tests/unit/test_operators.cpp` — operator invariants: crossover and mutation
      output checked, over multiple trials each, for no duplicate/lost items, no
      over-capacity bin, and `binsUsed == bins.size()`. Wired into `tests/CMakeLists.txt`
      + CTest (`ctest` in `hh/build`); passing.
- [x] `tests/unit/test_lower_bound.cpp` — `computeLowerBound()` against 4 hand-computed
      textbook cases (all-large, no-large exact-ceil, no-large exact-division, mixed
      general Martello-Toth branch). Passing.
- [x] `tests/unit/test_schedules.cpp` — schedule tests, pure functions of
      `(generation, max_gen)`: `LinearDecaySchedule(13.5, 4.5)` checked at generation 0,
      `max_gen`, and the midpoint; `DampedCosineSchedule(7.5, 1.5)` checked at
      generation 0 (cos²(0)==1, so it should sit at its ceiling) and clamped-into-range
      across the whole `[0, max_gen]` sweep. Passing.
- [x] `tests/unit/test_adaptive_parameter_controller.cpp` — `AdaptiveParameterController`
      checked against exact `p_m`/`p_c`/`k_cs`/`k_ncs` values captured from the real
      `gga-cgt-info` binary during golden-test debugging (not invented expected
      values), plus two structural properties (`p_m` stays constant across generations
      when `repeatedFitness` is always 0; `k_cs`/`k_ncs` are pure functions of
      `(generation, max_gen)` independent of population state). Passing.
- [x] Golden-test harness (base variant): `hh/tests/golden/run_golden.sh <instance>
      <P_size> <max_gen> <p_m> <p_c> <k_ncs> <k_cs> <B_size> <life_span> <seed>` —
      builds scratch working dirs for both the repo-root `gga-cgt` binary and
      `hh/build/gga-cgt-hh`, runs both, and diffs every field of the first
      `Solutions_GGA-CGT/*.txt` row except wall-clock time. A shell script rather than
      a CMake/CTest target since it needs the already-built original binary at the repo
      root, not just `hh/`'s own build.
- [x] Golden-test harness (informacion variant): `hh/tests/golden/run_golden_informacion.sh`,
      same argument shape, against `gga-cgt-info`/`gga-cgt-info-hh`. Needs a more
      involved setup than the base script because the ORIGINAL informacion binary has
      three confirmed, independent bugs of its own (see Recommendations #18) that the
      base variant doesn't have — the script routes around all three so a clean,
      comparable run is possible at all, and documents each one inline.
- [x] **Five golden cases run and diffed, all exact matches** on every reproducible
      field except wall-clock time (and, for the informacion cases, the confirmed-dead
      `Mutations` counter — see Recommendations #18):
      1. Base: `BPP.5_100_23.txt`, `P=20 max_gen=30 p_m=.83 p_c=.2 k_ncs=1.3 k_cs=4.0 B=.1 life_span=10 seed=12345`
         → found at Gen 0 (first crossover child already optimal).
      2. Base: same instance, `P=20 max_gen=8 p_m=.7 p_c=.35 seed=999` → also Gen 0.
      3. Base: `Hard28_BPP14.txt` (160 items), `P=15 max_gen=12 p_m=.7 p_c=.35 seed=555` →
         ran the full 12 generations without finding L2 (24 crossovers, **120
         mutations** — this exercises `RearrangementByPairs` heavily) and still matched
         exactly. Strong evidence the RNG threading, controlled selection/replacement,
         and RP port are correct, not just the easy early-exit path.
      4. Informacion: same Hard28 instance/config as #3 but through
         `AdaptiveParameterController`/`GeneLevelCrossoverFFDGLX`/`RearrangementByPairsGLX`
         → also ran the full 12 generations (84 crossovers this time, since `p_c` is
         adaptive and starts near 1.0) and matched exactly on L2/bins/generation/
         DelBins/DelItems/Crosses.
      5. Informacion: `BPP.5_100_23.txt`, `P=25 max_gen=15 B=.15 life_span=5 seed=777` →
         found at Gen 0, matched exactly.
      Two real bugs were found and fixed in the base-variant port via this process
      (Recommendations #1-#2); THREE more were found and fixed specifically while
      getting the informacion cases (#4-#5) to match — two in `RearrangementByPairsGLX`
      alone (Recommendations #19-20) plus a same-shaped fix already covered by #12 for
      the shared counter-accumulation code path. The golden harness earned its keep
      again, more than once.

## §7 speed/memory items (folded into the fresh implementation, not a later swap step)

Because `hh/` is new code rather than an edit of the original, §6's "swap data
structures only after correctness is re-validated" ordering is inverted here: the
vector-based `Bin`/dynamically-sized `Population` are built in from the start (§7.1,
§7.2), and validated against golden output once the golden harness (Phase 9) is in place
— see Recommendations for why this changes the risk profile.

- [x] §7.1 `Bin` as contiguous `std::vector`, no per-item `new`/`delete` — `core/bin.hpp`.
      Exercised by every golden run, including the 12-generation Hard28 case with heavy
      crossover/mutation churn.
- [x] §7.2 `Population`/`Chromosome` sized to actual instance/`P_size`, not `500×6000` —
      `core/population.hpp`/`core/chromosome.hpp`. Confirmed indirectly: every golden
      run's `P_size` (15, 20, 25) and instance size (100-160 items) are all far below
      the original's `500`/`6000` compile-time maximums, and nothing in `hh/` has a
      compile-time bound on either — sizes flow entirely from `GaParameters`/`BppInstance`.
- [x] §7.3 Deterministic-key sorts (`Sort_Descending_Weights`, `Sort_Ascending_BinFullness`,
      `Sort_Descending_BinFullness_Items`) use `std::stable_sort` — safe because these
      comparators are pure key comparisons (no RNG in the comparator), so the sorted
      *result* is identical to the bubble sort's, only the comparison count differs.
      `Sort_Ascending_IndividualsFitness` is **not** upgraded this way (see Phase 2).
      **Verified**, not just implemented: the 12-generation Hard28 golden run (Phase 9)
      exercises all three of these sorts under real selection pressure and still
      matches the original exactly.
- [x] §7.4 `PopulationStats` computed once per generation, passed by `const&`.
- [x] §7.5 Loggers open their file once per run (`std::ofstream` member), not per generation.
- [x] §7.6 `std::filesystem::path` for all path building, no `sprintf`/`strcat`.
- [x] §7.7 `std::move` used where the source population slot is discarded (child/mutant
      replacing a population member) instead of deep clone (e.g. `children.push_back(std::move(child))`,
      `Population::at()` assignment in the engine's replacement/mutation steps).
- [ ] §7.8 Parallelization — **deferred**, see Recommendations (must not start before
      Phase 9's golden harness passes, per the strategy doc's own ordering — it now
      has, for the cases tested, so this is the natural next increment).
- [ ] §7.9 Copy-on-write bins — **deferred/optional**, only worth it if profiling after
      §7.1 shows bin cloning is still a hotspot (per strategy doc).

---

## Recommendations / follow-ups

### Bugs found in the ORIGINAL codebase (not `hh/` bugs — flagging for awareness)

1. **`Repeated_Fitness/*.txt`'s last column (`id_worst`) is undefined behavior in every
   historical run of the original binaries.** `Generation()`'s per-generation write has
   a misplaced parenthesis:
   ```cpp
   fprintf(output_gen, "...%d", ..., (int)dif_worst), (int)id_worst;
   ```
   The closing paren lands right after `dif_worst`; `(int)id_worst` after the comma is
   the comma operator, not an 11th fprintf argument — so the format string's final `%d`
   reads a vararg that was never supplied. There is no deterministic "correct" garbage
   to reproduce. `io/generation_trace_logger.cpp` fixes the call (passes `id_worst` for
   real) rather than reproducing unreproducible UB — see that file's header comment.
   **If any published analysis reads the last column of any `Repeated_Fitness/*.txt`
   file, that column should be treated as meaningless.**

2. **`main()`'s configuration loop runs the last configuration's (instance, config)
   pairing twice**, due to the classic `while(!feof(f))` C anti-pattern: `feof()` only
   becomes true *after* a failed read, so after the last config line is consumed
   successfully, the loop body runs one more time with `fscanf` failing silently and
   every config variable retaining its *stale value from the previous iteration* — for
   a single-config `configurations.txt` (the common case), this means the whole
   instance list gets processed twice, with the second pass continuing from wherever
   the first pass left the shared RNG state (`seed` is never reset between the two
   passes, only `seed_permutation`/`seed_emptybin` are). This produces two rows in
   `Solutions_GGA-CGT/*.txt` per instance instead of one, and — because
   `Details_GGA-CGT/*.txt` **and `Repeated_Fitness/*.txt`** are both opened with
   `"w+"` (truncate) once per instance in `main()` — **the second pass's detail file
   AND generation trace both silently overwrite the first pass's**, so neither the
   saved "best solution" detail nor the per-generation trace necessarily correspond to
   the summary line's first (real) result. Confirmed for `Repeated_Fitness` too during
   this session's informacion work: diffing `hh`'s single-pass trace against the
   original's for the Hard28 golden case showed the original's row 0 "worst"/"best"
   values matching individuals from the SECOND pass's initial population, not the
   first (see Phase 6).
   `hh/`'s `io/config_loader.cpp` and `io/instance_list_loader.cpp` use ordinary
   `while (in >> x)` loops, which don't have this failure mode — confirmed during golden
   testing (`hh/` produces exactly one summary row per instance where the original
   produces two, with the two original rows differing in `DelBins`/`DelItems`/`Mutations`).
   **This means the original's own output files can't be fully trusted for anything that
   assumes one row = one run**, and any already-published analysis that used the
   *second* duplicate row (or the detail file, which always reflects the second pass
   when a single config is used) should be re-examined.

3. **Several bubble-sort functions are dead code** in both `.CPP` files:
   `Sort_Descending_RepeatedBinFullness`, `Sort_Descending_BinFullness`,
   `Sort_Ascending_Items`, `Sort_Ascending_Items_Fullness`,
   `Sort_Descending_BinFullness_Items_Weight`, and `get_rand_ij` — declared, defined,
   never called from anywhere in either file (confirmed by grepping every call site).
   Not ported to `hh/`. Worth deleting from the originals as a zero-risk cleanup
   independent of this refactor, if anyone wants the diff to be smaller later.

4. **`FF_n_`'s trailing `FF(permutation[j], ..., is_last=1)` call is unconditional even
   when the "small items" pool is empty** (`n_ == number_items`, i.e. every item is
   "large"). In that case it reads `permutation[0]` uninitialized/stale. Reachable only
   for a pathological instance where every item exceeds half the bin capacity.
   `operators/ff_initializer.cpp` guards this (skips the trailing FF call when there are
   no small items) rather than reproducing the read — documented in that file.

### Deviations made deliberately in `hh/` (all documented in-place; summarized here)

5. **`Chromosome::minBinFullness`** is named for what the original's slot literally
   stores (the fullness of the least-full bin seen), not
   REFACTORING_STRATEGY.md §3's suggested name `maxSpareCapacity` (which describes the
   *derived* quantity every read site computes via `capacity - thatValue`). See
   `core/chromosome.hpp`.
6. **`LowerBound()`'s two sentinel-array reads** (`ordered_weight[number_items] =
   number_items; weight[number_items] = 0;` used to make an unguarded `while` safe) are
   replaced with explicit bounds checks in `core/lower_bound.cpp` — behaviorally
   identical for any instance with strictly positive weights (true for every format this
   codebase reads), and doesn't depend on the original's global-array reuse between
   instances.
7. **An apparent original RNG quirk is reproduced, not fixed**: `Sort_Random`'s
   `lighter_weight`-tracking side effect does a `weight[array[i]]` lookup regardless of
   whether `array` actually holds item ids. Two call sites pass it arrays of BIN indices
   (`RearrangementByPairs`'s `ordered_BinFullness`) — so the side effect corrupts
   `lighter_weight` with a nonsense "weight of item N" reading for bin index N — and
   this value is later genuinely read (`RP`'s `if(lighter_weight > capacity - ...)`
   branch), so it's not dead code there; it's reproduced exactly (`rearrangement_by_pairs.cpp`).
   Two OTHER call sites (the persistent `random_individuals`/`best_individuals`
   selection pools) pass it arrays of INDIVIDUAL indices, where `weight[]` isn't even
   safely sized for that (an individual index can exceed `numberOfItems()` for a small
   instance with a large `P_size`) — but there the side effect's output is provably dead
   (`RP()` always overwrites the global `lighter_weight` before ever reading it), so
   `core/rng.hpp` provides a second `shuffleRange` overload without the tracking for
   those two call sites specifically, avoiding an out-of-bounds read that would
   otherwise be reachable. See `core/rng.hpp`'s doc comments.
8. **`sendtofile`'s non-fatal accumulated-weight-mismatch warning** (`printf` +
   `getchar()`, no exit) is not reproduced as an exception in `io/solution_writer.cpp` —
   only the *fatal* over-capacity case throws (after the file is fully written, matching
   the original's post-write `exit(1)`). The interactive `getchar()` prompts throughout
   the original (there are several) are dropped everywhere as debugging leftovers
   inappropriate for a batch/library context — none of them gate any file-observable
   behavior.
9. **`ATTRIBUTES`-based size guards are gone** (`LoadData`'s `number_items > ATTRIBUTES
   - 6` check, the `500×6000` worst-case population arrays) — a direct, intended
   consequence of §7.2's dynamic sizing, not something requiring a runtime check anymore.

### Bugs found AND fixed during `hh/`'s own development (via golden testing)

10. **Crossover's fullness-tie item-count tie-break was inverted** in the first draft of
    `operators/gene_level_crossover.cpp`: the original's bubble-sort swap
    (`if(items1_bin > items2_bin) swap`) pushes higher-item-count bins to *later*
    positions, meaning ties break by ASCENDING item count — the first draft's
    comparator had `>` where it needed `<`. Found by bisecting a golden-test divergence
    (initial population and first parent selection matched bit-for-bit; the crossover
    of those identical parents didn't) down to comparing the shuffled+sorted bin-order
    arrays directly against an instrumented scratch copy of the original. Fixed; the
    golden tests below reflect the fix.
11. **The same file's `Sort_Descending_BinFullness_Items` port sorted the WHOLE
    `counter`-sized array instead of just the calling parent's own bin-count prefix** —
    the original bounds this sort by the parent's own bin count, not by
    `max(bins1, bins2)` (which is what the shared `order1`/`order2` arrays are sized
    to). Harmless when both parents happen to have equal bin counts (true in every
    golden case run so far, since they all diverged from a common initial population),
    but reads `parent.bins[i]` out of range whenever the two crossing parents have
    different bin counts — very likely to happen naturally as individuals evolve over
    many generations. Fixed by bounding the sort to `binIndices.begin() + count`.
12. **A run-level counter ordering bug in `ga/gga_cgt_engine.cpp`**:
    `checkOptimalAndRecord()` snapshots the engine's running `deletedBins_`/
    `deletedItems_` totals into the returned `RunResult`, but the per-crossover
    `CrossoverCounters` were only being folded into those running totals *after* the
    early-return check — so a run that found its optimal solution during its very first
    crossover reported `DelBins=0 DelItems=0` regardless of what that crossover
    actually rejected. Found via golden testing (matched on Gen/Crosses/Mutations but
    not on DelBins/DelItems). Fixed by fully accumulating into `deletedBins_`/
    `deletedItems_` (and resetting the per-crossover counter) before the optimal check,
    every iteration.

### What's still open

13. **RESOLVED** (was: "the `IParameterController::next()` call site's exact position
    relative to selection/crossover/mutation is unverified against the informacion
    variant"). The extracted spec (Recommendations #17) confirms all four parameters
    are recomputed at the same single point in `Generation()` that `hh`'s engine
    already calls `next()` from — see Phase 5.
14. **`applyControlledReplacement`'s "last generated child can be silently uncopied"
    quirk** (Recommendation-worthy original behavior, see that file's header comment) —
    reproduced faithfully, not fixed, since it's plausibly load-bearing for
    reproducibility even though it looks unintentional.
15. **§7.8 parallelization** is unstarted. The golden harness now passes for the cases
    tested, which is the strategy doc's own precondition for starting it (§7.8: "do this
    last, after the correctness-preserving structural refactor is validated"). Next
    natural increment if this work continues.
16. **`tests/golden/run_golden.sh` exists and passes on all three cases tested, but
    isn't wired into CTest** (it needs the repo-root `gga-cgt` binary to already be
    built, which `hh/`'s own CMake build doesn't produce — a CTest target would need a
    `configure`-time check or an explicit prerequisite build step for the original).
    It already handles Recommendation #2 correctly (compares only the first summary
    row). Worth adding a small fixed suite of (instance, config, seed) triples run
    through it in CI if this becomes a maintained project.
17. **Full spec for the informacion-variant work, extracted by reading `diff -u`
    between the two original `.CPP` files myself, and since then IMPLEMENTED and
    golden-tested** (a spec-extraction fork was launched early in this session to do
    this without spending my own context on a 3124-line read, but it did not return
    within the session; the extraction below was done directly instead and has since
    been cross-checked against real program behavior via golden testing, which is a
    stronger validation than the fork would have provided on its own):

    - **`p_m` (adaptive)**: `not_rep_pob = ((P_size - repeated_fitness) / P_size) ^ 1`
      (the `^1` is literal in the source — a no-op, presumably a leftover from tuning,
      not reproduced as an actual `pow()` call); `not_reps_acumul += not_rep_pob`
      (running sum across the whole run); `umbral = not_reps_acumul / (generation + 1)`
      (running average); `p_m = (int)roundTo002(umbral * (P_size - B_size)) / 100`
      (note the `(int)` truncation of `roundTo002`'s result -- easy to miss since
      `roundTo002` is normally applied to values already in [0,1], but here the
      argument is on the scale of `P_size`, so the truncation is load-bearing, not a
      no-op). **Implemented**: `control/adaptive_parameter_controller.cpp`.
      Golden-tested and unit-tested against real captured values.
    - **`p_c` (adaptive)**: computed via `calcular_prob_cruza_2(P_size, 1 -
      best_ratio_m, &acumulador_out)`, where `best_ratio_m` is the CURRENT best
      individual's `fullBinCount / binsUsed` ratio (computed earlier in the same
      `Generation()` call, from the population state BEFORE this generation's own
      crossover/mutation). `ri = clamp01(1 - best_ratio_m) ^ (1/k_cs)` (uses THIS
      generation's just-computed `k_cs` -- an ordering dependency, preserved);
      `acumulado += ri` (running sum, persisted across the whole run); `p_c =
      clamp01(roundTo002(acumulado / (generation + 1)))`. **Implemented and
      golden-tested**, same file.
    - **`k_cs`/`k_ncs`**: exactly `LinearDecaySchedule(13.5, 4.5)` /
      `DampedCosineSchedule(7.5, 1.5)` — implemented, see Phase 4. All four parameters
      confirmed computed at the SAME single point in `Generation()`, resolving what was
      an open question (ex-#13) before the informacion side was implemented.
    - **`RP_GLX`**: crossover-side rearrangement variant, called instead of plain
      First-Fit reinsertion for the child's leftover items. **Implemented**:
      `operators/rearrangement_by_pairs_glx.cpp` — see Phase 3, and Recommendations
      #19-20 for two real transcription bugs found and fixed here via golden testing
      (this function turned out to be even more intricate than the spec extraction
      captured: it has SEVEN swap cases, not five, and one of them can mutate state and
      fall through to the next check without breaking, both of which the first
      implementation attempt got wrong).
    - **`Mutation_reps/gene_detail_muta(<conf>)_<instance>`**: header
      `Gene\tComplete\tIncomplete\tDelete\tBinsSolPartialM\tItemsFreeM\tBinsSolPartialC\tItemsFreeC\tSize_father_B\tSize_father_prom\tComplete_Bins_M\tComplete_Bins_C\tP_c\tProportion_bins_full_c\tP_m`,
      one row/generation, values averaged from accumulator arrays that
      `Adaptive_Mutation_RP`/`Gene_Level_Crossover_FFD` fill via added `int*` output
      parameters in the original informacion variant (a real signature difference from
      the base variant's versions, not just extra logging bolted on after the call).
      **Implemented**: required extending `ICrossover`/`IMutation` with optional
      per-call stats-reporting methods and `IGenerationObserver` with a new
      per-generation accumulated-stats parameter — see Phase 6.
    - **`Ratios_bins/gene_detail_ratio(<conf>)_<instance>`**: header
      `Gene\tRatio_best_M\tRatio_worst_M\tRatio_prom_M\tRatio_best_C\tRatio_worst_C\tRatio_prom_C`,
      one row/generation. **Confirmed the "_M" and "_C" triples are pairwise
      numerically identical in the original** (traced `best_ratio_c`/`worst_ratio_c`/
      `average_ratio_c`'s formulas in the source directly -- they turn out to compute
      the exact same thing as their "_m" counterparts, evidently unintentionally, likely
      leftover from an edit that never differentiated them) — **implemented**
      accordingly, computing each pair once. See `PopulationStats`.
    - **`Repeated_Fitness` header differs from the base variant**: adds a trailing
      `\tprom ` column (population-average fitness) AND uses a different "worst"
      individual convention (`ordered_population[repeated_fitness]`, not
      `ordered_population[0]`). **Implemented as a sibling class**,
      `io/adaptive_generation_trace_logger.cpp`, not a flag on `GenerationTraceLogger`
      (see `PopulationStats::worstFitnessNonDuplicate`'s doc comment for the full
      reasoning) — confirmed field-by-field against a real `Repeated_Fitness/*.txt`
      output during this session's debugging (see Phase 6).
    - **Stagnation dump**: triggers when `best_ratio_m == 0` after mutation (i.e. the
      current best individual has NO fully-packed bins) — appends (not overwrites) a
      full per-bin item dump of the triggering individual plus a whole-population
      "aptitud + bins llenos" summary to
      `Details_GGA-CGT/detalle_solucion_<instance-without-extension>_conf(<confId>).txt`.
      **Implemented**: `io/stagnation_dump_observer.cpp`, via a new optional
      `IGenerationObserver::onStagnationDetected(const Population&, GenerationNumber)`
      hook the engine calls (only when a registered observer wants it) right before
      that generation's own crossover/mutation run, matching the original's timing
      exactly. Not exercised by any golden run this session (the condition never
      triggered in the cases tested), so the dump's exact byte-for-byte content format
      is transcribed carefully but not independently verified against a real trigger.
    - This confirmed a real finding about REFACTORING_STRATEGY.md's own §2 framing
      ("differing only in which parameter controller and which observers get plugged
      in"): the informacion variant genuinely needed a different crossover-side
      rearrangement operator (`RP_GLX`) and interface extensions on `ICrossover`/
      `IMutation`, not just a different control/observer composition. `hh/` handled
      this by extending the operator interfaces non-invasively (optional methods,
      default no-op) rather than by widening REFACTORING_STRATEGY.md's original
      "engine depends only on `ICrossover`/`IMutation`" contract for the common case.

18. **Three independent, confirmed bugs make the ORIGINAL `gga-cgt-info` binary unable
    to process this repository's own `instances.txt`/`configurations.txt` as-is** —
    found while building the informacion golden-test harness, each pinned down via
    `gdb`/instrumented scratch copies, not guessed:
    - `LoadData()` still has the exact double-`data/`-prefix bug CHANGES.md documented
      and fixed for the BASE file (`sprintf(string, "data/%s", file)` instead of
      `sprintf(string, "%s", file)`) — this separate source file evidently never
      received that fix. Any instance path from this repo's real `instances.txt`
      (already `"data/..."`-prefixed) becomes `"data/data/..."` and fails to load.
    - `sendtofile()`'s local `fil` buffer was shrunk to 30 bytes (from the base file's
      600) while the `strcpy`/`strcat` chain that fills it (`"Details_GGA-CGT/GGA-CGT_S_("`
      alone is 28 characters) needs 48+ bytes for any real filename — a deterministic
      stack buffer overflow, confirmed via `gdb` backtrace
      (`__strcat_chk` → `sendtofile` → `WriteOutput` → `main`) and glibc's
      `*** buffer overflow detected ***` abort. Triggers on every run with
      `save_bestSolution=1` — i.e. save-best-solution mode is unconditionally broken in
      the shipped binary.
    - `Repeated_Fitness`/`Mutation_reps`/`Ratios_bins` filenames are built from the
      FULL instance path (directories included) rather than a basename — the
      `file_base` extraction present in the base file appears to have been removed
      entirely in this source, not just left unfixed — so any nested instance path
      (again, everything in this repo's real `instances.txt`) makes these `fopen`
      calls fail; unlike `sendtofile`, none of these three call sites check for a NULL
      `FILE*` before `fprintf`-ing into it, so this manifests as a segfault.
    - Consequence: **none of this repository's real instance data can be run through
      the original `gga-cgt-info` binary without hitting at least one of these bugs**,
      which is why `tests/golden/run_golden_informacion.sh` needs the elaborate
      workaround setup it has (flat `data/<basename>` layout, `save_bestSolution=0`)
      just to get a comparable, non-crashing run out of the reference binary at all.
      `hh/`'s informacion port has none of these three bugs (same fixes as the base
      variant throughout its I/O layer).
    - Separately (not a crash, just dead code): the informacion source deletes
      `realized_mutation += 1;` from its mutation loop, so its `Solutions_GGA-CGT`
      "Mutations" column is always 0 regardless of how many mutations actually ran.
      `hh/`'s port does not reproduce this (a permanently-zero counter carries no
      information to preserve) — `AdaptiveMutationRP`'s mutation count is tracked
      correctly and will legitimately differ from the original's column, by design.
      The golden scripts exclude this field from comparison for exactly this reason.

19. **Bug found and fixed in `hh/`'s own `RearrangementByPairsGLX` (via golden
    testing): case 3's swap can mutate the bin/free-pool and fall through to the next
    case WITHOUT breaking, but the first implementation cached `pItem`/`sItem`/`pW`/
    `sW`/`fk`/`fk2` once per k2-loop iteration** instead of re-reading them fresh
    before each case check. The original dereferences live pointers (`p->data`,
    `s->data`), so a case-3 swap that doesn't hit exact capacity is immediately visible
    to the cases checked right after it in the same iteration; the cached-copy version
    saw stale pre-swap values instead. `RearrangementByPairs` (plain `RP`) doesn't have
    this failure mode since none of its three cases fall through without breaking —
    this is a genuinely GLX-specific pitfall, not something the already-validated `RP`
    port would have caught. Found by bisecting a golden-test divergence down to
    `RearrangementByPairsGLX::rearrange()`'s post-pairwise-loop state (`total_free`,
    `number_free_items`, `binI`, `higher_weight`, `lighter_weight` all matched the real
    binary exactly; the actual bin fullness values and free-item-pool contents didn't).
    Fixed by re-reading every value fresh immediately before each case's condition.

20. **A second, larger bug found in the same function immediately after fixing #19**:
    the original has FOUR single-item replacement cases, not two -- ALL FOUR
    combinations of {F[k], F[k2]} x {p, s} (`F[k]`→p, `F[k]`→s, `F[k2]`→p, `F[k2]`→s).
    The first implementation only had two of these (`F[k]`→p and `F[k2]`→s), having
    apparently pattern-matched the "two extra cases beyond RP" summary in the original
    spec extraction (Recommendation #17, before this fix) too literally instead of
    re-deriving each case from the source. Found by continuing the golden-test
    bisection into the source's raw `RP_GLX` text line by line and discovering two
    more `ban = 1;` sites this port had never accounted for. Fixed by implementing all
    four cases in the source's exact order (case 4 through case 7, after RP's three).
    This — combined with #19 — was the actual root cause of the informacion golden
    tests failing to match; after both fixes, all informacion golden cases matched the
    real binary exactly (see Phase 9).

21. **Long double/float mixed-precision arithmetic** (the adaptive-mutation bin-count
    formula, `LowerBound()`'s fmod/ceil checks, `AdaptiveParameterController`'s `pow()`
    calls) was ported with the same *types* as the original at each step, but exact
    bit-for-bit floating-point equivalence across compilers/optimization levels was not
    exhaustively re-verified beyond the five golden runs in Phase 9 — those passed on
    this machine's `g++ 11.4 -O2`, but a different compiler/platform is not guaranteed
    to agree to the last bit on this arithmetic.
