
#include <iostream>
#include <cstdlib>
#include "prefix_max_fenwick.hpp"

using namespace std;

void expect_equal(Value actual, Value expected, const string& label) {
    if (actual != expected) {
        cerr << "FAILED: " << label
             << " | expected " << expected
             << ", got " << actual << '\n';
        exit(1);
    }

    cout << "PASS: " << label << '\n';
}

void expect_invariants(const PrefixMaxFenwick& fw) {
    string why;

    if (!fw.check_invariants(&why)) {
        cerr << "FAILED: " << why << '\n';
        exit(1);
    }

    cout << "PASS: structural invariant P1\n";
}

int main() {
    PrefixMaxFenwick fw;

    vector<Value> a = {6, 12, 12, 18};
    fw.build(a);

    expect_equal(fw.size(), 4, "size after build");
    expect_equal(fw.query_prefix_max(0), NEG_INF, "empty prefix");
    expect_equal(fw.query_prefix_max(1), 6, "prefix [1,1]");
    expect_equal(fw.query_prefix_max(4), 18, "initial prefix maximum");

    // Decrease the unique maximum.
    fw.reset_counters();
    fw.assign(4, 12);

    expect_equal(fw.query_prefix_max(4), 12,
                 "maximum after decreasing 18 to 12");

    expect_equal(fw.counters().recomputations, 1,
                 "one cell rebuilt after first decrease");

    expect_invariants(fw);

    // Decrease one of two tied maximum values.
    fw.reset_counters();
    fw.assign(2, 5);

    expect_equal(fw.query_prefix_max(2), 6,
                 "maximum after decreasing one tied maximum");

    expect_equal(fw.query_prefix_max(4), 12,
                 "other tied maximum remains");

    expect_equal(fw.counters().recomputations, 2,
                 "two cells rebuilt after tied-maximum decrease");

    expect_invariants(fw);

    // Increase a value.
    fw.reset_counters();
    fw.assign(1, 20);

    expect_equal(fw.query_prefix_max(4), 20,
                 "maximum after an increase");

    expect_equal(fw.counters().recomputations, 0,
                 "no rebuilds after an increase");

    expect_invariants(fw);

    // Decrease the current maximum.
    fw.reset_counters();
    fw.assign(1, 7);

    expect_equal(fw.query_prefix_max(2), 7,
                 "prefix maximum after decreasing current maximum");

    expect_equal(fw.query_prefix_max(4), 12,
                 "maximum outside updated prefix remains");

    expect_equal(fw.counters().recomputations, 3,
                 "three cells rebuilt after current-maximum decrease");

    expect_invariants(fw);
    PrefixMaxFenwick regression;
    regression.build({9, 1});

    expect_equal(regression.query_prefix_max(2), 9,
                 "regression: prefix maximum of [9, 1]");

    expect_invariants(regression);

    cout << "\nAll PrefixMaxFenwick tests passed!\n";
    return 0;
}
