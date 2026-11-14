

#pragma once

#include "../../include/range_structure.hpp"

struct GcdMonoid {
    using T = Value;

    // Calculate GCD using Euclidean Algorithm.
    static T calculate_gcd(T a, T b) {
        while (b != 0) {
            T temp = b;
            b = a % b;
            a = temp;
        }

        return a;
    }

    // Identity element.
    static T identity() {
        return 0;
    }

    // Combine two values.
    static T combine(T a, T b) {
        return calculate_gcd(a, b);
    }
};

