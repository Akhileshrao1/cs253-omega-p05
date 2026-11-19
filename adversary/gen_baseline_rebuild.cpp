// adversary/gen_baseline_rebuild.cpp
// D5(b): decreasing updates that force the prefix-max Fenwick BASELINE into
// repeated, expensive rebuilds.
//
//   readings a[i] = n+1-i                  (strictly decreasing)
//   sweep:  for t = 1..S:  R t 0           (S = min(n, ops))
// When t is lowered, positions before t already hold 0 and positions after t are
// smaller, so a[t] is the MAXIMUM of EVERY block containing t: every cell on t's
// update path is stale. This holds for any rebuild method, so the number of
// rebuilt cells (path length) is implementation-independent. If each rebuilt cell
// rescans its block, one sweep reads  sum_j lowbit(j)^2  elements (= 1.5n^2 - n/2
// for n = 2^m). If ops > n, the sweep is followed by cheap raises and repeats.
// Optional --qfrac: % of operations that are PREFIX queries "Q 1 r" (the baseline
// can only answer prefix maxima).
//
// Usage: gen_baseline_rebuild --n 1000000 --ops 1000000 --seed 1 [--qfrac 0] [--calib multiples]
//                             [--out file] [--log ../results/seeds.csv]
#include "common.hpp"

#include <algorithm>
#include <string>

using namespace adv;

int main(int argc, char** argv) {
    Args args(argc, argv);
    int64_t n = args.num("n", 1000000);
    int64_t ops = args.num("ops", 1000000);
    uint64_t seed = static_cast<uint64_t>(args.num("seed", 1));
    int64_t qfrac = args.num("qfrac", 0);
    std::string calib_mode = args.str("calib", "multiples");
    std::string out = args.str("out", "-");
    std::string log = args.str("log", "");
    args.finish();

    if (n < 1 || n > VALUE_MAX) die("--n must be in 1..1e9");
    if (ops < 1) die("--ops must be >= 1");
    if (qfrac < 0 || qfrac > 100) die("--qfrac must be 0..100");

    Rng rng(seed);
    std::vector<int64_t> readings(n);
    for (int64_t i = 1; i <= n; ++i) readings[i - 1] = n + 1 - i;
    std::vector<int64_t> calib = make_calibration(calib_mode, n, rng);

    const int64_t S = std::min(n, ops);
    std::vector<Op> list;
    list.reserve(ops);
    int64_t pos = 0;
    uint64_t lowers = 0, queries = 0, cells = 0, reads = 0;
    for (int64_t k = 0; k < ops; ++k) {
        if (qfrac > 0 && static_cast<int64_t>(rng.below(100)) < qfrac) {
            list.push_back({'Q', 1, rng.range(1, n)});
            ++queries;
            continue;
        }
        if (pos < S) {
            int64_t t = pos + 1;
            list.push_back({'R', t, 0});
            cells += path_length(n, t);
            reads += rescan_cost_path(n, t);
            ++lowers;
        } else {
            int64_t t = pos - S + 1;
            list.push_back({'R', t, n + 1 - t});
        }
        pos = (pos + 1) % (2 * S);
    }

    Writer w(out);
    w.instance(readings, calib, list);

    write_meta(out, {"generator : gen_baseline_rebuild (target: prefix-max Fenwick baseline)",
                     "n=" + std::to_string(n) + " ops=" + std::to_string(ops) + " seed=" + std::to_string(seed) +
                         " qfrac=" + std::to_string(qfrac) + " calib=" + calib_mode,
                     "lowering updates (decreases) = " + std::to_string(lowers) + ", prefix queries = " + std::to_string(queries),
                     "predicted rebuilt cells (every cell on every lowering path is stale) = " + std::to_string(cells),
                     "predicted elements rescanned IF each rebuild rescans its block        = " + std::to_string(reads)});
    append_log(log, "gen_baseline_rebuild," + std::to_string(n) + "," + std::to_string(ops) + "," + std::to_string(seed) +
                        ",qfrac=" + std::to_string(qfrac) + ";calib=" + calib_mode + "," + out);
    return 0;
}
