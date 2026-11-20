

#pragma once

#include "../../include/range_structure.hpp"

#include <vector>
#include <cstddef>
#include <stdexcept>
#include <limits>
#include <type_traits>
#include <string>
#include <algorithm>

using namespace std;

template <typename M, typename = void>
struct HasSegmentTreeInterface : false_type {};

template <typename M>
struct HasSegmentTreeInterface<
    M, void_t<typename M::Interface>
> : true_type {};

template <
    typename M,
    bool HasInterface = HasSegmentTreeInterface<M>::value
>
class SegTree;

template <typename M>
class SegTree<M, true> final : public M::Interface {
public:
    using T = typename M::T;

private:
    Index n = 0;
    Index built_size = 0;
    Index cap = 1;

    vector<T> tree;
    vector<Value> raw_values;

    mutable OpCounters op_counters;

    T read_node(Index p) const {
        ++op_counters.node_reads;
        return tree[p];
    }

    void write_node(Index p, const T& value) {
        ++op_counters.node_writes;
        tree[p] = value;
    }

    T combine_nodes(const T& a, const T& b) const {
        ++op_counters.combines;
        return M::combine(a, b, &op_counters);
    }

    void prepare_tree(Index count) {
        n = count;
        built_size = count;
        cap = 1;

        while (cap < n) {
            if (cap > numeric_limits<Index>::max() / 2) {
                throw length_error("Segment tree is too large");
            }
            cap *= 2;
        }

        if (cap > numeric_limits<Index>::max() / 2) {
            throw length_error("Segment tree is too large");
        }

        tree.assign(2 * cap, M::identity());
        op_counters.node_writes += 2 * cap;

        raw_values.clear();
        raw_values.reserve(n);
    }

    void rebuild_path(Index p) {
        while (p > 1) {
            p /= 2;

            T left = read_node(2 * p);
            T right = read_node(2 * p + 1);

            T result = combine_nodes(left, right);

            ++op_counters.recomputations;
            write_node(p, result);
        }
    }

    void assign_value(Index i, Value v) {
        if (i < 1 || i > n) {
            throw out_of_range("Invalid segment tree index");
        }

        T lifted = M::from_value(v);

        ++op_counters.node_writes;
        raw_values[i - 1] = v;

        Index p = cap + i - 1;
        write_node(p, lifted);

        rebuild_path(p);
    }

    T query_internal(Index l, Index r) const {
        if (l == 0 || r > n || (l > r && l - r != 1)) {
            throw out_of_range("Invalid segment tree range");
        }

        if (l == r + 1) {
            return M::identity();
        }

        Index left = cap + l - 1;
        Index right = cap + r - 1;

        T left_result = M::identity();
        T right_result = M::identity();

        while (left <= right) {
            if (left % 2 == 1) {
                left_result = combine_nodes(
                    left_result, read_node(left)
                );
                ++left;
            }

            if (right % 2 == 0) {
                right_result = combine_nodes(
                    read_node(right), right_result
                );
                --right;
            }

            left /= 2;
            right /= 2;
        }

        return combine_nodes(left_result, right_result);
    }

    bool fail(string* why, const string& message) const {
        if (why != nullptr) {
            *why = message;
        }
        return false;
    }

    void build_lifted(const vector<T>& a) {
        op_counters.reset();
        prepare_tree(a.size());

        for (Index i = 0; i < n; ++i) {
            T input = a[i];
            ++op_counters.node_reads;

            Value v = M::to_value(input);
            T expected = M::from_value(v);

            if constexpr (is_same<T, MaxCount>::value) {
                if (input.max != expected.max ||
                    input.count != expected.count) {
                    throw invalid_argument("Invalid MaxCount leaf");
                }
            }

            raw_values.push_back(v);
            ++op_counters.node_writes;

            write_node(cap + i, input);
        }

        for (Index p = cap; p-- > 1;) {
            T left = read_node(2 * p);
            T right = read_node(2 * p + 1);

            write_node(p, combine_nodes(left, right));
        }
    }

public:
    SegTree() = default;

    explicit SegTree(const vector<Value>& a) {
        build(a);
    }

    // Backward-compatible constructor for pre-lifted MaxCount leaves.
    template <
        typename U = T,
        typename enable_if<!is_same<U, Value>::value, int>::type = 0
    >
    explicit SegTree(const vector<T>& a) {
        build_lifted(a);
    }

    // Official project build interface.
    void build(const vector<Value>& a) override {
        op_counters.reset();
        prepare_tree(a.size());

        for (Index i = 0; i < n; ++i) {
            Value v = a[i];
            ++op_counters.node_reads;

            T lifted = M::from_value(v);

            raw_values.push_back(v);
            ++op_counters.node_writes;

            write_node(cap + i, lifted);
        }
        for (Index p = cap; p-- > 1;) {
            T left = read_node(2 * p);
            T right = read_node(2 * p + 1);

            write_node(p, combine_nodes(left, right));
        }
    }

    // Backward-compatible build for pre-lifted values.
    template <
        typename U = T,
        typename enable_if<!is_same<U, Value>::value, int>::type = 0
    >
    void build(const vector<U>& a) {
        build_lifted(a);
    }

    Index size() const override {
        return n;
    }

    // Dynamic array payload only; excludes the vector objects and allocator metadata.
    size_t storage_bytes() const {
        return tree.capacity() * sizeof(T) +
               raw_values.capacity() * sizeof(Value);
    }

    Value get(Index i) const override {
        if (i < 1 || i > n) {
            throw out_of_range("Invalid segment tree index");
        }

        return M::to_value(read_node(cap + i - 1));
    }

    // Official project assignment interface.
    void assign(Index i, Value v) override {
        assign_value(i, v);
    }

    // Backward-compatible assignment for pre-lifted MaxCount values.
    template <
        typename U = T,
        typename enable_if<!is_same<U, Value>::value, int>::type = 0
    >
    void assign(Index i, const U& value) {
        assign_value(i, M::to_value(value));
    }

    // Generic monoid query, retained for existing smoke tests.
    T query(Index l, Index r) const {
        return query_internal(l, r);
    }

    // RangeMax interface.
    Value query_max(Index l, Index r) const {
        return M::to_value(query_internal(l, r));
    }

    // RangeMaxCount interface.
    MaxCount query_max_count(Index l, Index r) const {
        if constexpr (is_same<T, MaxCount>::value) {
            return query_internal(l, r);
        } else {
            throw logic_error("query_max_count requires MaxCountMonoid");
        }
    }

    // RangeGcd interface.
    Value query_gcd(Index l, Index r) const {
        if constexpr (is_same<T, Value>::value) {
            return query_internal(l, r);
        } else {
            throw logic_error("query_gcd requires GcdMonoid");
        }
    }

    const OpCounters& counters() const override {
        return op_counters;
    }

    void reset_counters() override {
        op_counters.reset();
    }

    bool check_invariants(string* why = nullptr) const override {
        if (n != built_size || raw_values.size() != n) {
            return fail(why, "G1: size changed between builds");
        }

        // G2 and S1: real leaves match the stored input values.
        for (Index i = 0; i < n; ++i) {
            Value v = raw_values[i];
            ++op_counters.node_reads;

            T actual = read_node(cap + i);
            T expected = M::from_value(v);

            if constexpr (is_same<T, MaxCount>::value) {
                if (actual.max != expected.max ||
                    actual.count != expected.count) {
                    return fail(why, "S1/G2: incorrect real leaf");
                }
            } else {
                if (actual != expected) {
                    return fail(why, "S1/G2: incorrect real leaf");
                }
            }
        }

        // S2: padded leaves must contain the identity.
        for (Index i = n; i < cap; ++i) {
            T actual = read_node(cap + i);
            T identity = M::identity();

            if constexpr (is_same<T, MaxCount>::value) {
                if (actual.max != identity.max ||
                    actual.count != identity.count) {
                    return fail(why, "S2: incorrect padded leaf");
                }
            } else {
                if (actual != identity) {
                    return fail(why, "S2: incorrect padded leaf");
                }
            }
        }

        // S3: every internal node must equal its children combined.
        for (Index p = 1; p < cap; ++p) {
            T left = read_node(2 * p);
            T right = read_node(2 * p + 1);
            T actual = read_node(p);
            T expected = combine_nodes(left, right);

            if constexpr (is_same<T, MaxCount>::value) {
                if (actual.max != expected.max ||
                    actual.count != expected.count) {
                    return fail(why, "S3: incorrect internal node");
                }
            } else {
                if (actual != expected) {
                    return fail(why, "S3: incorrect internal node");
                }
            }
        }

        // S4: MaxCount count validity follows from S1, S2 and S3.
        // Leaves have count 1, padded leaves have count 0,
        // and internal counts are obtained by combining children.

        // S5: GCD divisibility and zero properties follow from
        // S1, S2 and S3 because each internal node stores the
        // GCD of its two children.
        // G3: query/get are const and do not modify tree or raw values.
        // G4: empty-range identities are checked by the query methods.
        if (why != nullptr) {
            why->clear();
        }

        return true;
    }
};
// Generic segment tree for monoids that do not implement
// one of the project's Structure interfaces.
template <typename M>
class SegTree<M, false> {
public:
    using T = typename M::T;

private:
    Index n = 0;
    Index cap = 1;
    vector<T> tree;

public:
    SegTree() = default;

    explicit SegTree(const vector<T>& a) {
        build(a);
    }

    void build(const vector<T>& a) {
        n = a.size();
        cap = 1;

        while (cap < n) {
            cap *= 2;
        }

        tree.assign(2 * cap, M::identity());

        for (Index i = 0; i < n; ++i) {
            tree[cap + i] = a[i];
        }

        for (Index p = cap; p-- > 1;) {
            tree[p] = M::combine(
                tree[2 * p],
                tree[2 * p + 1]
            );
        }
    }

    Index size() const {
        return n;
    }

    T get(Index i) const {
        if (i < 1 || i > n) {
            throw out_of_range("Invalid segment tree index");
        }

        return tree[cap + i - 1];
    }

    void assign(Index i, const T& value) {
        if (i < 1 || i > n) {
            throw out_of_range("Invalid segment tree index");
        }

        Index p = cap + i - 1;
        tree[p] = value;

        while (p > 1) {
            p /= 2;

            tree[p] = M::combine(
                tree[2 * p],
                tree[2 * p + 1]
            );
        }
    }

    T query(Index l, Index r) const {
        if (l == 0 || r > n ||
            (l > r && l - r != 1)) {
            throw out_of_range("Invalid segment tree range");
        }

        if (l == r + 1) {
            return M::identity();
        }

        Index left = cap + l - 1;
        Index right = cap + r - 1;

        T left_result = M::identity();
        T right_result = M::identity();

        while (left <= right) {
            if (left % 2 == 1) {
                left_result = M::combine(
                    left_result, tree[left]
                );
                ++left;
            }

            if (right % 2 == 0) {
                right_result = M::combine(
                    tree[right], right_result
                );
                --right;
            }

            left /= 2;
            right /= 2;
        }

        return M::combine(left_result, right_result);
    }
};
