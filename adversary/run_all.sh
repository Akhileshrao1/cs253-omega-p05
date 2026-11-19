#!/usr/bin/env bash
# adversary/run_all.sh - build everything, run the D5 demo, MEASURE Structure B,
# generate small inputs from the measurements, and replay them through
# Structure A, Structure B and the oracle. Ubuntu / WSL:  bash adversary/run_all.sh
set -euo pipefail
cd "$(dirname "$0")"

make -s all
mkdir -p out ../results
N=2048
LOG=../results/seeds.csv

echo "== D5 demo (counterexample + cost formula) =="
./bin/demo_increase_only_fails | tail -3

echo "== measure Structure B at n=$N =="
./bin/find_worst_updates --n $N --out out/worst_positions.txt | sed -n '2,3p;6p'
./bin/find_worst_queries --n $N --out out/worst_pairs.txt | sed -n '3,4p'

echo "== generate inputs (seeds appended to results/seeds.csv) =="
./bin/gen_baseline_rebuild --n $N --ops 4000 --seed 1 --qfrac 10 --out out/baseline.txt --log "$LOG"
./bin/gen_b_updates --n $N --ops 4000 --seed 2 --positions out/worst_positions.txt --qfrac 10 --out out/b_updates.txt --log "$LOG"
./bin/gen_b_queries --n $N --ops 4000 --seed 3 --mode pairs --pairs out/worst_pairs.txt --ufrac 20 --cfrac 30 --out out/b_queries.txt --log "$LOG"

echo "== validate, check the baseline prediction, replay against the oracle =="
./bin/check_input --in out/baseline.txt --baseline 1 | tail -3
for f in baseline b_updates b_queries; do
    ./bin/run_adversarial --in out/$f.txt | head -1
done
echo "ALL DONE"
