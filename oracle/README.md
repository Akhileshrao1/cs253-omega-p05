# Oracle Owner deliverable

This folder contains the plain-array reference implementations and the seeded differential-testing harness for Project 5.

## Files

- `oracle.hpp`, `oracle.cpp`: reference implementations for MaxCount, GCD and prefix maximum.
- `differential_test.hpp`, `differential_test.cpp`: seeded operation generation, differential runners, periodic invariant checks and a delta-debugging shrinker.
- `oracle_smoke_test.cpp`: deterministic Oracle edge-case tests.
- `oracle_large_selftest.cpp`: configurable seeded workload for the Oracle itself.
- `oracle_differential_selftest.cpp`: differential tests for the currently available Structure A MaxCount, Structure A GCD and Structure B maximum implementations.

## Build and run

Run from the repository root with a C++17 compiler.

### 1. Oracle smoke test

```bash
g++ -std=c++17 -O2 -Wall -Wextra -pedantic oracle/oracle.cpp oracle/oracle_smoke_test.cpp -o oracle_smoke_test
./oracle_smoke_test
```

Expected final line: `Oracle smoke tests passed.`

### 2. Large seeded Oracle workload

```bash
g++ -std=c++17 -O2 -Wall -Wextra -pedantic oracle/oracle.cpp oracle/differential_test.cpp oracle/oracle_large_selftest.cpp -o oracle_large_selftest
./oracle_large_selftest 1000000 1000000 20261118
```

Arguments are `n`, operation count and seed, in that order. This runs one million generated operations against the Oracle and checks its invariants at the end. It does not compare Structure A/B.

### 3. Differential test against Structure A and B

```bash
g++ -std=c++17 -O2 -Wall -Wextra -pedantic \
  oracle/oracle.cpp oracle/differential_test.cpp \
  oracle/oracle_differential_selftest.cpp -o oracle_differential_selftest
./oracle_differential_selftest 1000000 1000000 20261118
```

The three test streams use seeds `20261118`, `20261119` and `20261120` respectively. The runner checks:

- Structure A `SegTree<MaxCountMonoid>` against `OracleMaxCount`.
- Structure A `SegTree<GcdMonoid>` against `OracleGcd`.
- Structure B `DimaCeterchiMax` against the maximum projection of `OracleMaxCount`.

Each stream has one million operations by default in the command above. Assignments include increases and decreases; queries include short ranges and valid empty ranges. Point reads and query results are compared after each operation. Full invariant checks are periodic (at least every 1000 operations, and scaled for very large arrays) and always run after the final operation.

Expected successful output:

```text
PASS: Structure A MaxCount
PASS: Structure A GCD
PASS: Structure B maximum
All available differential tests passed.
```

This is the expected success format, not a recorded run result. Record the actual terminal output and machine/compiler details after running it.

## Prefix maximum

The concrete `PrefixMaxFenwick` baseline is present at `src/structB/prefix_max_fenwick.hpp`. It implements prefix-maximum queries and point assignments, including decreases by recomputing affected Fenwick blocks. It is exercised by the benchmark/adversary workflow, but is not currently integrated as a candidate in `oracle_differential_selftest.cpp`. That self-test currently covers Structure A MaxCount, Structure A GCD and Structure B Dima–Ceterchi maximum.

## Failure handling and shrinking

Assignments are checked immediately by reading the assigned position back from both the candidate and Oracle. Build exceptions and size mismatches are reported before replay; their reproducer is an empty operation sequence because no operation caused the failure.

For operation failures, the self-test reducer rebuilds from the same initial array and accepts a reduced sequence only when it reproduces the original specific failure reason (including the invariant identifier where applicable). This prevents a query mismatch from being reported as a smaller, unrelated invariant failure. Preserve the seed and initial array with a minimized sequence for reproducibility.

## Generator and domain

- The default maximum query width is 64, keeping brute-force Oracle queries practical.
- GCD values are generated in `[0, 1e9]`; max-family values avoid reserved `NEG_INF`.
- Prefix queries include endpoint zero.
- Structure B is maximum-only; it is not tested for max frequency or GCD.
- Full invariant checks are periodic because scanning a million-element structure after every operation would dominate the test.
