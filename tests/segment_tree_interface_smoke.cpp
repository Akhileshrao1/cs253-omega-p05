
#include <iostream>
#include <cassert>
#include <vector>

#include "../src/structA/segment_tree.hpp"
#include "../src/structA/max_count_monoid.hpp"
#include "../src/structA/gcd_monoid.hpp"

using namespace std;

int main() {
    // Test the official RangeMaxCount interface.
    SegTree<MaxCountMonoid> mx;

    vector<Value> a = {5, 8, 8, 3, 8};
    mx.build(a);

    RangeMaxCount* max_interface = &mx;

    assert(max_interface->size() == 5);
    assert(max_interface->get(2) == 8);

    MaxCount result = max_interface->query_max_count(1, 5);
    assert(result.max == 8 && result.count == 3);

    assert(max_interface->query_max(1, 5) == 8);
    assert(max_interface->query_max_count(3, 2).count == 0);

    string why;
    assert(max_interface->check_invariants(&why));

    max_interface->assign(2, 1);
    assert(max_interface->get(2) == 1);
    assert(max_interface->check_invariants(&why));

    // Test the official RangeGcd interface.
    SegTree<GcdMonoid> gd;

    vector<Value> b = {12, 18, 24, 30, 42};
    gd.build(b);

    RangeGcd* gcd_interface = &gd;

    assert(gcd_interface->size() == 5);
    assert(gcd_interface->get(1) == 12);
    assert(gcd_interface->query_gcd(1, 5) == 6);
    assert(gcd_interface->query_gcd(3, 2) == 0);

    assert(gcd_interface->check_invariants(&why));

    gcd_interface->assign(1, 0);
    assert(gcd_interface->get(1) == 0);
    assert(gcd_interface->check_invariants(&why));

    // Test counters.
    gd.reset_counters();
    gcd_interface->query_gcd(1, 5);

    assert(gcd_interface->counters().node_reads > 0);
    assert(gcd_interface->counters().combines > 0);
    assert(gcd_interface->counters().gcd_steps > 0);

    cout << "All segment tree interface tests passed!" << endl;

    return 0;
}
