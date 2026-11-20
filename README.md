# Project 05: Aggregates a Fenwick tree cannot support

Team Omega — CS 253 / MA 253, Spring 2026

## Team and roles

| Name | Roll no. | GitHub | Role | Module folder | Derivation |
| --- | --- | --- | --- | --- | --- |
| Hanchate Akhilesh Rao | 250001031 | @Akhileshrao1 | Architect | `app/` | D3 |
| Tharun A Surana | 250001074 | @tharunsurana-cell | Structure A owner | `src/structA/` | D1 |
| G Chetan Kumar | 250001030 | @chetankumar543-collab | Structure B owner | `src/structB/` | D2 |
| Kasireddy Pranav Karthik | 250041020 | @pranavkarthik1817 | Oracle owner | `oracle/` | D4 |
| Koushik Reddy K | 250001041 | @kkreddy2332 | Adversary | `adversary/` | D5 |
| Thakur Tejasva Singh | 250001072 | @thakur-tejasva | Analyst | `bench/` | D6 |
| Aashish Sharma | 250041001 | @aashishsharma9098 | Scale & Reproducibility | `bench/` | D7 |

## Summary

The sensor replay runs the same input with either structure. Structure A uses a generic segment tree with separate max-count and GCD monoids and returns the maximum, its frequency, and the calibration GCD. Structure B uses a two-tree Fenwick implementation and returns the range maximum. The benchmark also measures a prefix-only Fenwick baseline and the plain-array oracle. See [REPORT.md](REPORT.md) for the measured crossover and the cost of decreases.

## Build and test

Use a C++17 compiler compatible with GCC, Python 3.9 or newer, and Matplotlib 3.6 or newer for figures. The archived run used GCC 15.2.0 (MinGW-w64) and Python 3.14.8 on Windows. The benchmark and app are built from the repository root.

```sh
python -m pip install matplotlib
python bench/run.py --build-only
python bench/run.py --all --profile quick
```

`--build-only` compiles the harness and runs the unit smoke tests and sensor sample. The quick run adds the differential checks and a reduced-size benchmark. The full run uses one million operations per differential workload, seed `20261119`, and prints `Verified differential ... (1000000 operations)` for each operation family.

## Quick Start

From the repository root on Ubuntu or WSL, build the evaluator and save the sample below as `trace.txt`.

```bash
mkdir -p build
g++ -std=c++17 -O2 -Iinclude -I. app/sensor_demo.cpp -o build/sensor_demo
./build/sensor_demo --structure A < trace.txt
./build/sensor_demo --structure B < trace.txt
```

### Evaluation interface

Both modes read the positive sensor count, that many initial readings, that many initial calibration values, and a nonnegative command count. Then supply one command per line. Positions start at 1 and query ranges include both endpoints. Readings are signed 64-bit integers with the minimum value reserved. Calibration values range from 0 through 1,000,000,000.

| Sensor demo command | Meaning |
| --- | --- |
| `R i x` | Set the reading at position `i` to `x`. |
| `C i x` | Set the calibration value at position `i` to `x` in A. B validates and consumes the same command. |
| `Q l r` | Query positions `l` through `r`. A prints `maximum count gcd` and B prints `maximum`. |

Each query prints one output row. Update commands produce no output. Running the evaluator without arguments selects Structure A.

### Sample input

```text
3
4 9 9
6 15 21
7
Q 1 3
R 2 2
Q 1 3
R 3 3
Q 1 3
C 1 10
Q 1 3
```

### Expected output for Structure A

```text
9 2 3
9 1 3
4 1 3
4 1 1
```

### Expected output for Structure B

```text
9
9
4
4
```

## Benchmarks and figures

```sh
python bench/run.py --all
python results/plot.py results
```

The first command runs the tests, sensor example, full-scale benchmarks and figures. On Ubuntu/WSL, `bash scripts/run_all.sh` is equivalent. Allow several minutes for the full run. It writes to `build/benchmark/full/` by default. The second command rebuilds the archived figures and summary from existing CSVs.

See [bench/README.md](bench/README.md) for options, input generation, CSV schemas and failure replay. See [the raw measurements](results/raw/) and [the timing summary](results/summary.csv) for the archived run.

## Repository map

| Path | Contents |
| --- | --- |
| `include/`, `src/` | Shared interfaces and the structures under study |
| `app/` | Sensor replay and the command format shown above |
| `oracle/`, `tests/` | Array oracles and unit smoke tests |
| `adversary/`, `bench/` | D5 tools, workload generation, differential tests and timing harness |
| `results/`, `docs/` | Measurement data, figures and derivations |
| `scripts/` | One-command build, test and benchmark on Ubuntu/WSL |

## Results at a glance

| Full-scale range maximum | Segment tree | Two-tree Fenwick |
| --- | ---: | ---: |
| Width 500,000, queries only | 260.63 ± 12.38 ns/query | 96.05 ± 6.49 ns/query |
| Width 64, 50% assignments | 228.05 ± 25.13 ns/op | 815.95 ± 186.05 ns/op |

Values are mean ± sample standard deviation across seven trials. See [the full results](REPORT.md#11-results) <!--and [the PDF report](REPORT.pdf)-->
 To export both the report and D6 after editing, use `python scripts/export_report.py --chromium PATH_TO_CHROME` with Pandoc installed.

## Known limitations

The former standalone oracle large self-test references an absent helper header. The reproducible million-operation differential suite lives in `bench/benchmark.cpp`.

See [TOOLS.md](TOOLS.md) for tool use and [REPORT.md](REPORT.md#13-references) for references.
