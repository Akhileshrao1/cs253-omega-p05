# Benchmark harness

From the repository root, use Python 3.9+, a C++17 GCC-compatible compiler, and Matplotlib 3.6+. The C++ harness has no third-party dependencies. GCC with MinGW-w64 works on Windows. On Linux or WSL, use GCC and `python3`.

```sh
python -m pip install matplotlib
python bench/run.py --all --profile quick
python bench/run.py --all
python results/plot.py results
```

- The quick run builds the harness and runs the complete matrix at pilot scale. `--all` also builds and runs the existing smoke tests and checks the sensor demo's sample output.
- The full run defaults to seed `20261119`, one discarded warm-up and seven measured trials per configuration. Use `--seed`, `--runs` (at least five), `--cxx`, or `--out` to repeat with different conditions.
- New runs save to `build/benchmark/full` or `build/benchmark/quick` by default. `--out results` republishes the report's CSVs and figures from the current machine. `REPORT.md` describes the archived run and should be reviewed after replacing its inputs.
- Use `python bench/run.py --case E2-mix-50` for one experiment. It produces raw CSVs and environment metadata in its own output folder. `bash scripts/run_all.sh` is the Linux/WSL entry point for the app, tests and entire matrix.
- To rebuild the report figures without collecting new timings, use the fourth command above. It also rebuilds `results/summary.csv`.

## How the measurements work

1. `workloads.hpp` generates one initial array and operation stream per case. It uses the D5 SplitMix64 generator with independent streams for values, positions and the mix. Seeds and actual operation counts are written into the CSVs. The synthetic readings include negative values, ties, and assignments in both directions. Calibration inputs use a separate nonnegative domain.
2. The harness verifies every candidate query result and checks every assignment with `get`. It checks structural invariants after build and replay, after every operation on edge cases, and every 1,000 operations in the full differential suite. Static wide maximum traces use independently calculated sliding-window maxima for the full trace and a literal oracle for the matched prefix. The oracle executes every query on other traces.
3. Each trial constructs a fresh instance and times its build and operation batch separately. Trace generation, validation and diagnostics are outside the replay timer. Calls use concrete types and keep counters enabled. Variant order rotates each trial. The replay checksum is checked outside the timer.
4. Literal array comparisons use a prefix of the **same** trace bounded by 100 million total queried elements per trial (two million for the pilot). When a cap applies, the indexed structures run both the original full workload and the identical shorter prefix. Compare the `matched` rows across all variants for those cases.

## Results and debugging

| File | Contents |
| --- | --- |
| `raw/E1.csv` through `raw/E10.csv` | Per-trial times in nanoseconds, operation mix, checksum and work counters |
| `raw/checks.csv` | Edge, differential and per-workload verification outcomes and invariant cadence |
| `raw/targets.csv` | High-step query endpoints and the hot assignment positions selected for E5 |
| `raw/update_work.csv` | Histograms of assignment-only counter deltas by workload, variant and direction |
| `raw/index_probe.csv` | Paired decrease/increase counter samples across the array |
| `environment.json` | Machine, flags, seed, caps and hashes of the sources for the run |
| `summary.csv` | Arithmetic mean, sample standard deviation, range and stable work counters by case |

If correctness fails, the runner preserves the original failing prefix and a reduced trace inside the printed staging directory. The reducer preserves the failure message, holds the initial array fixed, and tries up to 64 removals. Reproduce with `build/benchmark/benchmark --replay PATH.trace` (`benchmark.exe` on Windows). The replay file begins with the query kind, fixed width (0 for variable), invariant interval and input size, followed by initial values, the operation count and lines of `kind l r value`. `U` assigns, `Q` queries and `R` reads. Positions are 1-based.
