// adversary/find_worst_queries.cpp
// Finds the (l, r) that make a structure take the MOST steps for query_max,
// WITHOUT needing to know its internal loop: it runs queries and reads the
// structure's own OpCounters.
//   exhaustive : every (l, r)   -> the TRUE worst case, use for n <= 8192
//   hill       : random restarts + hill climbing over bit flips of l-1 and r -> large n
// Output lines "l r steps" feed gen_b_queries --mode pairs.
//
// Usage: find_worst_queries --n 1024 [--mode exhaustive|hill] [--metric reads|combines|writes]
//                           [--seed 1] [--restarts 200] [--iters 2000] [--topk 1000] [--out worst_pairs.txt]
#include "common.hpp"
#include "target.hpp"

#include <algorithm>
#include <map>
#include <string>

using namespace adv;

static uint64_t metric_value(const OpCounters& c, const std::string& m) {
    if (m == "reads") return c.node_reads;
    if (m == "combines") return c.combines;
    if (m == "writes") return c.node_writes;
    die("unknown --metric " + m + " (reads, combines, writes)");
}

int main(int argc, char** argv) {
    Args args(argc, argv);
    int64_t n = args.num("n", 1024);
    std::string mode = args.str("mode", n <= 4096 ? "exhaustive" : "hill");
    uint64_t seed = static_cast<uint64_t>(args.num("seed", 1));
    int64_t restarts = args.num("restarts", 200);
    int64_t iters = args.num("iters", 2000);
    int64_t topk = args.num("topk", 1000);
    std::string metric = args.str("metric", "reads");
    std::string out = args.str("out", "worst_pairs.txt");
    args.finish();

    if (n < 2) die("--n must be >= 2");
    if (mode != "exhaustive" && mode != "hill") die("--mode must be exhaustive or hill");
    if (mode == "exhaustive" && n > 8192) die("exhaustive is too slow above n=8192; use --mode hill");

    Rng rng(seed);
    std::vector<Value> arr(n);
    for (auto& x : arr) x = rng.range(0, 1000000);
    auto s = make_range_max();
    s->build(arr);

    auto measure = [&](int64_t l, int64_t r) -> uint64_t {
        s->reset_counters();
        (void)s->query_max(static_cast<Index>(l), static_cast<Index>(r));
        return metric_value(s->counters(), metric);
    };

    std::vector<std::pair<std::pair<int64_t, int64_t>, uint64_t>> result;
    uint64_t best = 0;
    double mean = 0;
    std::map<uint64_t, uint64_t> hist;

    if (mode == "exhaustive") {
        uint64_t total = 0, count = 0;
        for (int64_t l = 1; l <= n; ++l)
            for (int64_t r = l; r <= n; ++r) {
                uint64_t v = measure(l, r);
                best = std::max(best, v);
                total += v; ++count; ++hist[v];
            }
        mean = static_cast<double>(total) / static_cast<double>(count);
        uint64_t seen = 0;  // reservoir-sample up to topk pairs attaining the maximum
        for (int64_t l = 1; l <= n; ++l)
            for (int64_t r = l; r <= n; ++r)
                if (measure(l, r) == best) {
                    ++seen;
                    if (static_cast<int64_t>(result.size()) < topk) result.push_back({{l, r}, best});
                    else {
                        uint64_t j = rng.below(seen);
                        if (j < static_cast<uint64_t>(topk)) result[j] = {{l, r}, best};
                    }
                }
        std::printf("pairs attaining the maximum: %llu of %llu\n", (unsigned long long)seen, (unsigned long long)count);
    } else {
        uint64_t total = 0;
        const int samples = 20000;
        for (int i = 0; i < samples; ++i) {
            int64_t l = rng.range(1, n), r = rng.range(1, n);
            if (l > r) std::swap(l, r);
            total += measure(l, r);
        }
        mean = static_cast<double>(total) / samples;

        const int bits = ilog2(static_cast<uint64_t>(n)) + 1;
        std::map<std::pair<int64_t, int64_t>, uint64_t> found;
        for (int64_t rs = 0; rs < restarts; ++rs) {
            int64_t l = rng.range(1, n), r = rng.range(1, n);
            if (l > r) std::swap(l, r);
            uint64_t cur = measure(l, r);
            for (int64_t it = 0; it < iters; ++it) {
                int64_t nl = l, nr = r;
                int b = static_cast<int>(rng.below(bits));
                switch (rng.below(6)) {
                    case 0: nl = ((l - 1) ^ (int64_t(1) << b)) + 1; break;
                    case 1: nr = r ^ (int64_t(1) << b); break;
                    case 2: nl = l + 1; break;
                    case 3: nl = l - 1; break;
                    case 4: nr = r + 1; break;
                    default: nr = r - 1; break;
                }
                if (nl < 1 || nr > n || nl > nr) continue;
                uint64_t v = measure(nl, nr);
                if (v >= cur) { l = nl; r = nr; cur = v; }  // plateau moves allowed
                if (v > best) { best = v; found.clear(); }
                if (v == best && found.size() < 200000) found[{nl, nr}] = v;
            }
        }
        std::vector<std::pair<std::pair<int64_t, int64_t>, uint64_t>> all(found.begin(), found.end());
        for (size_t i = all.size(); i > 1; --i) std::swap(all[i - 1], all[rng.below(i)]);
        for (size_t i = 0; i < all.size() && static_cast<int64_t>(i) < topk; ++i) result.push_back(all[i]);
    }

    FILE* f = std::fopen(out.c_str(), "w");
    if (!f) die("cannot open " + out);
    std::fprintf(f, "# find_worst_queries n=%lld mode=%s metric=%s seed=%llu best=%llu\n", (long long)n, mode.c_str(),
                 metric.c_str(), (unsigned long long)seed, (unsigned long long)best);
    for (const auto& p : result)
        std::fprintf(f, "%lld %lld %llu\n", (long long)p.first.first, (long long)p.first.second, (unsigned long long)p.second);
    std::fclose(f);

    std::printf("n=%lld  mode=%s  metric=%s\n", (long long)n, mode.c_str(), metric.c_str());
    std::printf("worst case found : %llu steps   (e.g. l=%lld r=%lld)\n", (unsigned long long)best,
                result.empty() ? 0LL : (long long)result[0].first.first, result.empty() ? 0LL : (long long)result[0].first.second);
    std::printf("mean (random l,r): %.2f steps\n", mean);
    std::printf("log2(n)          : %d   (worst/log2(n) = %.2f)\n", ilog2(static_cast<uint64_t>(n)),
                static_cast<double>(best) / std::max(1, ilog2(static_cast<uint64_t>(n))));
    std::printf("saved %zu pairs to %s\n", result.size(), out.c_str());
    if (mode == "exhaustive") {
        std::printf("histogram (steps: number of ranges):\n");
        for (const auto& h : hist) std::printf("  %llu: %llu\n", (unsigned long long)h.first, (unsigned long long)h.second);
    }
    return 0;
}
