
#pragma once
#include <stdexcept>
#include "../../include/range_structure.hpp"

// Stores the maximum value and its frequency.
struct MaxCountMonoid {
    using T = MaxCount;

    // Identity element: no actual elements.
    static T identity() {
        return {NEG_INF, 0};
    }

    // Convert an actual array value into a leaf.
    static T from_value(Value v) {
        if (v == NEG_INF) {
           throw std::invalid_argument("NEG_INF is reserved");
        }

        return {v, 1};
    }

    // Combine information from two child nodes.
    static T combine(const T& a, const T& b) {

        // Left child has a greater maximum.
        if (a.max > b.max) {
            return a;
        }

        // Right child has a greater maximum.
        if (b.max > a.max) {
            return b;
        }

        // Both children have the same maximum.
        return {a.max, a.count + b.count};
    }
};
