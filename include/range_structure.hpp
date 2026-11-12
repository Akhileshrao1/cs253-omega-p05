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
