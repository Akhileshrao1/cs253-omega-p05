# INTERFACE.md — Project 5: Aggregates a Fenwick tree cannot support

Common interface and invariants for Structure A, Structure B, the baseline and the oracle.

## 1. Conventions

| Item | Decision |
|------|----------|
| Language | C++17 (Structure A, Structure B and the oracle are all in C++, as required) |
| Namespace | `using namespace std;` in the shared header. No free function named `gcd`, `max`, `min`, `size` or `count` may be defined in project code, because it would clash with `std`. Name helpers `euclid_gcd` and so on. |
| Indexing | **1-based.** Position `i` is valid for `1 <= i <= n`. Fenwick cells use the position itself, so `i & -i` works directly with no shifting. |
| Ranges | **Closed `[l, r]`**, valid for `1 <= l` and `r <= n` and `l <= r + 1`. `l == r + 1` is the empty range. |
| Build input | `build(a)` takes a `vector<Value>` of `n` values, where `a[0]` is the value for position 1. Only this one function sees 0-based data. |
| Value type | `Value = int64_t` |
| Max domain | Any `Value` except `INT64_MIN`, which is reserved as the identity `-inf`. |
| Gcd domain | `0 <= v <= V` with `V = 10^9` (revise when the evaluation format is published). `gcd(0, x) = x`, so `0` is the identity. |
| Two arrays | Readings (for max and count) and calibration factors (for gcd) are separate arrays. A gcd structure never sees readings and vice versa. |
| Errors | Out-of-range index or range, or a value outside its domain: throw `out_of_range` or `invalid_argument`. Never undefined behaviour. |
| Size | `n` is fixed at `build`. There is no insert or erase. |
| Containers | No library container stands in for a structure under study. `vector` is allowed as raw storage. `map`, `priority_queue` and similar are allowed only in `tests/` and data generation. |
| Dispatch | The abstract classes below exist for the differential test. Concrete classes are `final`, and benchmarks call them through the concrete type, so virtual dispatch does not distort timings. |

## 2. Operations and semantics

| Operation | Meaning | Empty range (`l == r + 1`) |
|-----------|---------|----------------------------|
| `build(a)` | Replace the contents, set `n = a.size()`, position `i` gets `a[i-1]`. Resets counters. | n/a (`n = 0` allowed) |
| `size()` | Returns `n`. | n/a |
| `get(i)` | Current value at position `i` (last assigned, else initial). | n/a |
| `assign(i, v)` | Set position `i` to `v`. `v` may be smaller than, equal to, or larger than the old value. | n/a |
| `query_max(l, r)` | maximum of positions `l..r` | returns `NEG_INF` |
| `query_max_count(l, r)` | `(m, c)`: the maximum `m` and the number `c` of positions in `l..r` holding `m` | returns `(NEG_INF, 0)` |
| `query_gcd(l, r)` | gcd of positions `l..r` | returns `0` |
| `query_prefix_max(r)` | maximum of positions `1..r`, same as `query_max(1, r)`. Valid for `0 <= r <= n`. | `r = 0` returns `NEG_INF` |

Queries never change the observable state. A query after `assign(i, v)` sees `v` at position `i`.

## 3. Interface (`include/range_structure.hpp`)

```cpp
#pragma once
#include <climits>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>

using namespace std;

using Value = int64_t;
using Index = size_t;                    // a 1-based position or a Fenwick cell number

inline constexpr Value NEG_INF = INT64_MIN;     // identity of max

struct MaxCount {                               // monoid element for (max, count)
    Value    max;                               // NEG_INF for the identity
    uint64_t count;                             // 0 for the identity
};

// Operation counters, maintained by every structure (used by the Analyst).
struct OpCounters {
    uint64_t node_reads     = 0;   // array or tree cells read
    uint64_t node_writes    = 0;   // array or tree cells written
    uint64_t combines       = 0;   // monoid operations applied (max, merge, gcd)
    uint64_t recomputations = 0;   // cells rebuilt from other cells (decrease handling)
    uint64_t gcd_steps      = 0;   // iterations of Euclid's algorithm
    void reset() { *this = OpCounters{}; }
};

// Members shared by every structure.
class Structure {
public:
    virtual ~Structure() = default;
    virtual Index size() const = 0;
    virtual Value get(Index i) const = 0;               // 1 <= i <= n
    virtual void  assign(Index i, Value v) = 0;         // 1 <= i <= n
    virtual const OpCounters& counters() const = 0;
    virtual void reset_counters() = 0;
    // Checks every structural invariant of INTERFACE.md section 4 that applies.
    // Returns true if all hold; otherwise false and a message naming the
    // invariant ID and the position in *why (if why != nullptr).
    virtual bool check_invariants(string* why = nullptr) const = 0;
};

class RangeMax : public Structure {              // Structure A (max), Structure B, oracle
public:
    virtual void  build(const vector<Value>& a) = 0;   // a[0] is position 1
    virtual Value query_max(Index l, Index r) const = 0;       // closed [l, r]
};

class RangeMaxCount : public RangeMax {          // Structure A (max, count), oracle
public:
    virtual MaxCount query_max_count(Index l, Index r) const = 0;
};

class RangeGcd : public Structure {              // Structure A (gcd), oracle
public:
    virtual void  build(const vector<Value>& a) = 0;   // a[0] is position 1
    virtual Value query_gcd(Index l, Index r) const = 0;
};

class PrefixMax : public Structure {             // prefix-max Fenwick baseline, oracle
public:
    virtual void  build(const vector<Value>& a) = 0;   // a[0] is position 1
    virtual Value query_prefix_max(Index r) const = 0;        // 0 <= r <= n
};
```

### Who implements what

| Class | Implementations | Owner | Folder |
|-------|-----------------|-------|--------|
| `RangeMaxCount` | `SegTree<MaxCountMonoid>` (Structure A) | Structure A owner | `src/structA/` |
| `RangeGcd` | `SegTree<GcdMonoid>` (Structure A) | Structure A owner | `src/structA/` |
| `RangeMax` | `DimaCeterchiMax` (Structure B) | Structure B owner | `src/structB/` |
| `PrefixMax` | `PrefixMaxFenwick` (baseline) | Structure B owner | `src/structB/` |
| all four | `OracleMaxCount`, `OracleGcd`, `OraclePrefixMax`: a plain array | Oracle owner | `oracle/` |

Structure B answers max only. Counting the maximum is not part of its contract, and the report states this as a finding.
The baseline's `assign` must be correct for decreases (it rebuilds affected cells and increments `recomputations`), because the differential test applies the same operation stream to every structure.

Monoid requirements for `SegTree<M>`: a type `M::T`, a constant `M::identity()`, and an associative `M::combine(T, T)`. Commutativity is not assumed. Leaves are combined in position order, so the tree is correct for non-commutative monoids too.

## 4. Structural invariants

Every invariant has an ID. `check_invariants` reports the ID of the first violation, and the bug log (report section 7) uses the same IDs. The checker runs after every operation in small tests and every 1000 operations in the 10^5 and 10^6 operation tests.

### 4.1 All structures

| ID | Invariant |
|----|-----------|
| G1 | `size()` is constant between `build` calls. |
| G2 | `get(i)` equals the last value assigned to position `i` (or the built value). |
| G3 | Queries and `get` leave the structure unchanged. |
| G4 | Empty-range queries return the identity of section 2. |

### 4.2 Structure A: segment tree

Let `cap` be the smallest power of two with `cap >= n`. The leaf for position `i` is `t[cap + i - 1]`. Node `k` has children `2k` and `2k + 1`.

| ID | Invariant |
|----|-----------|
| S1 | For `1 <= i <= n`: `t[cap + i - 1] = lift(a[i])`, where `lift(v) = (v, 1)` for max-count and `lift(v) = v` for gcd. |
| S2 | For `n < i <= cap`: `t[cap + i - 1] = identity`. |
| S3 | For `1 <= k < cap`: `t[k] = combine(t[2k], t[2k + 1])`. |
| S4 (max-count) | For every node with `max != NEG_INF`: `1 <= count <=` number of real leaves under the node. |
| S5 (gcd) | Every `t[k] >= 0`, `t[k]` divides every real leaf under it, and `t[k] = 0` only if all those leaves are 0. |

### 4.3 Structure B: Dima–Ceterchi two-tree Fenwick

`a[1..n]` is the stored array, `T1` the standard tree and `T2` the second (reverse) tree. `lowbit(i) = i & -i`.

| ID | Invariant |
|----|-----------|
| F1 | `T1[i] = max(a[i - lowbit(i) + 1 .. i])` for `1 <= i <= n`. |
| F2 | `T2[i] = max(a[i .. min(n, i + lowbit(i) - 1)])` for `1 <= i <= n`. |
| F3 | `a[i]` equals the last value assigned to position `i`. |

The exact tree definitions are to be confirmed against the paper in D2. If they differ, the Structure B owner and the Architect update F1 and F2 in a pull request before `c2`.

### 4.4 Baseline: prefix-maximum Fenwick

| ID | Invariant |
|----|-----------|
| P1 | `bit[i] = max(a[i - lowbit(i) + 1 .. i])` for `1 <= i <= n`, after every operation including decreases. |

### 4.5 Oracle

| ID | Invariant |
|----|-----------|
| O1 | The only state is the array `a` and the counters. There is no auxiliary index that could share a bug with a structure under test. |

## 5. Complexity targets

These are design targets. The measured and predicted costs are in `REPORT.md`.

| Operation | Structure A (segment tree) | Structure B (two-tree Fenwick) | Baseline | Oracle |
|-----------|---------------------------|--------------------------------|----------|--------|
| `build` | O(n) | O(n) | O(n) | O(n) |
| `query_max` / `query_max_count` | O(log n) | O(log n) | prefix only | O(r − l) |
| `query_gcd` | O(log n + log V) | n/a | n/a | O(r − l + log V) |
| `assign` (increase) | O(log n) | O(log n) expected | O(log n) | O(1) |
| `assign` (decrease) | O(log n) | to be measured (adversarial target) | repeated rebuilds (adversarial target) | O(1) |

## 6. Differential-test operation model

The differential test (`oracle/`) draws operations from a seeded generator, applies each to the oracle and to the structure under test, compares every query result, and runs `check_invariants` at the intervals in section 4. Positions are 1-based and ranges closed.

| Op | Form | Compared |
|----|------|----------|
| assign | `A i v` | none (checked later through queries and `get`) |
| max query | `M l r` | `Value` |
| max-count query | `C l r` | `(max, count)` |
| gcd query | `G l r` | `Value` |
| prefix query | `P r` | `Value` |
| read | `R i` | `Value` |

The seed and the operation mix are printed at the start of every run. A failing sequence is shrunk to a minimal sequence before it is reported.

The format of the hidden-test evaluation interface will be published by the instructor. `app/` will implement it and README.md will describe it. It is separate from the internal model above.

## 7. Counters

Every structure increments `OpCounters` on each cell it reads or writes, each combine it applies, each cell it rebuilds from other cells, and each Euclid iteration. Counters are plain integers with no atomics and no I/O. The predicted-cost model in `REPORT.md` is stated in these units, which makes the prediction directly comparable with measurement.
