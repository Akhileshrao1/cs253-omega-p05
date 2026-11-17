# Structure B — Dima–Ceterchi Range Maximum

## Files

- `dima_ceterchi_max.hpp`: implementation of the two-tree Dima–Ceterchi range-maximum structure.
- `test_structB.cpp`: small hand-checked test program, including the requested decrease of the unique maximum.

## Interface

`DimaCeterchiMax` implements the repository's `RangeMax` interface from `include/range_structure.hpp`.

- `build(a)`: builds the structure from a zero-indexed `vector<Value>`.
- `query_max(l, r)`: returns the maximum on the **1-based inclusive** range `[l, r]`.
- `assign(p, v)`: assigns `v` at the **1-based** position `p`; increases and decreases are supported.
- `get(i)`, `size()`, `counters()`, `reset_counters()` and `check_invariants()` implement the shared `Structure` interface.

This implementation stores and returns the range maximum only. It does not maintain the frequency of the maximum.

## Compile

Run the following command from the repository root (the directory containing `include/` and `src/`):

```bash
g++ -std=c++17 -Wall -Wextra -Iinclude src/structB/test_structB.cpp -o test_structB
```

## Run

Linux/macOS:

```bash
./test_structB
```

Windows PowerShell:

```powershell
.\test_structB.exe
```

## Hand-checked decrease test

Initial array:

```text
[6, 12, 12, 18]
```

The initial query `query_max(1, 4)` must return `18`.

Then perform `assign(4, 12)`. The array becomes:

```text
[6, 12, 12, 12]
```

The query `query_max(1, 4)` must now return `12`.

The test also checks a subrange, a single-element range, an increase, another decrease, and the structural invariants F1, F2 and F3.

## Expected final output

```text
PASS: initial maximum of [1, 4]
PASS: maximum after decreasing 18 to 12
PASS: maximum of subrange [2, 4]
PASS: single-element query [1, 1]
PASS: maximum after increasing a non-maximum element
PASS: maximum after decreasing the current maximum
PASS: structural invariants F1, F2 and F3
All Structure B tests passed.
```
