// adversary/baseline_sim.hpp
// A tiny, self-contained model of the BASELINE: a prefix-maximum Fenwick tree.
// Used by the D5 demo and by check_input to verify cost predictions.
// (Plain std::vector is fine here: this is test code, not a structure under study.)
#pragma once

#include "common.hpp"

#include <algorithm>
#include <vector>

namespace adv {

class PrefixMaxFenwick {
public:
    static constexpr int64_t NEG = INT64_MIN;

    int64_t n = 0;
    std::vector<int64_t> a;     // raw array, 1-based
    std::vector<int64_t> tree;  // tree[j] = max of block (j - lowbit(j), j]
    uint64_t stale_rebuilds = 0;  // cells that had to be rebuilt
    uint64_t rescan_reads = 0;    // array elements read while rebuilding

    void build(const std::vector<int64_t>& v) {
        n = static_cast<int64_t>(v.size());
        a.assign(n + 1, 0);
        tree.assign(n + 1, NEG);
        for (int64_t i = 1; i <= n; ++i) a[i] = v[i - 1];
        for (int64_t i = 1; i <= n; ++i) {
            tree[i] = std::max(tree[i], a[i]);
            int64_t parent = i + lowbit(i);
            if (parent <= n) tree[parent] = std::max(tree[parent], tree[i]);
        }
    }

    int64_t prefix_max(int64_t i) const {
        int64_t m = NEG;
        for (int64_t j = i; j > 0; j -= lowbit(j)) m = std::max(m, tree[j]);
        return m;
    }

    // What the block of cell j REALLY contains (brute force).
    int64_t true_block_max(int64_t j) const {
        int64_t m = NEG;
        for (int64_t p = j - lowbit(j) + 1; p <= j; ++p) m = std::max(m, a[p]);
        return m;
    }

    // The INCREASE-ONLY rule: tree[j] = max(tree[j], v). Wrong when v < old.
    // Kept only to demonstrate D5(a).
    void update_increase_only(int64_t i, int64_t v) {
        a[i] = v;
        for (int64_t j = i; j <= n; j += lowbit(j)) tree[j] = std::max(tree[j], v);
    }

    // Correct baseline: increases patch cells; a decrease rescans every cell
    // whose stored value equals the old element (it might have been the max).
    void update_rescan(int64_t i, int64_t v) {
        int64_t old = a[i];
        a[i] = v;
        if (v >= old) {
            for (int64_t j = i; j <= n; j += lowbit(j)) tree[j] = std::max(tree[j], v);
            return;
        }
        for (int64_t j = i; j <= n; j += lowbit(j)) {
            if (tree[j] == old) {  // old element may have been this block's max
                int64_t m = NEG;
                for (int64_t p = j - lowbit(j) + 1; p <= j; ++p) m = std::max(m, a[p]);
                tree[j] = m;
                ++stale_rebuilds;
                rescan_reads += static_cast<uint64_t>(lowbit(j));
            }
        }
    }
};

}  // namespace adv
