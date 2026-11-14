
#include <iostream>
#include <cassert>
#include <vector>

#include "../src/structA/segment_tree.hpp"
#include "../src/structA/gcd_monoid.hpp"

using namespace std;

int main() {

    vector<Value> a = {12, 18, 24, 30, 42};

    SegTree<GcdMonoid> st(a);

    // Test 1: Entire range.
    assert(st.query(1, 5) == 6);

    // Test 2: Subrange.
    assert(st.query(2, 4) == 6);

    // Test 3: Single-element range.
    assert(st.query(3, 3) == 24);

    // Test 4: Range containing zero.
    st.assign(1, 0);
    assert(st.query(1, 2) == 18);

    // Test 5: Decrease a value.
    st.assign(2, 5);
    assert(st.query(1, 5) == 1);

    // Test 6: Empty range.
    assert(st.query(3, 2) == 0);

    // Test 7: All zeros.
    st.assign(1, 0);
    st.assign(2, 0);
    st.assign(3, 0);
    st.assign(4, 0);
    st.assign(5, 0);

    assert(st.query(1, 5) == 0);

    cout << "All GCD tests passed!" << endl;

    return 0;
}
