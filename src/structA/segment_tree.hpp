
#pragma once

#include <vector>
#include <cstddef>
#include <stdexcept>
#include <limits>

using namespace std;

template <typename M>
class SegTree {
public:
    using T = typename M::T;

private:
    size_t n = 0;
    size_t cap = 1;
    vector<T> tree;

public:
    SegTree() = default;

    explicit SegTree(const vector<T>& a) {
        build(a);
    }

    // Build using zero-based input vector.
    // Public positions after building are 1-based.
    void build(const vector<T>& a) {
        n = a.size();

        cap = 1;
        while (cap < n) {
            if (cap > numeric_limits<size_t>::max() / 2) {
                throw length_error("Segment tree is too large");
            }
            cap *= 2;
        }

        if (cap > numeric_limits<size_t>::max() / 2) {
            throw length_error("Segment tree is too large");
        }

        tree.assign(2 * cap, M::identity());

        // Put actual values in the leaves.
        for (size_t i = 0; i < n; i++) {
            tree[cap + i] = a[i];
        }

        // Build internal nodes from bottom to top.
        for (size_t i = cap; i-- > 1;) {
            tree[i] = M::combine(
                tree[2 * i],
                tree[2 * i + 1]
            );
        }
    }

    size_t size() const {
        return n;
    }

    // Get the value at public position i (1-based).
    T get(size_t i) const {
        if (i < 1 || i > n) {
            throw out_of_range("Invalid segment tree index");
        }

        return tree[cap + i - 1];
    }

    // Assign a new value at public position i (1-based).
    void assign(size_t i, const T& value) {
        if (i < 1 || i > n) {
            throw out_of_range("Invalid segment tree index");
        }

        size_t p = cap + i - 1;
        tree[p] = value;

        // Recalculate all ancestors.
        while (p > 1) {
            p /= 2;

            tree[p] = M::combine(
                tree[2 * p],
                tree[2 * p + 1]
            );
        }
    }

    // Query the closed interval [l, r].
    // Empty range l == r + 1 returns the identity.
    T query(size_t l, size_t r) const {
        if (l < 1 || r > n || l > r + 1) {
            throw out_of_range("Invalid segment tree range");
        }

        if (l == r + 1) {
            return M::identity();
        }

        size_t left = cap + l - 1;
        size_t right = cap + r - 1;

        T left_result = M::identity();
        T right_result = M::identity();

        while (left <= right) {
            if (left % 2 == 1) {
                left_result = M::combine(
                    left_result, tree[left]
                );
                left++;
            }

            if (right % 2 == 0) {
                right_result = M::combine(
                    tree[right], right_result
                );
                right--;
            }

            left /= 2;
            right /= 2;
        }

        return M::combine(left_result, right_result);
    }
};
