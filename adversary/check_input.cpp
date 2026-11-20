// adversary/check_input.cpp
// Validates a generated file and reports statistics. With --baseline 1 it also
// REPLAYS the reading updates through the prefix-max Fenwick baseline MODEL
// (the team's own baseline is not written yet) and reports how many cells were
// rebuilt and how many elements rescanned, so a generator's prediction can be
// checked against an independent simulation.
//
// Usage: check_input --in file.txt [--baseline 1] [--budget 2000000000]
#include "baseline_sim.hpp"
#include "reader.hpp"

#include <algorithm>
#include <string>

using namespace adv;

int main(int argc, char** argv) {
    Args args(argc, argv);
    std::string in = args.str("in", "");
    int64_t baseline = args.num("baseline", 0);
    int64_t budget = args.num("budget", 2000000000LL);  // stop after this many rescan reads
    args.finish();
    if (in.empty()) die("usage: check_input --in <file> [--baseline 1]");

    Instance inst = read_instance(in);
    const int64_t n = static_cast<int64_t>(inst.readings.size());

    PrefixMaxFenwick sim;
    if (baseline) sim.build(inst.readings);
    std::vector<int64_t> cur = inst.readings;

    uint64_t r_upd = 0, c_upd = 0, queries = 0, dec = 0, inc = 0, same = 0, sum_w = 0, replayed_dec = 0;
    int64_t max_w = 0;
    bool stopped = false;
    size_t stopped_at = 0;
    for (size_t k = 0; k < inst.ops.size(); ++k) {
        const Op& op = inst.ops[k];
        if (op.type == 'R') {
            ++r_upd;
            bool is_dec = (op.b < cur[op.a - 1]);
            if (is_dec) ++dec;
            else if (op.b > cur[op.a - 1]) ++inc;
            else ++same;
            cur[op.a - 1] = op.b;
            if (baseline && !stopped) {
                if (is_dec) ++replayed_dec;
                sim.update_rescan(op.a, op.b);
                if (static_cast<int64_t>(sim.rescan_reads) > budget) { stopped = true; stopped_at = k + 1; }
            }
        } else if (op.type == 'C') {
            ++c_upd;
        } else {
            ++queries;
            sum_w += static_cast<uint64_t>(op.b - op.a + 1);
            max_w = std::max<int64_t>(max_w, op.b - op.a + 1);
        }
    }

    const size_t m = inst.ops.size();
    std::printf("OK  file=%s\n", in.c_str());
    std::printf("n=%lld  commands=%zu  (R=%llu, C=%llu, Q=%llu)\n", (long long)n, m, (unsigned long long)r_upd,
                (unsigned long long)c_upd, (unsigned long long)queries);
    std::printf("reading updates: decreases=%llu increases=%llu unchanged=%llu  (decreases = %.1f%% of all commands)\n",
                (unsigned long long)dec, (unsigned long long)inc, (unsigned long long)same,
                m ? 100.0 * static_cast<double>(dec) / static_cast<double>(m) : 0.0);
    if (queries)
        std::printf("query width: mean=%.1f max=%lld\n", static_cast<double>(sum_w) / static_cast<double>(queries), (long long)max_w);
    if (baseline) {
        std::printf("--- baseline model replay (prefix-max Fenwick, rescan on decrease) ---\n");
        if (stopped) std::printf("STOPPED after command %zu: budget of %lld rescan reads reached\n", stopped_at, (long long)budget);
        std::printf("cells rebuilt = %llu, elements rescanned = %llu\n", (unsigned long long)sim.stale_rebuilds,
                    (unsigned long long)sim.rescan_reads);
        if (replayed_dec) std::printf("avg cells rebuilt per decrease = %.1f   (ordinary Fenwick update touches about %d cells)\n",
                             static_cast<double>(sim.stale_rebuilds) / static_cast<double>(replayed_dec), ilog2(static_cast<uint64_t>(n)) + 1);
    }
    return 0;
}
