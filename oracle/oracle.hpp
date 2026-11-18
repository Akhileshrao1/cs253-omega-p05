#pragma once

#include "../include/range_structure.hpp"

// Oracle implementations intentionally store only the raw array and counters.
// API positions are 1-based; build input is 0-based.

class OracleMaxCount final : public RangeMaxCount {
public:
    void build(const vector<Value>& input) override;
    Index size() const override;
    Value get(Index i) const override;
    void assign(Index i, Value v) override;
    Value query_max(Index l, Index r) const override;
    MaxCount query_max_count(Index l, Index r) const override;
    const OpCounters& counters() const override;
    void reset_counters() override;
    bool check_invariants(string* why = nullptr) const override;

private:
    vector<Value> a_;             // a_[0] unused; position i is a_[i]
    mutable OpCounters ops_;

    static void validate_value(Value v);
    void validate_index(Index i) const;
    void validate_range(Index l, Index r) const;
};

class OracleGcd final : public RangeGcd {
public:
    void build(const vector<Value>& input) override;
    Index size() const override;
    Value get(Index i) const override;
    void assign(Index i, Value v) override;
    Value query_gcd(Index l, Index r) const override;
    const OpCounters& counters() const override;
    void reset_counters() override;
    bool check_invariants(string* why = nullptr) const override;

private:
    vector<Value> a_;
    mutable OpCounters ops_;

    static void validate_value(Value v);
    void validate_index(Index i) const;
    void validate_range(Index l, Index r) const;
    Value euclid_gcd(Value x, Value y) const;
};

class OraclePrefixMax final : public PrefixMax {
public:
    void build(const vector<Value>& input) override;
    Index size() const override;
    Value get(Index i) const override;
    void assign(Index i, Value v) override;
    Value query_prefix_max(Index r) const override;
    const OpCounters& counters() const override;
    void reset_counters() override;
    bool check_invariants(string* why = nullptr) const override;

private:
    vector<Value> a_;
    mutable OpCounters ops_;

    static void validate_value(Value v);
    void validate_index(Index i) const;
};
