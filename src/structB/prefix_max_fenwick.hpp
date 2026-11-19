
#pragma once

#include "../../include/range_structure.hpp"
#include <algorithm>

class PrefixMaxFenwick final : public PrefixMax {
private:
    Index n = 0;
    vector<Value> values;
    vector<Value> bit;
    mutable OpCounters ops;

    static Index lowbit(Index i) {
        return i & (~i + 1);
    }

    void validate_index(Index i) const {
        if (i < 1 || i > n)
            throw out_of_range("Invalid index");
    }

public:
    PrefixMaxFenwick() = default;

    void build(const vector<Value>& a) override {
        n = a.size();

        for (Value v : a) {
            if (v == NEG_INF)
                throw invalid_argument(
                    "NEG_INF is reserved as the identity"
                );
        }

        values.assign(n + 1, NEG_INF);
        bit.assign(n + 1, NEG_INF);

        for (Index i = 1; i <= n; ++i) {
            values[i] = a[i - 1];
            bit[i] = values[i];

            Index parent = i + lowbit(i);
            if (parent <= n)
                bit[parent] = max(bit[parent], bit[i]);
        }

        ops.reset();
    }

    Index size() const override {
        return n;
    }

    Value get(Index i) const override {
        validate_index(i);
        ++ops.node_reads;
        return values[i];
    }

    void assign(Index i, Value v) override {
        validate_index(i);

        if (v == NEG_INF)
            throw invalid_argument(
                "NEG_INF is reserved as the identity"
            );

        ++ops.node_reads;
        Value old = values[i];

        ++ops.node_writes;
        values[i] = v;

        for (Index j = i; j <= n;) {
            ++ops.node_reads;
            Value stored = bit[j];

            if (v >= old) {
                ++ops.combines;
                ++ops.node_writes;
                bit[j] = max(stored, v);
            }
            else if (stored == old) {
                Value rebuilt = NEG_INF;

                Index left = j - lowbit(j) + 1;

                for (Index k = left; k <= j; ++k) {
                    ++ops.node_reads;
                    ++ops.combines;
                    rebuilt = max(rebuilt, values[k]);
                }

                ++ops.recomputations;
                ++ops.node_writes;
                bit[j] = rebuilt;
            }

            if (lowbit(j) > n - j)
                break;

            j += lowbit(j);
        }
    }

    Value query_prefix_max(Index r) const override {
        if (r > n)
            throw out_of_range("Invalid prefix endpoint");

        Value answer = NEG_INF;

        for (Index j = r; j > 0; j -= lowbit(j)) {
            ++ops.node_reads;
            ++ops.combines;
            answer = max(answer, bit[j]);
        }

        return answer;
    }

    const OpCounters& counters() const override {
        return ops;
    }

    void reset_counters() override {
        ops.reset();
    }

    bool check_invariants(string* why = nullptr) const override {
        for (Index i = 1; i <= n; ++i) {
            Value expected = NEG_INF;
            Index left = i - lowbit(i) + 1;

            for (Index j = left; j <= i; ++j)
                expected = max(expected, values[j]);

            if (bit[i] != expected) {
                if (why)
                    *why = "P1 violated at position " +
                           to_string(i);
                return false;
            }
        }

        if (why)
            why->clear();

        return true;
    }
};
