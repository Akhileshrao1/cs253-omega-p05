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

## Quick Start

From the repository root, compile and run the sensor replay demo in Ubuntu or WSL.

```bash
g++ -std=c++17 -Iinclude -I. app/sensor_demo.cpp -o /tmp/sensor-demo && /tmp/sensor-demo
```

Enter the number of sensors, the initial readings, the initial calibration values,
and the number of commands. Then enter one command per line. Positions start at
1, and query ranges include both endpoints.

| Sensor demo command | Meaning |
| --- | --- |
| `R i x` | Set the reading at position `i` to `x`. |
| `C i x` | Set the calibration value at position `i` to `x`. |
| `Q l r` | Print the maximum reading, its count, and the calibration GCD over positions `l` through `r`. |

### Sample input

```text
3
4 9 9
6 15 21
5
Q 1 3
R 2 2
Q 1 3
C 1 10
Q 1 3
```

### Expected output

```text
9 2 3
9 1 3
9 1 1
```

Each output row contains `maximum count gcd`. Update commands produce no output.

## Oracle differential test

The Oracle is a plain-array reference implementation. The seeded runner compares each
available structure after point assignments (including decreases), point reads and
range queries. It periodically checks invariants and shrinks a failing operation prefix
with delta debugging.

Build and run the currently integrated differential tests:

```bash
g++ -std=c++17 -O2 -Wall -Wextra -pedantic \
  oracle/oracle.cpp oracle/differential_test.cpp \
  oracle/oracle_differential_selftest.cpp -o oracle_differential_selftest
./oracle_differential_selftest 1000000 1000000 20261118
```

Arguments are `n`, operation count and seed. For a quick local check, use
`./oracle_differential_selftest 5000 20000 20261118` first. The current integration
test covers Structure A MaxCount, Structure A GCD and Structure B maximum.
A PrefixMax differential overload is available, but a concrete prefix-max baseline
is not yet present in the repository, so it is not included in the integration run.
Record the actual compiler output and test output before treating the million-operation
run as verified.

## Oracle-owner deliverables

- Oracle implementation: `oracle/oracle.hpp` and `oracle/oracle.cpp`.
- Differential runner and shrinker: `oracle/differential_test.hpp` and
  `oracle/differential_test.cpp`.
- Integrated self-test: `oracle/oracle_differential_selftest.cpp`.
- D4 derivation: `docs/derivations/D4_250041020.md` (PDF export:
  `D4_250041020.pdf`).
- Known limitation: the PrefixMax baseline implementation is not yet available,
  so only its Oracle and generic differential-test overload are provided.
