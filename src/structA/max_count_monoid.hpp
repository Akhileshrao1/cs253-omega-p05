

#pragma once

#include <stdexcept>
#include "../../include/range_structure.hpp"

struct MaxCountMonoid {
    using T = MaxCount;
    using Interface = RangeMaxCount;

    static T identity() {
        return {NEG_INF, 0};
    }

    static T from_value(Value v) {
        if (v == NEG_INF) {
            throw std::invalid_argument("NEG_INF is reserved");
        }
        return {v, 1};
    }

    static Value to_value(const T& v) {
        return v.max;
    }

    static T combine(const T& a, const T& b) {
        if (a.max > b.max) return a;
        if (b.max > a.max) return b;
        return {a.max, a.count + b.count};
    }

    // Counter-aware overload. MaxCount has no Euclidean steps.
    static T combine(const T& a, const T& b, OpCounters*) {
        return combine(a, b);
    }
};
