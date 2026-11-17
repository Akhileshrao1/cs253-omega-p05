
#include <iostream>
#include <cassert>
#include <vector>

#include "../src/structA/segment_tree.hpp"
#include "../src/structA/max_count_monoid.hpp"

using namespace std;

int main() {

    vector<Value> a = {5, 8, 8, 3, 8};

    vector<MaxCount> leaves;

    for (Value v : a) {
        leaves.push_back(MaxCountMonoid::from_value(v));
    }

    SegTree<MaxCountMonoid> st(leaves);

    // Test 1: Entire range.
    MaxCount result = st.query(1, 5);

    assert(result.max == 8);
    assert(result.count == 3);

    // Test 2: Subrange.
    result = st.query(1, 3);

    assert(result.max == 8);
    assert(result.count == 2);

    // Test 3: Update a value.
    st.assign(2, MaxCountMonoid::from_value(4));

    result = st.query(1, 5);

    assert(result.max == 8);
    assert(result.count == 2);

    // Test 4: Decrease another maximum.
    st.assign(3, MaxCountMonoid::from_value(1));

    result = st.query(1, 5);

    assert(result.max == 8);
    assert(result.count == 1);
    // Test 5: All values become equal.
st.assign(1, MaxCountMonoid::from_value(8));
st.assign(4, MaxCountMonoid::from_value(8));

result = st.query(1, 5);

assert(result.max == 8);
assert(result.count == 3);

// Test 6: Single-element range.
result = st.query(4, 4);

assert(result.max == 8);
assert(result.count == 1);

    cout << "All MaxCountMonoid tests passed!" << endl;

    return 0;
}
