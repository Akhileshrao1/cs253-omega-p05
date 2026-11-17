



#pragma once

#include "../../include/range_structure.hpp"
#include <stdexcept>

struct GcdMonoid {
    using T = Value;
    using Interface = RangeGcd;

    static T from_value(Value v) {
        if (v < 0 || v > 1000000000LL) {
            throw std::invalid_argument("GCD value must be between 0 and 1e9");
        }
        return v;
    }

    static Value to_value(T v) {
        return v;
    }

    static T calculate_gcd(T a, T b, OpCounters* counters = nullptr) {
        while (b != 0) {
            if (counters != nullptr) {
                counters->gcd_steps++;
            }

            T temp = b;
            b = a % b;
            a = temp;
        }
        return a;
    }

    static T identity() {
        return 0;
    }

    static T combine(T a, T b) {
        return calculate_gcd(a, b);
    }

    static T combine(T a, T b, OpCounters* counters) {
        return calculate_gcd(a, b, counters);
    }
};

