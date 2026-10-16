# HH-GGA-CGT

Hyper-heuristic based on the **Grouping Genetic Algorithm with Controlled Gene
Transmission (GGA-CGT)** for the one-dimensional **Bin Packing Problem (BPP)**.

Given a set of items with integer weights and bins of fixed capacity, the goal is to
pack every item while using as few bins as possible. This project is a modular C++17
reimplementation of GGA-CGT. Its components (initializer, crossover, mutation and
parameter controller) are interchangeable, so you can compose different algorithm
variants from the same engine.

The adaptive variant implements the algorithm described in Amador-Larrea et al. (2026),
*An Adaptive Grouping Genetic Algorithm with Controlled Gene Transmission Based on
Fullness and Item Strategies (AGGA-CGT-FIS)*. The paper is in [docs/](docs/). If you
use this code, please cite that work (see [Citation](#citation)).

> **Status:** this repository is the baseline for my PhD research. It currently holds
> only the implementation of the published algorithm and contains no new contributions
> yet. I keep it here to track the starting point of the investigation.

## Functionality

### The algorithm

Each individual is a *grouping* solution: a list of bins, each holding a set of items.
Fitness rewards full bins (the average squared bin fullness), so the search prefers
solutions where a few bins are nearly empty and the rest are full.

The engine ([src/ga/gga_cgt_engine.cpp](src/ga/gga_cgt_engine.cpp)) runs this loop:

1. **Lower bound.** It computes the Martello–Toth lower bound `L2` for the instance
   ([src/core/lower_bound.cpp](src/core/lower_bound.cpp)). A run stops early as soon as
   a solution reaches `L2` bins, because that solution is provably optimal.
2. **Initial population.** It builds `|P|` individuals with First-Fit on random item
   permutations (large items are placed first).
3. **Each generation, until `max_gen` or until it finds the optimum:**
   - **Controlled selection.** Parents are chosen by tournament, with
     elite and non-elite pools.
   - **Gene-level crossover (FFD).** The best bins of both parents are inherited.
     Items that are left over are reinserted with First-Fit Decreasing.
   - **Controlled replacement.** Offspring replace duplicate or worse individuals.
     Elite solutions (`B_size`) are kept for at most `life_span` generations.
   - **Adaptive mutation (RP).** It removes the least-full bins
     (`k_cs` controls cloned solutions and `k_ncs` controls non-cloned ones), then
     repacks the freed items with the *Rearrangement by Pairs* heuristic.
   - The global best solution is updated.

### Two executables

| Binary | Entry point | Parameter control | Crossover |
|---|---|---|---|
| `gga-cgt-hh` | [src/app/main_base.cpp](src/app/main_base.cpp) | **Static.** `p_m`, `p_c`, `k_ncs` and `k_cs` are read from `configurations.txt` and stay constant. | Gene-level crossover + FFD |
| `gga-cgt-info-hh` | [src/app/main_informacion.cpp](src/app/main_informacion.cpp) | **Adaptive.** The parameters are recomputed every generation from population feedback ([src/control/adaptive_parameter_controller.cpp](src/control/adaptive_parameter_controller.cpp)). | Gene-level crossover + FFD with GLX-style Rearrangement by Pairs |

In the adaptive variant:
- `p_m` follows the accumulated fraction of non-repeated fitness values in the
  population.
- `p_c` follows the full-bin ratio of the best individual.
- `k_cs` decays linearly from 13.5 to 4.5.
- `k_ncs` follows a damped cosine schedule between 7.5 and 1.5
  ([src/control/schedules.hpp](src/control/schedules.hpp)).

The `p_m`, `p_c`, `k_ncs` and `k_cs` values in `configurations.txt` are only echoed
into the summary header. The variant does not use them.

### Project layout

```
src/
  core/       Instance, bins, chromosome, population, lower bound, RNG (Schrage)
  operators/  FF initializer, gene-level crossovers, adaptive mutation,
              Rearrangement by Pairs, controlled selection/replacement
  control/    Parameter controllers (static / adaptive) and schedules
  ga/         GgaCgtEngine (generation loop) and observer interface
  io/         Config/instance loaders, summary/solution writers, trace loggers
  app/        Composition roots (main_base.cpp, main_informacion.cpp)
tests/
  unit/       Unit tests (lower bound, operators, schedules, adaptive controller)
  golden/     Scripts that compare output against the original GGA-CGT binaries
docs/         Reference paper (AGGA-CGT-FIS) and its BibTeX entry (cite.bib)
```

## Requirements

- A C++17 compiler: GCC ≥ 8, Clang ≥ 7 or MSVC 2019+. Tested with GCC 11.4.
- CMake ≥ 3.16.
- A build tool such as `make` or Ninja.
- BPP instance files. These are not included in the repository (see
  [Instance file format](#instance-file-format)).

The project has no third-party dependencies.

## Compiling

```bash
cmake -S . -B build              # Release (-O2) is the default build type
cmake --build build -j
```

To get a debug build instead:

```bash
cmake -S . -B build-debug -DCMAKE_BUILD_TYPE=Debug
cmake --build build-debug -j
```

This produces:

- `build/gga-cgt-hh`: the static-parameter variant
- `build/gga-cgt-info-hh`: the adaptive-parameter variant
- `build/tests/test_*`: the unit tests

### Running the tests

```bash
ctest --test-dir build --output-on-failure
```

## Usage

The programs take no command-line arguments. They read two files from the
**current working directory**:

- `configurations.txt`
- `instances.txt`

They write all their output into a `results/` directory inside the current working
directory. Run them
from the folder that holds those files, for example the repository root:

```bash
./build/gga-cgt-hh          # static parameters
./build/gga-cgt-info-hh     # adaptive parameters
```

Every configuration line is run on every instance listed in `instances.txt`. The
terminal shows one progress line per instance when that instance finishes. A single
instance can take several seconds, so a run over many instances takes minutes. The
full results are written to the files described in [Output](#output).

```
[conf 1] (1/28) data/standarized_instances/Hard28/Hard28_BPP561.txt ... L2=72 bins=73 gen=500 time=6.59s
```

### `configurations.txt`

The file has one header line, followed by one line per configuration:

```
CONF |P| max_gen p_m p_c k_ncs k_cs B_size life_span seed save_bestSolution
1 100 500 0.83 0.0 1.3 4.0 0.1 10 1 0
```

| Field | Meaning |
|---|---|
| `CONF` | Configuration ID, used in output file names |
| `\|P\|` | Population size |
| `max_gen` | Maximum number of generations |
| `p_m` | Mutation rate (static variant only) |
| `p_c` | Crossover rate (static variant only) |
| `k_ncs` | Mutation intensity for non-cloned solutions (static variant only) |
| `k_cs` | Mutation intensity for cloned solutions (static variant only) |
| `B_size` | Elite fraction of the population protected by controlled replacement |
| `life_span` | Maximum number of generations an elite solution survives |
| `seed` | Random seed. Runs are fully reproducible for a given seed. |
| `save_bestSolution` | `1` writes the best solution's bin-by-bin detail, `0` skips it |

### `instances.txt`

The file lists instance paths, separated by whitespace, one per line. Paths are relative
to the working directory:

```
data/bpplib_benchmarks_formatted/BPP.5/BPP.5_100_23.txt
data/bpplib_benchmarks_formatted/BPP.5/BPP.5_1000_34.txt
```

### Instance file format

Each line starting with `//` is a label. The value on the next line is read:

```
//Number of items
100
//Bin Capacity
1000
//Optimal Solution
42
//Weight of Item
512
487
...
```

Weights are usually integers. If they are given as fractions of the bin capacity, the
loader rescales them to integers automatically.

### Output

All output directories are created automatically under `results/` in the working directory.

| Path | Variant | Contents |
|---|---|---|
| `results/Solutions_GGA-CGT/GGA-CGT_(<conf>).txt` | both | Summary with one row per instance: `L2`, bins used, generation where the optimum was found (`max_gen` if it was not found), time in seconds, deleted bins and items, and the number of crossovers and mutations performed |
| `results/Details_GGA-CGT/GGA-CGT_S_(<conf>)_<instance>` | both, if `save_bestSolution = 1` | Best solution: fullness, gap and items of each bin |
| `results/Repeated_Fitness/gene_detail_(<conf>)_<instance>` | both | Per-generation trace: repeated fitness counts, best, worst and average fitness |
| `results/Mutation_reps/gene_detail_muta(<conf>)_<instance>` | adaptive | Per-generation statistics for mutation and crossover, plus the adapted `p_c` and `p_m` |
| `results/Ratios_bins/gene_detail_ratio(<conf>)_<instance>` | adaptive | Full-bin ratios of the best, worst and average individuals |
| `results/Details_GGA-CGT/detalle_solucion_<instance>_conf(<conf>).txt` | adaptive | Population dump, written when the search stagnates |

## Golden tests

The scripts in [tests/golden/](tests/golden/) run the original GGA-CGT binaries and
this port on the same instance, configuration and seed, then diff the summary output.
They need the original `gga-cgt` / `gga-cgt-info` executables, which are not part of
this repository. The scripts were written for an older `hh/` subdirectory layout, so
adjust their paths before running them. Example:

```bash
tests/golden/run_golden.sh data/.../BPP.5_100_23.txt 20 30 0.83 0.2 1.3 4.0 0.1 10 12345
```

[CHECKLIST.md](CHECKLIST.md) records the refactoring progress and the known
differences from the original implementation.

## Citation

This code implements the algorithm from the following paper. If you use it in your
research, please cite the original work:

> S. Amador-Larrea, M. Quiroz-Castellanos, O. Ramos-Figueroa and A. Guerra-Hernández,
> "An Adaptive Grouping Genetic Algorithm with Controlled Gene Transmission Based on
> Fullness and Item Strategies (AGGA-CGT-FIS)," *Mathematical and Computational
> Applications*, vol. 31, no. 2, p. 34, 2026.

```bibtex
@article{amador2026adaptive,
  title={An Adaptive Grouping Genetic Algorithm with Controlled Gene Transmission Based on Fullness and Item Strategies (AGGA-CGT-FIS)},
  author={Amador-Larrea, Stephanie and Quiroz-Castellanos, Marcela and Ramos-Figueroa, Octavio and Guerra-Hern{\'a}ndez, Alejandro},
  journal={Mathematical and Computational Applications},
  volume={31},
  number={2},
  pages={34},
  year={2026},
  publisher={MDPI}
}
```

The same entry is in [docs/cite.bib](docs/cite.bib).

## License

This code is released under the [MIT License](LICENSE).
