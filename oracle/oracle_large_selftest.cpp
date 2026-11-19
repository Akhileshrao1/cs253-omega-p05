#include "../oracle/oracle.hpp"
#include "../oracle/differential_test.hpp"
#include <iostream>

int main(int argc, char** argv) {
    TestConfig c;
    if (argc > 1) c.n = static_cast<Index>(stoull(argv[1]));
    if (argc > 2) c.operations = stoull(argv[2]);
    if (argc > 3) c.seed = stoull(argv[3]);

    mt19937_64 rng(c.seed);
    auto initial = generate_initial_array(TestKind::MaxCount, c.n, rng);
    auto ops = generate_operations(TestKind::MaxCount, c, rng);

    OracleMaxCount oracle;
    oracle.build(initial);
    for (const auto& op : ops) {
        if (op.kind == OpKind::Assign) oracle.assign(op.first, op.value);
        else if (op.kind == OpKind::Read) (void)oracle.get(op.first);
        else (void)oracle.query_max_count(op.first, op.second);
    }
    string why;
    if (!oracle.check_invariants(&why)) {
        cerr << "Invariant failure: " << why << "\n";
        return 1;
    }
    const auto& cts = oracle.counters();
    cout << "Oracle workload completed\n"
         << "seed=" << c.seed << " n=" << c.n << " operations=" << c.operations << "\n"
         << "node_reads=" << cts.node_reads << " node_writes=" << cts.node_writes
         << " combines=" << cts.combines << "\n";
}
