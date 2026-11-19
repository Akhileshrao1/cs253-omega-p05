# adversary/ — Adversary module (D5)

Owner: Koushik Reddy K (`adversary/`, derivation D5). Builds worst-case inputs for Structure B and the
prefix-max baseline, measures them with the structures' own counters, and checks every answer against the oracle.

All inputs are in **exactly the format `app/sensor_demo.cpp` reads** (see README.md quick start), so
`/tmp/sensor-demo < adversary/out/file.txt` runs any generated file.

## Files
| File | Purpose |
|---|---|
| `common.hpp` | option parsing, seeded RNG (identical on every compiler), the file writer, Fenwick formulas |
| `reader.hpp` | reads and validates an input file |
| `baseline_sim.hpp` | model of the prefix-max Fenwick baseline (the team's own baseline is not written yet) |
| `find_worst_updates.cpp` | measures B's update cost at every position; reports whether decreases cost more |
| `find_worst_queries.cpp` | finds the (l, r) that cost B the most query steps (exhaustive, or hill-climb for large n) |
| `target.hpp` | which structure the find_worst_* tools probe (default: `DimaCeterchiMax`) |
| `gen_baseline_rebuild.cpp` | D5(b): decreasing updates that force the baseline into repeated rebuilds |
| `gen_b_updates.cpp` | decreasing updates on B's most expensive positions (measured), also hostile to the baseline |
| `gen_b_queries.cpp` | worst-case queries (`pairs`), range-width sweeps (`width`), controls (`random`) |
| `run_adversarial.cpp` | replays a file through A, B and the oracle, compares every answer, prints counters |
| `check_input.cpp` | validates a file; `--baseline 1` replays it through the baseline model |
| `demo_increase_only_fails.cpp` | executable evidence for D5(a) and D5(b) |

## Build (repository root)
Ubuntu / WSL:
```
make -C adversary                 # everything
bash adversary/run_all.sh         # build + demo + measure + generate + replay
```
PowerShell / MinGW (no make needed), from the repository root:
```
mkdir adversary\bin
g++ -std=c++17 -O2 adversary/gen_baseline_rebuild.cpp -o adversary/bin/gen_baseline_rebuild.exe
g++ -std=c++17 -O2 adversary/gen_b_updates.cpp -o adversary/bin/gen_b_updates.exe
g++ -std=c++17 -O2 adversary/gen_b_queries.cpp -o adversary/bin/gen_b_queries.exe
g++ -std=c++17 -O2 adversary/check_input.cpp -o adversary/bin/check_input.exe
g++ -std=c++17 -O2 adversary/demo_increase_only_fails.cpp -o adversary/bin/demo_increase_only_fails.exe
g++ -std=c++17 -O2 adversary/find_worst_updates.cpp -o adversary/bin/find_worst_updates.exe
g++ -std=c++17 -O2 adversary/find_worst_queries.cpp -o adversary/bin/find_worst_queries.exe
g++ -std=c++17 -O2 adversary/run_adversarial.cpp oracle/oracle.cpp -o adversary/bin/run_adversarial.exe
```
**Always use `--out file`.** In PowerShell, `> file` writes UTF-16 and the application cannot read it.

## Workflow
```
# 1. MEASURE Structure B first (the generators use what is measured, not a guess)
bin/find_worst_updates --n 1000000 --out out/worst_positions.txt
bin/find_worst_queries --n 1000000 --mode hill --out out/worst_pairs.txt   # exact search: --n 1024 (<= 8192)

# 2. generate (every seed can be appended to ../results/seeds.csv with --log)
bin/gen_baseline_rebuild --n 1000000 --ops 1000000 --seed 1 --out out/baseline.txt --log ../results/seeds.csv
bin/gen_b_updates  --n 1000000 --ops 1000000 --seed 1 --positions out/worst_positions.txt --out out/b_updates.txt --log ../results/seeds.csv
bin/gen_b_queries  --n 1000000 --ops 1000000 --seed 1 --mode pairs --pairs out/worst_pairs.txt --out out/b_queries.txt --log ../results/seeds.csv

# 3. sweeps for the Analyst (experiments E1/E2)
bin/gen_b_queries --n 1000000 --ops 1000000 --seed 1 --mode width --width 64 --ufrac 10 --out out/sweep.txt

# 4. check correctness and read the counters (the oracle scans each range: use moderate n or few wide queries)
bin/run_adversarial --in out/b_updates.txt
bin/check_input --in out/baseline.txt --baseline 1
```
Each generated file has a `<file>.meta` sidecar with its parameters and predictions. Large files in `out/`
are NOT committed; regenerate them from the recorded seed.

## What was measured (counters, so machine-independent)
| n = 1,000,000 | result |
|---|---|
| B update (reads) | 191 at the worst position, mean 189.7; **identical for decreases and increases**; 92% of positions within 1% of the worst |
| A update (reads) | 40 |
| B query, worst (reads) | 38 = 2·log2 n, e.g. (l=1, r=917503); random ranges average 18.9 |
| A query (reads) | mean 17.9 |
| Baseline model, n decreasing sweep | 10,095,616 cells rebuilt; 548,010,882,784 elements rescanned if each rebuild rescans its block (exactly Σ lowbit(j)²) |
Exhaustive at n = 1024 and 2048: B's single worst query is (1, n−1) with 2·log2 n − 1 reads; mean = log2 n − 1.

## Assumptions still open
1. The baseline is modelled (`baseline_sim.hpp`); once `PrefixMaxFenwick` exists, point `target.hpp`-style probing at it.
2. "Rescanning a block" is one possible rebuild. The count of REBUILT CELLS does not depend on that choice; the element-read count does.
3. Measured counts use `OpCounters` as defined in INTERFACE.md section 7.
