#include <bits/stdc++.h>
#include "dima_ceterchi_max.hpp"

using namespace std;

static void expect_equal(Value actual, Value expected, const string& label) {
    if (actual != expected) {
        cerr << "FAILED: " << label
             << " | expected " << expected
             << ", got " << actual << '\n';
        exit(1);
    }
    cout << "PASS: " << label << '\n';
}

int main() {
    // Teammate's requested hand-checked decrease test.
    vector<Value> a = {6, 12, 12, 18};
    DimaCeterchiMax dc;
    dc.build(a);

    expect_equal(dc.query_max(1, 4), 18,
                 "initial maximum of [1, 4]");

    // Decrease the unique maximum from 18 to 12.
    dc.assign(4, 12);
    a[3] = 12;

    expect_equal(dc.query_max(1, 4), 12,
                 "maximum after decreasing 18 to 12");

    // Additional range and update checks.
    expect_equal(dc.query_max(2, 4), 12,
                 "maximum of subrange [2, 4]");
    expect_equal(dc.query_max(1, 1), 6,
                 "single-element query [1, 1]");

    dc.assign(1, 20);
    a[0] = 20;
    expect_equal(dc.query_max(1, 4), 20,
                 "maximum after increasing a non-maximum element");

    dc.assign(1, 7);
    a[0] = 7;
    expect_equal(dc.query_max(1, 4), 12,
                 "maximum after decreasing the current maximum");

    // Verify the internal T1 and T2 interval invariants.
    string why;
    if (!dc.check_invariants(&why)) {
        cerr << "FAILED: invariant check: " << why << '\n';
        return 1;
    }
    cout << "PASS: structural invariants F1, F2 and F3\n";

    cout << "All Structure B tests passed.\n";
    return 0;
}
