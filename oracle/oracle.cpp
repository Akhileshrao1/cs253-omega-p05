#include "oracle.hpp"

#include <algorithm>
#include <limits>
#include <sstream>

namespace {
bool valid_closed_range(Index n, Index l, Index r) {
    // Valid non-empty range: 1 <= l <= r <= n.
    // Valid empty range:     1 <= l <= n+1 and l == r+1, with r <= n.
    if (l == 0 || r > n) return false;
    if (l <= r) return true;
    return (l - r == 1); // avoids evaluating r + 1 (which could overflow)
}

bool fail_with(string* why, const string& message) {
    if (why) *why = message;
    return false;
}
}

void OracleMaxCount::validate_value(Value v) {
    if (v == NEG_INF)
        throw invalid_argument("INT64_MIN is reserved as NEG_INF");
}

void OracleMaxCount::validate_index(Index i) const {
    if (i == 0 || i >= a_.size())
        throw out_of_range("OracleMaxCount index must be in [1,n]");
}

void OracleMaxCount::validate_range(Index l, Index r) const {
    if (!valid_closed_range(size(), l, r))
        throw out_of_range("OracleMaxCount range must be a valid closed or empty range");
}

void OracleMaxCount::build(const vector<Value>& input) {
    for (Value v : input) validate_value(v);
    a_.assign(input.size() + 1, 0);
    for (Index i = 0; i < input.size(); ++i) a_[i + 1] = input[i];
    ops_.reset();
}

Index OracleMaxCount::size() const { return a_.empty() ? 0 : a_.size() - 1; }

Value OracleMaxCount::get(Index i) const {
    validate_index(i);
    ++ops_.node_reads;
    return a_[i];
}

void OracleMaxCount::assign(Index i, Value v) {
    validate_index(i);
    validate_value(v);
    ++ops_.node_reads;             // read old cell before replacing it
    ++ops_.node_writes;
    a_[i] = v;
}

MaxCount OracleMaxCount::query_max_count(Index l, Index r) const {
    validate_range(l, r);
    MaxCount answer{NEG_INF, 0};

    for (Index i = l; i <= r; ++i) {
        ++ops_.node_reads;
        const Value v = a_[i];

        if (v > answer.max) {
            answer = {v, 1};
        } else if (v == answer.max) {
            ++answer.count;
        }
        if (i != l) ++ops_.combines;
    }
    return answer;
}

Value OracleMaxCount::query_max(Index l, Index r) const {
    return query_max_count(l, r).max;
}

const OpCounters& OracleMaxCount::counters() const { return ops_; }
void OracleMaxCount::reset_counters() { ops_.reset(); }

bool OracleMaxCount::check_invariants(string* why) const {
    if (a_.empty()) return fail_with(why, "O1: backing array is not initialized");
    for (Index i = 1; i < a_.size(); ++i) {
        if (a_[i] == NEG_INF) {
            ostringstream out; out << "O1: reserved value at position " << i;
            return fail_with(why, out.str());
        }
    }
    // O1 is principally an architectural invariant: this class has exactly
    // one data array plus counters, and no cached/query index.
    if (why) why->clear();
    return true;
}

// ------------------------------- GCD oracle -------------------------------

void OracleGcd::validate_value(Value v) {
    if (v < 0 || v > 1000000000LL)
        throw invalid_argument("GCD values must be in [0, 1e9]");
}

void OracleGcd::validate_index(Index i) const {
    if (i == 0 || i >= a_.size())
        throw out_of_range("OracleGcd index must be in [1,n]");
}

void OracleGcd::validate_range(Index l, Index r) const {
    if (!valid_closed_range(size(), l, r))
        throw out_of_range("OracleGcd range must be a valid closed or empty range");
}

void OracleGcd::build(const vector<Value>& input) {
    for (Value v : input) validate_value(v);
    a_.assign(input.size() + 1, 0);
    for (Index i = 0; i < input.size(); ++i) a_[i + 1] = input[i];
    ops_.reset();
}

Index OracleGcd::size() const { return a_.empty() ? 0 : a_.size() - 1; }

Value OracleGcd::get(Index i) const {
    validate_index(i);
    ++ops_.node_reads;
    return a_[i];
}

void OracleGcd::assign(Index i, Value v) {
    validate_index(i);
    validate_value(v);
    ++ops_.node_reads;
    ++ops_.node_writes;
    a_[i] = v;
}

Value OracleGcd::euclid_gcd(Value x, Value y) const {
    while (y != 0) {
        const Value remainder = x % y;
        x = y;
        y = remainder;
        ++ops_.gcd_steps;
    }
    return x;
}

Value OracleGcd::query_gcd(Index l, Index r) const {
    validate_range(l, r);
    Value answer = 0; // gcd identity

    for (Index i = l; i <= r; ++i) {
        ++ops_.node_reads;
        answer = euclid_gcd(answer, a_[i]);
        ++ops_.combines;
    }
    return answer;
}

const OpCounters& OracleGcd::counters() const { return ops_; }
void OracleGcd::reset_counters() { ops_.reset(); }

bool OracleGcd::check_invariants(string* why) const {
    if (a_.empty()) return fail_with(why, "O1: backing array is not initialized");
    for (Index i = 1; i < a_.size(); ++i) {
        if (a_[i] < 0 || a_[i] > 1000000000LL) {
            ostringstream out; out << "O1: GCD value outside domain at position " << i;
            return fail_with(why, out.str());
        }
    }
    if (why) why->clear();
    return true;
}

// ---------------------------- Prefix maximum oracle -----------------------

void OraclePrefixMax::validate_value(Value v) {
    if (v == NEG_INF)
        throw invalid_argument("INT64_MIN is reserved as NEG_INF");
}

void OraclePrefixMax::validate_index(Index i) const {
    if (i == 0 || i >= a_.size())
        throw out_of_range("OraclePrefixMax index must be in [1,n]");
}

void OraclePrefixMax::build(const vector<Value>& input) {
    for (Value v : input) validate_value(v);
    a_.assign(input.size() + 1, 0);
    for (Index i = 0; i < input.size(); ++i) a_[i + 1] = input[i];
    ops_.reset();
}

Index OraclePrefixMax::size() const { return a_.empty() ? 0 : a_.size() - 1; }

Value OraclePrefixMax::get(Index i) const {
    validate_index(i);
    ++ops_.node_reads;
    return a_[i];
}

void OraclePrefixMax::assign(Index i, Value v) {
    validate_index(i);
    validate_value(v);
    ++ops_.node_reads;
    ++ops_.node_writes;
    a_[i] = v;
}

Value OraclePrefixMax::query_prefix_max(Index r) const {
    if (r > size()) throw out_of_range("prefix endpoint must be in [0,n]");
    Value answer = NEG_INF;
    for (Index i = 1; i <= r; ++i) {
        ++ops_.node_reads;
        answer = std::max(answer, a_[i]);
        ++ops_.combines;
    }
    return answer;
}

const OpCounters& OraclePrefixMax::counters() const { return ops_; }
void OraclePrefixMax::reset_counters() { ops_.reset(); }

bool OraclePrefixMax::check_invariants(string* why) const {
    if (a_.empty()) return fail_with(why, "O1: backing array is not initialized");
    for (Index i = 1; i < a_.size(); ++i) {
        if (a_[i] == NEG_INF) {
            ostringstream out; out << "O1: reserved value at position " << i;
            return fail_with(why, out.str());
        }
    }
    if (why) why->clear();
    return true;
}
