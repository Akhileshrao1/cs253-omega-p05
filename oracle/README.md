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
g++ -std=c++17 -O2 -Wall -Wextra -pedantic \\
  oracle/oracle.cpp oracle/differential_test.cpp \\
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

The `PrefixMax` differential overload is implemented. A concrete `PrefixMaxFenwick` implementation is not currently present in the repository, so it is not included in the compiled integration self-test. Add it when that implementation is available.

## Failure shrinking

`shrink_failure(sequence, predicate)` applies delta-debugging to remove chunks of operations while preserving a failure. The predicate must rebuild the candidate from the original initial array before replaying each trial. Preserve the seed and initial array with a minimized sequence for reproducibility.

## Generator and domain

- The default maximum query width is 64, keeping brute-force Oracle queries practical.
- GCD values are generated in `[0, 1e9]`; max-family values avoid reserved `NEG_INF`.
- Prefix queries include endpoint zero.
- Structure B is maximum-only; it is not tested for max frequency or GCD.
- Full invariant checks are periodic because scanning a million-element structure after every operation would dominate the test.
