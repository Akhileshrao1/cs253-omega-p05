#include "differential_test.hpp"

#include "../src/structA/segment_tree.hpp"
#include "../src/structA/max_count_monoid.hpp"
#include "../src/structA/gcd_monoid.hpp"
#include "../src/structB/dima_ceterchi_max.hpp"

#include <algorithm>
#include <iostream>
#include <random>

template <typename Candidate, typename Runner>
static bool run_case(const char* label, Candidate& candidate,
                     const std::vector<Value>& initial,
                     const std::vector<TestOperation>& operations,
                     Index invariant_stride, Runner runner) {
    TestFailure failure = runner(candidate, initial, operations, invariant_stride);
    if (!failure.failed) {
        std::cout << "PASS: " << label << '\n';
        return true;
    }

    // Re-run each reduced sequence from the same initial state. The predicate
    // returns true only if the same differential/invariant failure persists.
    const auto minimal = shrink_failure(
        failure.sequence,
        [&](const std::vector<TestOperation>& trial) {
            return runner(candidate, initial, trial, invariant_stride).failed;
        });

    std::cerr << "FAIL: " << label << " at operation "
              << failure.operation_index << ": " << failure.message
              << "\nexpected: " << failure.expected
              << "\nactual:   " << failure.actual
              << "\nShrunk reproducer (" << minimal.size() << " operations):\n";
    for (const auto& op : minimal) {
        if (op.kind == OpKind::Assign)
            std::cerr << "A " << op.first << ' ' << op.value << '\n';
        else if (op.kind == OpKind::Read)
            std::cerr << "R " << op.first << '\n';
        else
            std::cerr << "Q " << op.first << ' ' << op.second << '\n';
    }
    return false;
}

int main(int argc, char** argv) {
    TestConfig config;
    config.n = 5000;
    config.operations = 20000;
    if (argc > 1) config.n = static_cast<Index>(stoull(argv[1]));
    if (argc > 2) config.operations = stoull(argv[2]);
    if (argc > 3) config.seed = stoull(argv[3]);
    if (config.n == 0 && config.operations != 0) {
        std::cerr << "n must be positive when operations are requested\n";
        return 2;
    }

    const Index invariant_stride = std::max<Index>(1000, config.n / 10);
    std::cout << "n=" << config.n << " operations=" << config.operations
              << " seed=" << config.seed << '\n';
    bool ok = true;
    {
        std::mt19937_64 rng(config.seed);
        auto initial = generate_initial_array(TestKind::MaxCount, config.n, rng);
        auto ops = generate_operations(TestKind::MaxCount, config, rng);
        SegTree<MaxCountMonoid> candidate;
        ok &= run_case("Structure A MaxCount", candidate, initial, ops,
            invariant_stride,
            [](auto& c, const auto& a, const auto& o, Index stride) {
                return differential_test(c, a, o, stride);
            });
    }
    {
        std::mt19937_64 rng(config.seed + 1);
        auto initial = generate_initial_array(TestKind::Gcd, config.n, rng);
        auto ops = generate_operations(TestKind::Gcd, config, rng);
        SegTree<GcdMonoid> candidate;
        ok &= run_case("Structure A GCD", candidate, initial, ops,
            invariant_stride,
            [](auto& c, const auto& a, const auto& o, Index stride) {
                return differential_test(c, a, o, stride);
            });
    }
    {
        std::mt19937_64 rng(config.seed + 2);
        auto initial = generate_initial_array(TestKind::Max, config.n, rng);
        auto ops = generate_operations(TestKind::Max, config, rng);
        DimaCeterchiMax candidate;
        ok &= run_case("Structure B maximum", candidate, initial, ops,
            invariant_stride,
            [](auto& c, const auto& a, const auto& o, Index stride) {
                return differential_test(c, a, o, stride);
            });
    }

    if (!ok) return 1;
    std::cout << "All available differential tests passed.\n";
    return 0;
}
