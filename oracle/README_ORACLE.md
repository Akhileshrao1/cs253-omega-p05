# Oracle Owner deliverable

This folder is designed for the Project 5 C++17 interface in `include/range_structure.hpp`.

## Files

- `oracle/oracle.hpp`, `oracle/oracle.cpp`: three plain-array Oracle classes.
- `oracle/differential_test.hpp`, `oracle/differential_test.cpp`: seeded operation generation, differential runners for the four relevant interfaces, and delta-debugging shrinker.
- `tests/oracle_smoke_test.cpp`: deterministic edge-case tests.
- `tests/oracle_large_selftest.cpp`: configurable seeded workload for the Oracle itself.
- `docs/derivations/D4.md`: proof that prefix maxima have no inverse.

## Expected repository layout

Place these files in your repository while retaining the shared header:

```text
project/
  include/range_structure.hpp
  oracle/
    oracle.hpp
    oracle.cpp
    differential_test.hpp
    differential_test.cpp
  tests/
    oracle_smoke_test.cpp
    oracle_large_selftest.cpp
  docs/derivations/D4.md
```

The `#include "../include/range_structure.hpp"` line assumes this layout.

## Build

From the repository root:

```bash
g++ -std=c++17 -O2 -Wall -Wextra -pedantic \
    oracle/oracle.cpp tests/oracle_smoke_test.cpp \
    -o oracle_smoke_test
./oracle_smoke_test
```

Build the workload test:

```bash
g++ -std=c++17 -O2 -Wall -Wextra -pedantic \
    oracle/oracle.cpp oracle/differential_test.cpp \
    tests/oracle_large_selftest.cpp -o oracle_large_selftest
```

Run the requested scale:

```bash
./oracle_large_selftest 1000000 1000000 20261118
```

This large self-test runs one million generated operations against the Oracle. It is
not a substitute for differential testing against Structure A/B; those tests need
the teammates' concrete classes.

## Connecting Structure A and Structure B

Once their headers and concrete classes are available, instantiate them and call
the matching overload:

```cpp
// Max-count segment tree:
SegTree<MaxCountMonoid> seg_max_count;
TestFailure f1 = differential_test(
    seg_max_count, initial_readings, operations, 1000);

// GCD segment tree:
SegTree<GcdMonoid> seg_gcd;
TestFailure f2 = differential_test(
    seg_gcd, initial_calibration_factors, gcd_operations, 1000);

// Structure B (max only):
DimaCeterchiMax two_tree;
TestFailure f3 = differential_test(
    two_tree, initial_readings, max_operations, 1000);

// Prefix-max Fenwick baseline:
PrefixMaxFenwick baseline;
TestFailure f4 = differential_test(
    baseline, initial_readings, prefix_operations, 1000);
```

The exact constructors and include paths depend on teammates' files. The harness
takes references to the abstract interface, so their concrete classes need only
inherit the appropriate interface.

For a failed test, shrink the sequence by replaying it from a fresh build:

```cpp
auto failure = differential_test(candidate, initial, operations, 1000);
if (failure.failed) {
    auto still_fails = [&](const vector<TestOperation>& trial) {
        return differential_test(candidate, initial, trial, 1).failed;
    };
    auto smaller = shrink_failure(failure.sequence, still_fails);
    // Print `smaller` and retain the seed + initial array for reproduction.
}
```

For a truly minimal reproducer, preserve the same failure signature (e.g. same
mismatch type and location), rather than merely accepting any failure. The simple
predicate above preserves the existence of a failure.

## Test scope and limitations

- The generator uses short random query ranges by default (`max_query_width=64`).
  This keeps a brute-force oracle practical for one million operations.
- For broader range coverage, run additional smaller tests with a larger
  `max_query_width`, including full-range queries.
- The generic differential functions compare only operations supported by the
  candidate interface. Structure B is max-only: it must not be tested for max
  frequency or GCD.
- The shrinker minimizes the operation list, not the initial array. Keep the
  seed and initial array in the bug report. If required, add a second-stage
  value/array minimizer.
- The Oracle's O1 property (no auxiliary index) is guaranteed by its design.
  Runtime `check_invariants` validates the stored array's value domain; G1-G4
  are also exercised by deterministic and differential tests. A black-box
  checker cannot independently prove that a query made no hidden mutation
  without taking an external snapshot of all observable values.
- Counter definitions should be kept consistent with the team's agreed model.
  This implementation counts array reads/writes, scan combines, and Euclid
  modulo-loop iterations. Build resets counters as required by the interface.
