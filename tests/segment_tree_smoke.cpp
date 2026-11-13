
#include <iostream>
#include <cassert>
#include <string>

#include "../src/structA/segment_tree.hpp"

using namespace std;

// First test: sum operation
struct SumMonoid {
    using T = int;

    static T identity() {
        return 0;
    }

    static T combine(T a, T b) {
        return a + b;
    }
};

// Second test: string concatenation.
// This checks that the tree preserves left-to-right order.
struct StringMonoid {
    using T = string;

    static T identity() {
        return "";
    }

    static T combine(const T& a, const T& b) {
        return a + b;
    }
};

int main() {
    // Test 1: Build and range sum.
    SegTree<SumMonoid> st({2, 4, 6, 8, 10});

    assert(st.size() == 5);
    assert(st.query(2, 4) == 18);
    assert(st.get(3) == 6);

    // Test 2: Point assignment.
    st.assign(3, 1);

    assert(st.get(3) == 1);
    assert(st.query(2, 4) == 13);

    // Test 3: Decrease an existing value.
    st.assign(1, 0);

    assert(st.query(1, 5) == 23);

    // Test 4: Empty range.
    assert(st.query(2, 1) == 0);

    // Test 5: Preserve order for non-commutative operation.
    SegTree<StringMonoid> words({"a", "b", "c", "d"});

    assert(words.query(2, 4) == "bcd");

    cout << "All segment tree smoke tests passed!" << endl;

    return 0;
}
