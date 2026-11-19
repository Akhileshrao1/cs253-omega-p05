#include "differential_test.hpp"

#include "../src/structA/segment_tree.hpp"
#include "../src/structA/max_count_monoid.hpp"
#include "../src/structA/gcd_monoid.hpp"
#include "../src/structB/dima_ceterchi_max.hpp"

#include <iostream>
#include <random>

static bool report(const char* label, const TestFailure& failure) {
    if (!failure.failed) {
        std::cout << "PASS: " << label << '\n';
        return true;
    }
    std::cerr << "FAIL: " << label << " at operation "
              << failure.operation_index << ": " << failure.message
              << "\nexpected: " << failure.expected
              << "\nactual:   " << failure.actual << '\n';
    std::cerr << "Reproducer (" << failure.sequence.size() << " operations):\n";
    for (const auto& op : failure.sequence) {
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

    bool ok = true;
    {
        std::mt19937_64 rng(config.seed);
        auto initial = generate_initial_array(TestKind::MaxCount, config.n, rng);
        auto ops = generate_operations(TestKind::MaxCount, config, rng);
        SegTree<MaxCountMonoid> candidate;
        ok &= report("Structure A MaxCount", differential_test(candidate, initial, ops, 1000));
    }
    {
        std::mt19937_64 rng(config.seed + 1);
        auto initial = generate_initial_array(TestKind::Gcd, config.n, rng);
        auto ops = generate_operations(TestKind::Gcd, config, rng);
        SegTree<GcdMonoid> candidate;
        ok &= report("Structure A GCD", differential_test(candidate, initial, ops, 1000));
    }
    {
        std::mt19937_64 rng(config.seed + 2);
        auto initial = generate_initial_array(TestKind::Max, config.n, rng);
        auto ops = generate_operations(TestKind::Max, config, rng);
        DimaCeterchiMax candidate;
        ok &= report("Structure B maximum", differential_test(candidate, initial, ops, 1000));
    }

    if (!ok) return 1;
    std::cout << "All available differential tests passed.\n";
    return 0;
}
