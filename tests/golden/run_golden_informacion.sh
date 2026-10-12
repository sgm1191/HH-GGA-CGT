#!/usr/bin/env bash
# Golden-test harness for the informacion (adaptive) variant: runs the ORIGINAL
# gga-cgt-info binary (repo root) and the hh port (gga-cgt-info-hh) on the same
# (instance, config, seed) and diffs their Solutions_GGA-CGT/*.txt summary output.
#
# This needs a different (and more careful) setup than run_golden.sh, because the
# informacion variant's original binary has THREE confirmed, independent bugs that
# run_golden.sh's base-variant setup doesn't need to route around (see
# hh/CHECKLIST.md Recommendations for the full writeup of each):
#
#   1. LoadData() unconditionally prepends "data/" to the instance path (a bug
#      CHANGES.md already documented and fixed for the base file, apparently not
#      carried over to this separate source file) -- so the instance must be placed at
#      <workdir>/data/<basename>, with instances.txt containing only <basename>, NOT
#      the "data/..." path this repo's real instances.txt uses.
#   2. sendtofile()'s local `fil` buffer was shrunk to 30 bytes while the
#      strcpy/strcat chain that fills it needs ~48+ bytes for any real filename --
#      i.e. it deterministically stack-smashes and aborts whenever save_bestSolution=1.
#      This script always uses save_bestSolution=0 to get a clean, comparable run out
#      of the original at all.
#   3. Repeated_Fitness/Mutation_reps/Ratios_bins filenames are built from the FULL
#      instance path (directories included), not a basename -- fine here since we
#      already need a flat (no-subdirectory) instance path for bug #1 above, but it
#      means this harness can't validate a nested-path instance against the original
#      (hh's own port doesn't have this bug -- see io/paths.cpp -- so hh alone handles
#      nested paths fine; there is just nothing to diff it against for that case).
#   4. The informacion source deletes `realized_mutation += 1;` from its mutation loop,
#      so its "Mutations" column is always 0 regardless of how many mutations actually
#      ran. hh's port does NOT reproduce this (a dead, always-zero counter provides no
#      information to preserve) -- so this script excludes the Mutations field from
#      the comparison, by design, not because it's expected to match.
#
# Usage: same positional arguments as run_golden.sh.
#   hh/tests/golden/run_golden_informacion.sh <instance-path-relative-to-repo-root> \
#       <P_size> <max_gen> <p_m> <p_c> <k_ncs> <k_cs> <B_size> <life_span> <seed>

set -euo pipefail

if [ "$#" -ne 10 ]; then
    echo "Usage: $0 <instance> <P_size> <max_gen> <p_m> <p_c> <k_ncs> <k_cs> <B_size> <life_span> <seed>" >&2
    exit 2
fi

INSTANCE="$1"; P_SIZE="$2"; MAX_GEN="$3"; P_M="$4"; P_C="$5"
K_NCS="$6"; K_CS="$7"; B_SIZE="$8"; LIFE_SPAN="$9"; SEED="${10}"
BASENAME="$(basename "$INSTANCE")"

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/../../.." && pwd)"
HH_DIR="$REPO_ROOT/hh"

if [ ! -x "$REPO_ROOT/gga-cgt-info" ]; then
    echo "error: $REPO_ROOT/gga-cgt-info not found or not executable (build the original first)" >&2
    exit 1
fi
if [ ! -x "$HH_DIR/build/gga-cgt-info-hh" ]; then
    echo "error: $HH_DIR/build/gga-cgt-info-hh not found (run: cmake -S \"$HH_DIR\" -B \"$HH_DIR/build\" && cmake --build \"$HH_DIR/build\")" >&2
    exit 1
fi

WORKDIR="$(mktemp -d)"
trap 'rm -rf "$WORKDIR"' EXIT

# Original: needs data/<basename> (bug #1 above) and a config line with
# save_bestSolution=0 (bug #2 above).
mkdir -p "$WORKDIR/orig/data"
cp "$REPO_ROOT/$INSTANCE" "$WORKDIR/orig/data/$BASENAME"
echo "$BASENAME" > "$WORKDIR/orig/instances.txt"
printf 'CONF |P| max_gen p_m p_c k_ncs k_cs B_size life_span seed save_bestSolution\n1 %s %s %s %s %s %s %s %s %s 0' \
    "$P_SIZE" "$MAX_GEN" "$P_M" "$P_C" "$K_NCS" "$K_CS" "$B_SIZE" "$LIFE_SPAN" "$SEED" > "$WORKDIR/orig/configurations.txt"
cp "$REPO_ROOT/gga-cgt-info" "$WORKDIR/orig/"

# hh: flat basename, no "data/" prefix needed (that bug isn't reproduced).
mkdir -p "$WORKDIR/hh"
cp "$REPO_ROOT/$INSTANCE" "$WORKDIR/hh/$BASENAME"
echo "$BASENAME" > "$WORKDIR/hh/instances.txt"
printf 'CONF |P| max_gen p_m p_c k_ncs k_cs B_size life_span seed save_bestSolution\n1 %s %s %s %s %s %s %s %s %s 0' \
    "$P_SIZE" "$MAX_GEN" "$P_M" "$P_C" "$K_NCS" "$K_CS" "$B_SIZE" "$LIFE_SPAN" "$SEED" > "$WORKDIR/hh/configurations.txt"
cp "$HH_DIR/build/gga-cgt-info-hh" "$WORKDIR/hh/"

( cd "$WORKDIR/orig" && timeout 120 ./gga-cgt-info < /dev/null > run.log 2>&1 ) || true
( cd "$WORKDIR/hh" && timeout 120 ./gga-cgt-info-hh < /dev/null > run.log 2>&1 )

# The original's own while(!feof()) bug (Recommendations #2) still applies even with
# save_bestSolution=0 (it affects the config loop, unrelated to bug #2 above) -- take
# only the FIRST data row.
ORIG_ROW="$(grep "^$BASENAME" "$WORKDIR/orig/Solutions_GGA-CGT/GGA-CGT_(1).txt" | head -1)"
HH_ROW="$(grep "^$BASENAME" "$WORKDIR/hh/results/Solutions_GGA-CGT/GGA-CGT_(1).txt" | head -1)"

if [ -z "$ORIG_ROW" ]; then
    echo "error: original produced no summary row (see $WORKDIR/orig/run.log before it's cleaned up -- rerun without the trap to inspect)" >&2
    cat "$WORKDIR/orig/run.log" >&2
    exit 1
fi

echo "original: $ORIG_ROW"
echo "hh:       $HH_ROW"

# Fields: 0=instance 1=L2 2=bins 3=gen 4=time 5=delbins 6=delitems 7=crosses 8=mutations.
# Field 4 (time) and field 8 (mutations, see bug #4 above) are excluded by design.
ORIG_FIELDS=($ORIG_ROW)
HH_FIELDS=($HH_ROW)

MISMATCH=0
for i in "${!ORIG_FIELDS[@]}"; do
    if [ "$i" -eq 4 ] || [ "$i" -eq 8 ]; then continue; fi
    if [ "${ORIG_FIELDS[$i]}" != "${HH_FIELDS[$i]:-}" ]; then
        echo "MISMATCH at field $i: original='${ORIG_FIELDS[$i]}' hh='${HH_FIELDS[$i]:-<missing>}'" >&2
        MISMATCH=1
    fi
done

if [ "$MISMATCH" -eq 0 ]; then
    echo "MATCH (all fields except wall-clock time and the dead Mutations counter)"
    exit 0
else
    exit 1
fi
