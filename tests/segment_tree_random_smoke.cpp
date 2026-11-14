
#include <iostream>
#include <vector>
#include <random>
#include <cassert>
#include <algorithm>
#include <numeric>
#include <string>

#include "../src/structA/segment_tree.hpp"
#include "../src/structA/max_count_monoid.hpp"
#include "../src/structA/gcd_monoid.hpp"

using namespace std;
Value calculate_gcd(Value a, Value b) {
    while (b != 0) {
        Value temp = b;
        b = a % b;
        a = temp;
    }
    return a;
}

int main() {
    mt19937 rng(42);

    const int n = 37;
    vector<Value> a(n);

    for (auto& x : a) {
        x = rng() % 1001;
    }

    SegTree<MaxCountMonoid> mx;
    SegTree<GcdMonoid> gd;

    mx.build(a);
    gd.build(a);

    string why;

    // Check the initial trees.
    assert(mx.check_invariants(&why));
    assert(gd.check_invariants(&why));

    // Perform 500 random operations.
    for (int step = 0; step < 500; ++step) {
        int operation = rng() % 2;

        if (operation == 0) {
            // Random point assignment.
            int index = rng() % n;
            Value new_value = rng() % 1001;

            a[index] = new_value;

            mx.assign(index + 1, new_value);
            gd.assign(index + 1, new_value);
        } else {
            // Random range query.
            int l = rng() % n;
            int r = rng() % n;

            if (l > r) swap(l, r);

            Value expected_max = a[l];
            uint64_t expected_count = 0;
            Value expected_gcd = 0;

            for (int i = l; i <= r; ++i) {
                expected_max = max(expected_max, a[i]);
                expected_gcd = calculate_gcd(expected_gcd, a[i]);
            }

            for (int i = l; i <= r; ++i) {
                if (a[i] == expected_max) {
                    ++expected_count;
                }
            }

            MaxCount actual = mx.query_max_count(l + 1, r + 1);

            assert(actual.max == expected_max);
            assert(actual.count == expected_count);

            assert(mx.query_max(l + 1, r + 1) == expected_max);
            assert(gd.query_gcd(l + 1, r + 1) == expected_gcd);
        }

        // Verify every element.
        for (int i = 0; i < n; ++i) {
            assert(mx.get(i + 1) == a[i]);
            assert(gd.get(i + 1) == a[i]);
        }

        // Verify structural invariants after every operation.
        assert(mx.check_invariants(&why));
        assert(gd.check_invariants(&why));
    }

    cout << "All 500 randomized differential tests passed!" << endl;

    return 0;
}
