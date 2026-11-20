// adversary/find_worst_updates.cpp
// MEASURES what an update costs at every position, using the structure's own
// counters, and answers two questions for the report:
//   1. Does a DECREASE cost more than an INCREASE?   (value dependence)
//   2. Which positions are the most expensive?        (position dependence)
// Output lines "position cost" (most expensive first) feed gen_b_updates --positions.
//
// Usage: find_worst_updates --n 1000000 [--metric reads|writes|combines|recomp|total]
//                           [--seed 1] [--topk 64] [--out worst_positions.txt]
#include "common.hpp"
#include "target.hpp"

#include <algorithm>
#include <numeric>
#include <string>

using namespace adv;

static uint64_t metric_value(const OpCounters& c, const std::string& m) {
    if (m == "reads") return c.node_reads;
    if (m == "writes") return c.node_writes;
    if (m == "combines") return c.combines;
    if (m == "recomp") return c.recomputations;
    if (m == "total") return c.node_reads + c.node_writes + c.combines;
    die("unknown --metric " + m + " (reads, writes, combines, recomp, total)");
}

int main(int argc, char** argv) {
    Args args(argc, argv);
    int64_t n = args.num("n", 1024);
    uint64_t seed = static_cast<uint64_t>(args.num("seed", 1));
    int64_t topk = args.num("topk", 64);
    std::string metric = args.str("metric", "recomp");
    std::string out = args.str("out", "worst_positions.txt");
    args.finish();
    if (n < 2) die("--n must be >= 2");

    Rng rng(seed);
    std::vector<Value> a(n);
    for (auto& x : a) x = rng.range(1000, 1000000);
    auto s = make_range_max();
    s->build(a);

    std::vector<uint32_t> cost(n + 1, 0);
    bool value_dependent = false;
    uint64_t dec_total = 0, inc_total = 0;
    for (int64_t p = 1; p <= n; ++p) {
        s->reset_counters();
        s->assign(static_cast<Index>(p), 0);  // a DECREASE (old value >= 1000)
        OpCounters d = s->counters();
        s->reset_counters();
        s->assign(static_cast<Index>(p), VALUE_MAX);  // an INCREASE
        OpCounters u = s->counters();
        if (d.node_reads != u.node_reads || d.node_writes != u.node_writes || d.combines != u.combines ||
            d.recomputations != u.recomputations)
            value_dependent = true;
        cost[p] = static_cast<uint32_t>(metric_value(d, metric));
        dec_total += metric_value(d, metric);
        inc_total += metric_value(u, metric);
    }

    std::vector<int64_t> idx(n);
    std::iota(idx.begin(), idx.end(), 1);
    if (topk > n) topk = n;
    std::partial_sort(idx.begin(), idx.begin() + topk, idx.end(), [&](int64_t x, int64_t y) {
        return cost[x] != cost[y] ? cost[x] > cost[y] : x < y;
    });
    uint32_t best = cost[idx[0]], worst = *std::min_element(cost.begin() + 1, cost.end());
    uint64_t within1 = 0;
    for (int64_t p = 1; p <= n; ++p)
        if (cost[p] * 100ULL >= best * 99ULL) ++within1;

    FILE* f = std::fopen(out.c_str(), "w");
    if (!f) die("cannot open " + out);
    std::fprintf(f, "# find_worst_updates n=%lld metric=%s seed=%llu max=%u\n", (long long)n, metric.c_str(),
                 (unsigned long long)seed, best);
    for (int64_t i = 0; i < topk; ++i) std::fprintf(f, "%lld %u\n", (long long)idx[i], cost[idx[i]]);
    std::fclose(f);

    std::printf("n=%lld  metric=%s\n", (long long)n, metric.c_str());
    std::printf("most expensive update : %u at position %lld\n", best, (long long)idx[0]);
    std::printf("cheapest update       : %u\n", worst);
    std::printf("mean (decreases)      : %.2f   mean (increases): %.2f\n", (double)dec_total / n, (double)inc_total / n);
    std::printf("positions within 1%% of the maximum: %llu of %lld\n", (unsigned long long)within1, (long long)n);
    std::printf("cost depends on the VALUE (decrease != increase)? %s\n", value_dependent ? "YES" : "no");
    std::printf("saved top %lld positions to %s\n", (long long)topk, out.c_str());
    return 0;
}
