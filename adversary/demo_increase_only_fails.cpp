// adversary/demo_increase_only_fails.cpp
// Executable evidence for D5. Prints:
//   Part A  : the counterexample (D5a) - the increase-only rule gives WRONG cells.
//   Part A2 : two different arrays that look IDENTICAL to the rule but need
//             different answers (this is why NO such rule can work).
//   Part B  : the construction of D5(b): measured rescan work vs the exact
//             formula  sum_j lowbit(j)^2  (= 1.5 n^2 - n/2 when n = 2^m).
#include "baseline_sim.hpp"

#include <cstdio>

using namespace adv;

static void print_row(const char* label, const PrefixMaxFenwick& f) {
    std::printf("%-26s", label);
    for (int64_t j = 1; j <= f.n; ++j) std::printf(" %2lld", (long long)f.tree[j]);
    std::printf("\n");
}
static void print_truth(const PrefixMaxFenwick& f) {
    std::printf("%-26s", "true block maxima:");
    for (int64_t j = 1; j <= f.n; ++j) std::printf(" %2lld", (long long)f.true_block_max(j));
    std::printf("\n");
}

int main() {
    std::printf("=== Part A: counterexample for the increase-only rule ===\n");
    std::vector<int64_t> v = {3, 1, 4, 1, 5, 9, 2, 6};
    PrefixMaxFenwick f;
    f.build(v);
    std::printf("%-26s", "cell index:");
    for (int j = 1; j <= 8; ++j) std::printf(" %2d", j);
    std::printf("\n");
    print_row("cells before:", f);
    std::printf("\nassign a[6] = 0   (was 9: a DECREASE of the block maximum)\n\n");
    f.update_increase_only(6, 0);
    print_row("cells after (rule):", f);
    print_truth(f);
    std::printf("wrong cells:");
    for (int64_t j = 1; j <= f.n; ++j)
        if (f.tree[j] != f.true_block_max(j))
            std::printf("  cell %lld stores %lld but the block really has max %lld;", (long long)j,
                        (long long)f.tree[j], (long long)f.true_block_max(j));
    std::printf("\nprefix_max(8) returns %lld, correct answer is %lld\n", (long long)f.prefix_max(8),
                (long long)f.true_block_max(8));

    std::printf("\n=== Part A2: two arrays the rule cannot tell apart ===\n");
    std::vector<int64_t> v1 = {3, 1, 4, 1, 5, 9, 2, 6}, v2 = {3, 1, 4, 1, 1, 9, 2, 6};
    PrefixMaxFenwick p, q;
    p.build(v1);
    q.build(v2);
    std::printf("cell 6 / old a[6] / new value:  array1 = %lld / %lld / 0   array2 = %lld / %lld / 0   (identical)\n",
                (long long)p.tree[6], (long long)p.a[6], (long long)q.tree[6], (long long)q.a[6]);
    p.update_increase_only(6, 0);
    q.update_increase_only(6, 0);
    std::printf("rule's output for cell 6:       array1 = %lld            array2 = %lld          (identical)\n",
                (long long)p.tree[6], (long long)q.tree[6]);
    std::printf("correct value of cell 6:        array1 = %lld            array2 = %lld          (different!)\n",
                (long long)p.true_block_max(6), (long long)q.true_block_max(6));
    std::printf("The missing information sits in cell 5 (a child). The rule never reads it;\n"
                "Structure B recomputes from children, which is exactly why it can handle decreases.\n");

    std::printf("\n=== Part B: D5(b) construction, measured vs formula ===\n");
    std::printf("array a[i]=n+1-i, then assign a[t]=0 for t=1..n  (n = 2^m)\n");
    std::printf("%8s %14s %14s %14s %9s %s\n", "n", "measured", "sum lowbit^2", "1.5n^2-n/2", "reads/n^2", "check");
    bool all_ok = true;
    for (int m = 6; m <= 14; ++m) {
        int64_t n = int64_t(1) << m;
        std::vector<int64_t> arr(n);
        for (int64_t i = 1; i <= n; ++i) arr[i - 1] = n + 1 - i;
        PrefixMaxFenwick g;
        g.build(arr);
        bool correct = true;
        int64_t step = std::max<int64_t>(1, n / 128);
        for (int64_t t = 1; t <= n; ++t) {
            g.update_rescan(t, 0);
            if (t % step == 0) {  // spot-check: baseline still answers correctly
                int64_t truth = PrefixMaxFenwick::NEG;
                for (int64_t i = 1; i <= n; ++i) truth = std::max(truth, g.a[i]);
                if (g.prefix_max(n) != truth) correct = false;
            }
        }
        uint64_t formula = 0;
        for (int64_t j = 1; j <= n; ++j) formula += static_cast<uint64_t>(lowbit(j)) * static_cast<uint64_t>(lowbit(j));
        uint64_t closed = static_cast<uint64_t>(3 * n * n / 2 - n / 2);
        bool ok = (g.rescan_reads == formula) && (formula == closed) && correct;
        all_ok = all_ok && ok;
        std::printf("%8lld %14llu %14llu %14llu %9.3f %s\n", (long long)n, (unsigned long long)g.rescan_reads,
                    (unsigned long long)formula, (unsigned long long)closed,
                    static_cast<double>(g.rescan_reads) / (static_cast<double>(n) * static_cast<double>(n)),
                    ok ? "OK" : "MISMATCH");
    }
    {
        int64_t n = 1000000;
        uint64_t formula = 0;
        for (int64_t j = 1; j <= n; ++j) formula += static_cast<uint64_t>(lowbit(j)) * static_cast<uint64_t>(lowbit(j));
        std::printf("\nFor n = 1,000,000 (not a power of two): sum lowbit^2 = %llu rescan reads for n lowering updates\n",
                    (unsigned long long)formula);
        std::printf("An ordinary Fenwick update would cost about n*log2(n) = %llu in total.\n",
                    (unsigned long long)n * 20);
    }
    std::printf("\n%s\n", all_ok ? "ALL CHECKS PASSED" : "SOME CHECKS FAILED");
    return all_ok ? 0 : 1;
}
