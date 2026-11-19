// adversary/gen_b_updates.cpp
// Decreasing-update stream aimed at Structure B (and, at the same time, at the
// prefix-max baseline).
//
// WHAT IS KNOWN ABOUT B (measured with find_worst_updates on the real code):
// its update cost depends only on the POSITION, never on the values, so the
// only lever against B is to hit the most expensive positions. Those positions
// come from --positions (output of find_worst_updates). The stream also makes
// every lowering hit the GLOBAL maximum, which is what hurts the baseline:
//   * hot positions start with huge distinct values BIG-0 > BIG-1 > ...
//   * round: lower hot[0], hot[1], ... (each is the global max when lowered),
//     then raise them back (cheap increases) and repeat.
//
// Usage: gen_b_updates --n 1000000 --ops 1000000 --seed 1 --positions worst_positions.txt
//                      [--hot 16] [--qfrac 0] [--calib multiples] [--out file] [--log file]
#include "common.hpp"

#include <algorithm>
#include <string>

using namespace adv;

int main(int argc, char** argv) {
    Args args(argc, argv);
    int64_t n = args.num("n", 1000000);
    int64_t ops = args.num("ops", 1000000);
    uint64_t seed = static_cast<uint64_t>(args.num("seed", 1));
    int64_t H = args.num("hot", 16);
    int64_t qfrac = args.num("qfrac", 0);
    std::string pos_file = args.str("positions", "");
    std::string calib_mode = args.str("calib", "multiples");
    std::string out = args.str("out", "-");
    std::string log = args.str("log", "");
    args.finish();

    if (n < 2 || n > VALUE_MAX) die("--n must be in 2..1e9");
    if (ops < 1) die("--ops must be >= 1");
    if (H < 1 || H > n) die("--hot must be in 1..n");
    if (qfrac < 0 || qfrac > 100) die("--qfrac must be 0..100");

    std::vector<int64_t> hot;
    std::string source;
    if (!pos_file.empty()) {
        FILE* f = std::fopen(pos_file.c_str(), "r");
        if (!f) die("cannot open " + pos_file);
        char line[256];
        while (std::fgets(line, sizeof line, f) && static_cast<int64_t>(hot.size()) < H) {
            if (line[0] == '#') continue;
            long long p, c;
            if (std::sscanf(line, "%lld %lld", &p, &c) >= 1 && p >= 1 && p <= n) hot.push_back(p);
        }
        std::fclose(f);
        if (hot.empty()) die("no usable positions for n=" + std::to_string(n) + " in " + pos_file);
        source = "measured positions from " + pos_file;
    } else {
        for (int64_t p = 1; p <= H; ++p) hot.push_back(p);
        source = "UNMEASURED default positions 1..H (run find_worst_updates and pass --positions)";
        std::fprintf(stderr, "warning: %s\n", source.c_str());
    }
    H = static_cast<int64_t>(hot.size());

    Rng rng(seed);
    std::vector<int64_t> readings(n);
    for (auto& x : readings) x = rng.range(0, 1000);
    for (int64_t r = 0; r < H; ++r) readings[hot[r] - 1] = VALUE_MAX - r;
    std::vector<int64_t> calib = make_calibration(calib_mode, n, rng);

    std::vector<Op> list;
    list.reserve(ops);
    int64_t pos = 0;
    uint64_t lowers = 0, queries = 0, cells = 0;
    for (int64_t k = 0; k < ops; ++k) {
        if (qfrac > 0 && static_cast<int64_t>(rng.below(100)) < qfrac) {
            int64_t l = rng.range(1, n), r = rng.range(1, n);
            if (l > r) std::swap(l, r);
            list.push_back({'Q', l, r});
            ++queries;
            continue;
        }
        if (pos < H) {
            list.push_back({'R', hot[pos], rng.range(0, 1000)});
            cells += path_length(n, hot[pos]);
            ++lowers;
        } else {
            int64_t r = pos - H;
            list.push_back({'R', hot[r], VALUE_MAX - r});
        }
        pos = (pos + 1) % (2 * H);
    }

    Writer w(out);
    w.instance(readings, calib, list);

    std::string hs;
    for (int64_t r = 0; r < H && r < 8; ++r) hs += std::to_string(hot[r]) + " ";
    write_meta(out, {"generator : gen_b_updates (target: Structure B updates + prefix baseline)",
                     "n=" + std::to_string(n) + " ops=" + std::to_string(ops) + " seed=" + std::to_string(seed) +
                         " hot=" + std::to_string(H) + " qfrac=" + std::to_string(qfrac) + " calib=" + calib_mode,
                     "hot positions: " + source + "; first: " + hs,
                     "lowering updates (decreases) = " + std::to_string(lowers) + ", queries = " + std::to_string(queries),
                     "baseline: rebuilt cells if every lowering is stale (it is, by construction) = " + std::to_string(cells)});
    append_log(log, "gen_b_updates," + std::to_string(n) + "," + std::to_string(ops) + "," + std::to_string(seed) +
                        ",hot=" + std::to_string(H) + ";qfrac=" + std::to_string(qfrac) + ";calib=" + calib_mode + "," + out);
    return 0;
}
