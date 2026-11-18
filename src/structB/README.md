# Structure B — Dima–Ceterchi Range Maximum

This folder contains the Structure B implementation and its small hand-checked test.

## Files
- `dima_ceterchi_max.hpp`: two-tree Dima–Ceterchi implementation.
- `range_structure.hpp`: shared project interface required by the implementation.
- `test_structB.cpp`: basic tests for range maximum, updates, and invariants.

## Compile (PowerShell, from the extracted `structureB` folder)

```powershell
g++ -std=c++17 -Wall -Wextra -O2 -Isrc/structB src/structB/test_structB.cpp -o test_structB.exe
```

## Run

```powershell
.\test_structB.exe
```

Structure B implements `RangeMax` and returns only the maximum value. Maximum-frequency counting is not part of its project interface.

Note: these are basic hand-checked tests, not a full randomized correctness or complexity evaluation.
