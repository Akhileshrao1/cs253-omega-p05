// adversary/gen_b_queries.cpp
// Query workloads for Structure B, plus controls for the Analyst.
//   pairs   replay the worst (l, r) pairs found by find_worst_queries   <-- the adversarial mode
//   width   random ranges of exactly --width elements (range-width sweeps, experiment E1)
//   random  uniform random ranges (control)
// --ufrac = % of operations that are random point updates (R, or C with --cfrac %),
// for the update-mix sweeps (E2).
//
// Usage: gen_b_queries --n 1000000 --ops 1000000 --seed 1 --mode pairs --pairs worst_pairs.txt
//                      [--ufrac 0] [--cfrac 0] [--vmax 1000000] [--width W] [--calib multiples]
//                      [--out file] [--log file]
#include "common.hpp"

#include <algorithm>
#include <string>

using namespace adv;

int main(int argc, char** argv) {
    Args args(argc, argv);
    int64_t n = args.num("n", 1000000);
    int64_t ops = args.num("ops", 1000000);
    uint64_t seed = static_cast<uint64_t>(args.num("seed", 1));
    std::string mode = args.str("mode", "random");
    std::string pairs_file = args.str("pairs", "");
    int64_t ufrac = args.num("ufrac", 0);
    int64_t cfrac = args.num("cfrac", 0);
    int64_t vmax = args.num("vmax", 1000000);
    int64_t width = args.num("width", n / 2);
    std::string calib_mode = args.str("calib", "multiples");
    std::string out = args.str("out", "-");
    std::string log = args.str("log", "");
    args.finish();

    if (n < 2) die("--n must be >= 2");
    if (ops < 1) die("--ops must be >= 1");
    if (ufrac < 0 || ufrac > 100 || cfrac < 0 || cfrac > 100) die("--ufrac and --cfrac must be 0..100");
    if (vmax < 0 || vmax > VALUE_MAX) die("--vmax must be 0..1e9");
    if (mode != "pairs" && mode != "width" && mode != "random") die("unknown --mode " + mode);
    if (mode == "width" && (width < 1 || width > n)) die("--width must be in 1..n");

    std::vector<std::pair<int64_t, int64_t>> pairs;
    if (mode == "pairs") {
        if (pairs_file.empty()) die("--mode pairs needs --pairs <file>");
        FILE* f = std::fopen(pairs_file.c_str(), "r");
        if (!f) die("cannot open " + pairs_file);
        char line[256];
        uint64_t skipped = 0;
        while (std::fgets(line, sizeof line, f)) {
            if (line[0] == '#') continue;
            long long l, r;
            if (std::sscanf(line, "%lld %lld", &l, &r) == 2) {
                if (l >= 1 && l <= r && r <= n) pairs.emplace_back(l, r);
                else ++skipped;
            }
        }
        std::fclose(f);
        if (pairs.empty()) die("no usable pairs for n=" + std::to_string(n) + " in " + pairs_file +
                               " (worst pairs depend on n: rerun find_worst_queries with the same --n)");
        if (skipped) std::fprintf(stderr, "warning: skipped %llu pairs outside 1..n\n", (unsigned long long)skipped);
    }

    Rng rng(seed);
    std::vector<int64_t> readings(n);
    for (auto& x : readings) x = rng.range(0, vmax);
    std::vector<int64_t> calib = make_calibration(calib_mode, n, rng);

    std::vector<Op> list;
    list.reserve(ops);
    uint64_t updates = 0, cupdates = 0, queries = 0;
    for (int64_t k = 0; k < ops; ++k) {
        if (ufrac > 0 && static_cast<int64_t>(rng.below(100)) < ufrac) {
            if (cfrac > 0 && static_cast<int64_t>(rng.below(100)) < cfrac) {
                list.push_back({'C', rng.range(1, n), 720720LL * rng.range(1, 1387)});
                ++cupdates;
            } else {
                list.push_back({'R', rng.range(1, n), rng.range(0, vmax)});
                ++updates;
            }
            continue;
        }
        int64_t l, r;
        if (mode == "pairs") {
            auto p = pairs[rng.below(pairs.size())];
            l = p.first; r = p.second;
        } else if (mode == "width") {
            l = rng.range(1, n - width + 1);
            r = l + width - 1;
        } else {
            l = rng.range(1, n); r = rng.range(1, n);
            if (l > r) std::swap(l, r);
        }
        list.push_back({'Q', l, r});
        ++queries;
    }

    Writer w(out);
    w.instance(readings, calib, list);
    write_meta(out, {"generator : gen_b_queries (target: Structure B query steps / controls)",
                     "n=" + std::to_string(n) + " ops=" + std::to_string(ops) + " seed=" + std::to_string(seed) +
                         " mode=" + mode + " ufrac=" + std::to_string(ufrac) + " cfrac=" + std::to_string(cfrac) +
                         " vmax=" + std::to_string(vmax) + (mode == "width" ? " width=" + std::to_string(width) : "") +
                         (mode == "pairs" ? " pairs=" + pairs_file : "") + " calib=" + calib_mode,
                     "queries=" + std::to_string(queries) + " reading-updates=" + std::to_string(updates) +
                         " calibration-updates=" + std::to_string(cupdates)});
    append_log(log, "gen_b_queries," + std::to_string(n) + "," + std::to_string(ops) + "," + std::to_string(seed) +
                        ",mode=" + mode + ";ufrac=" + std::to_string(ufrac) + ";cfrac=" + std::to_string(cfrac) +
                        ";vmax=" + std::to_string(vmax) + ";width=" + std::to_string(width) + "," + out);
    return 0;
}
