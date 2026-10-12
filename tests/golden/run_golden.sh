#!/usr/bin/env bash
# Golden-test harness: runs the ORIGINAL gga-cgt binary (repo root) and the hh port
# (gga-cgt-hh) on the same (instance, config, seed) and diffs their
# Solutions_GGA-CGT/*.txt summary output.
#
# NOTE on the original's duplicate-run bug (see hh/CHECKLIST.md Recommendations #2):
# main()'s configuration loop uses `while(!feof(f))`, which re-processes the LAST
# config line a second time with stale globals whenever configurations.txt has exactly
# one config line (the common case, and what this script uses). That produces TWO rows
# per instance in the original's Solutions_GGA-CGT/*.txt (and its Details_GGA-CGT/*.txt
# ends up holding the SECOND, buggy run's solution, not the first). This script only
# compares the FIRST row per instance -- the real, single, reproducible run -- against
# hh's single row, which is the only fields the original's own bug lets us trust.
#
# Usage:
#   hh/tests/golden/run_golden.sh <instance-path-relative-to-repo-root> \
#       <P_size> <max_gen> <p_m> <p_c> <k_ncs> <k_cs> <B_size> <life_span> <seed>
#
# Example:
#   hh/tests/golden/run_golden.sh \
#       data/bpplib_benchmarks_formatted/BPP.5/BPP.5_100_23.txt 20 30 0.83 0.2 1.3 4.0 0.1 10 12345

set -euo pipefail

if [ "$#" -ne 10 ]; then
    echo "Usage: $0 <instance> <P_size> <max_gen> <p_m> <p_c> <k_ncs> <k_cs> <B_size> <life_span> <seed>" >&2
    exit 2
fi

INSTANCE="$1"; P_SIZE="$2"; MAX_GEN="$3"; P_M="$4"; P_C="$5"
K_NCS="$6"; K_CS="$7"; B_SIZE="$8"; LIFE_SPAN="$9"; SEED="${10}"

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/../../.." && pwd)"
HH_DIR="$REPO_ROOT/hh"

if [ ! -x "$REPO_ROOT/gga-cgt" ]; then
    echo "error: $REPO_ROOT/gga-cgt not found or not executable (build the original first)" >&2
    exit 1
fi
if [ ! -x "$HH_DIR/build/gga-cgt-hh" ]; then
    echo "error: $HH_DIR/build/gga-cgt-hh not found (run: cmake -S \"$HH_DIR\" -B \"$HH_DIR/build\" && cmake --build \"$HH_DIR/build\")" >&2
    exit 1
fi

WORKDIR="$(mktemp -d)"
trap 'rm -rf "$WORKDIR"' EXIT

for VARIANT in orig hh; do
    DIR="$WORKDIR/$VARIANT"
    mkdir -p "$DIR/$(dirname "$INSTANCE")"
    cp "$REPO_ROOT/$INSTANCE" "$DIR/$INSTANCE"
    echo "$INSTANCE" > "$DIR/instances.txt"
    printf 'CONF |P| max_gen p_m p_c k_ncs k_cs B_size life_span seed save_bestSolution\n1 %s %s %s %s %s %s %s %s %s 1' \
        "$P_SIZE" "$MAX_GEN" "$P_M" "$P_C" "$K_NCS" "$K_CS" "$B_SIZE" "$LIFE_SPAN" "$SEED" > "$DIR/configurations.txt"
done
cp "$REPO_ROOT/gga-cgt" "$WORKDIR/orig/"
cp "$HH_DIR/build/gga-cgt-hh" "$WORKDIR/hh/"

( cd "$WORKDIR/orig" && timeout 120 ./gga-cgt < /dev/null > run.log 2>&1 )
( cd "$WORKDIR/hh" && timeout 120 ./gga-cgt-hh < /dev/null > run.log 2>&1 )

ORIG_ROW="$(sed -n '4p' "$WORKDIR/orig/Solutions_GGA-CGT/GGA-CGT_(1).txt")"
HH_ROW="$(sed -n '4p' "$WORKDIR/hh/results/Solutions_GGA-CGT/GGA-CGT_(1).txt")"

# Fields (whitespace-split): 0=instance 1=L2 2=bins 3=gen 4=time 5=delbins 6=delitems
# 7=crosses 8=mutations. Compare all except field 4 (wall-clock time), which is
# expected to differ.
ORIG_FIELDS=($ORIG_ROW)
HH_FIELDS=($HH_ROW)

echo "original: $ORIG_ROW"
echo "hh:       $HH_ROW"

MISMATCH=0
for i in "${!ORIG_FIELDS[@]}"; do
    if [ "$i" -eq 4 ]; then continue; fi # Time column
    if [ "${ORIG_FIELDS[$i]}" != "${HH_FIELDS[$i]:-}" ]; then
        echo "MISMATCH at field $i: original='${ORIG_FIELDS[$i]}' hh='${HH_FIELDS[$i]:-<missing>}'" >&2
        MISMATCH=1
    fi
done

if [ "$MISMATCH" -eq 0 ]; then
    echo "MATCH (all fields except wall-clock time)"
    exit 0
else
    exit 1
fi
