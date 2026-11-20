#pragma once

#include "../../include/range_structure.hpp"

#include <algorithm>

// Prefix-only baseline using the rescan strategy from adversary/baseline_sim.hpp.
class PrefixMaxFenwick final : public PrefixMax {
public:
    void build(const vector<Value>& input) override {
        if (std::find(input.begin(), input.end(), NEG_INF) != input.end())
            throw invalid_argument("NEG_INF is reserved as the identity");

        values.assign(input.size() + 1, NEG_INF);
        tree.assign(input.size() + 1, NEG_INF);
        for (Index i = 1; i <= size(); ++i) {
            values[i] = input[i - 1];
            tree[i] = std::max(tree[i], values[i]);
            if (lowbit(i) <= size() - i) {
                const Index parent = i + lowbit(i);
                tree[parent] = std::max(tree[parent], tree[i]);
            }
        }
        reset_counters();
    }

    Index size() const override { return values.empty() ? 0 : values.size() - 1; }

    // Dynamic array payload only; excludes the vector objects and allocator metadata.
    size_t storage_bytes() const {
        return (values.capacity() + tree.capacity()) * sizeof(Value);
    }

    Value get(Index i) const override {
        validate_index(i);
        ++ops.node_reads;
        return values[i];
    }

    void assign(Index i, Value v) override {
        validate_index(i);
        if (v == NEG_INF) throw invalid_argument("NEG_INF is reserved as the identity");

        ++ops.node_reads;
        const Value old = values[i];
        ++ops.node_writes;
        values[i] = v;
        for (Index j = i; j <= size(); j += lowbit(j)) {
            ++ops.node_reads;
            const Value stored = tree[j];
            if (v >= old) {
                ++ops.combines;
                ++ops.node_writes;
                tree[j] = std::max(stored, v);
            } else if (stored == old) {
                Value maximum = NEG_INF;
                for (Index k = j - lowbit(j) + 1; k <= j; ++k) {
                    ++ops.node_reads;
                    ++ops.combines;
                    maximum = std::max(maximum, values[k]);
                }
                ++ops.recomputations;
                ++ops.node_writes;
                tree[j] = maximum;
            }
            if (lowbit(j) > size() - j) break;
        }
    }

    Value query_prefix_max(Index r) const override {
        if (r > size()) throw out_of_range("Invalid prefix endpoint");
        Value maximum = NEG_INF;
        for (Index j = r; j > 0; j -= lowbit(j)) {
            ++ops.node_reads;
            ++ops.combines;
            maximum = std::max(maximum, tree[j]);
        }
        return maximum;
    }

    const OpCounters& counters() const override { return ops; }
    void reset_counters() override { ops.reset(); }

    bool check_invariants(string* why = nullptr) const override {
        for (Index j = 1; j <= size(); ++j) {
            Value maximum = NEG_INF;
            for (Index k = j - lowbit(j) + 1; k <= j; ++k)
                maximum = std::max(maximum, values[k]);
            if (tree[j] != maximum) {
                if (why) *why = "P1: incorrect block at position " + std::to_string(j);
                return false;
            }
        }
        if (why) why->clear();
        return true;
    }

private:
    vector<Value> values, tree;
    mutable OpCounters ops;

    static Index lowbit(Index i) { return i & (Index{0} - i); }

    void validate_index(Index i) const {
        if (i == 0 || i > size()) throw out_of_range("Invalid position");
    }
};
