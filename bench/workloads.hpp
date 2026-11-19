#pragma once

#include "../adversary/common.hpp"
#include "../include/range_structure.hpp"

#include <algorithm>
#include <numeric>

namespace bench {

enum class Query { Max, Prefix, Count, Gcd };
enum class Pattern { Uniform, HotMax, HotControl, HardQuery, Bounded, Ties, SmallGcd, LargeGcd, Fibonacci };

struct Config {
    string id;
    Query query;
    Index n, operations, width;
    unsigned updates = 0, decreases = 50;
    Pattern pattern = Pattern::Uniform;
    Value value_limit = 0;
};

struct Operation {
    Index l, r;
    Value value = 0;
    char kind = 'Q';  // Q = query, U = assign, R = get (correctness only)
};

struct Trace {
    vector<Value> initial;
    vector<Operation> operations;
};

struct Targets {
    Index right = 1;
    uint64_t query_reads = 0;
    vector<Index> positions;
};

inline string query_name(Query query) {
    switch (query) {
    case Query::Max: return "max";
    case Query::Prefix: return "prefix";
    case Query::Count: return "max_count";
    case Query::Gcd: return "gcd";
    }
    throw logic_error("Unknown query");
}

inline vector<Config> experiments(bool quick) {
    const Index n = quick ? 4096 : 1000000;
    const Index ops = quick ? 20000 : 1000000;
    const Index prefix_n = quick ? 512 : 32768;
    const Index prefix_ops = quick ? 512 : 4096;
    vector<Config> configs;
    for (Index w : {Index{1}, Index{8}, Index{64}, n / 16, n / 2, n})
        configs.push_back({"E1-width-" + to_string(w), Query::Max, n, ops, w});
    for (unsigned u : {0, 10, 50, 90})
        configs.push_back({"E2-mix-" + to_string(u), Query::Max, n, ops, 64, u});
    configs.push_back({"E3-increase", Query::Max, n, ops, 0, 100, 0});
    configs.push_back({"E4-decrease", Query::Max, n, ops, 0, 100, 100});
    configs.push_back({"E5-hard-query", Query::Max, n, ops, 0, 0, 0, Pattern::HardQuery});
    configs.push_back({"E5-hot-assign", Query::Max, n, ops, 0, 100, 50, Pattern::HotMax});
    for (Index w : {Index{1}, Index{64}, n / 16, n})
        configs.push_back({"E6-prefix-" + to_string(w), Query::Prefix, n, ops, w});
    for (unsigned u : {10, 50, 90})
        configs.push_back({"E6-prefix-mix-" + to_string(u), Query::Prefix, n, ops, 64, u});
    configs.push_back({"E6-rescan", Query::Prefix, prefix_n, prefix_ops, prefix_n, 50, 50, Pattern::HotMax});
    configs.push_back({"E6-control", Query::Prefix, prefix_n, prefix_ops, prefix_n, 50, 0, Pattern::HotControl});
    configs.push_back({"E7-ties", Query::Count, n, ops, 64, 20, 50, Pattern::Ties});
    configs.push_back({"E7-ties-narrow", Query::Count, n, ops, 8, 20, 50, Pattern::Ties});
    configs.push_back({"E8-small", Query::Gcd, n, ops, 64, 0, 0, Pattern::SmallGcd, 120});
    configs.push_back({"E8-large", Query::Gcd, n, ops, 64, 0, 0, Pattern::LargeGcd, 1000000000});
    configs.push_back({"E8-fibonacci", Query::Gcd, n, ops, 2, 0, 0, Pattern::Fibonacci, 701408733});
    configs.push_back({"E8-mixed", Query::Gcd, n, ops, 64, 50, 50, Pattern::LargeGcd, 1000000000});
    for (unsigned d : {25, 50, 75})
        configs.push_back({"E9-decrease-share-" + to_string(d), Query::Max, n, ops, 0, 100, d});
    const vector<Index> sizes = quick ? vector<Index>{128, 512, n} : vector<Index>{1024, 16384, 262144, n};
    for (Index size : sizes) {
        const Index scale_ops = size == n ? ops : ops / 5;
        configs.push_back({"E10-scale-query-" + to_string(size), Query::Max, size, scale_ops, size / 2});
        configs.push_back({"E10-scale-assign-" + to_string(size), Query::Max, size, scale_ops, 0, 100});
    }
    return configs;
}

inline Value generated_value(Pattern pattern, adv::Rng& rng, Index i) {
    switch (pattern) {
    case Pattern::Ties: return rng.range(-3, 3);
    case Pattern::SmallGcd: return 12 * rng.range(0, 10);
    case Pattern::LargeGcd: return i % 17 == 0 ? 0 : rng.range(1, 1000000000);
    case Pattern::Fibonacci: return i % 2 == 0 ? 701408733 : 433494437;
    default: return rng.range(-1000000, 1000000);
    }
}

inline Trace make_trace(Config cfg, uint64_t seed, const Targets& targets = {}) {
    adv::Rng values(seed), updates(seed ^ 0xe59373db3ULL), locations(seed ^ 0x4dc089deULL);
    Trace trace;
    trace.initial.resize(cfg.n);
    for (Index i = 0; i < cfg.n; ++i)
        trace.initial[i] = generated_value(cfg.pattern, values, i);

    const bool hot = cfg.pattern == Pattern::HotMax || cfg.pattern == Pattern::HotControl;
    const vector<Index> hot_positions = cfg.query == Query::Prefix ? vector<Index>{1} : targets.positions;
    constexpr Value high = 2000000;
    if (hot) {
        for (Index i = 0; i < hot_positions.size(); ++i)
            trace.initial[hot_positions[i] - 1] = high - static_cast<Value>(i);
    }
    vector<Value> current = trace.initial;
    trace.operations.reserve(cfg.operations);
    Index remaining_updates = cfg.operations * cfg.updates / 100;
    Index assigned = 0;
    for (Index k = 0; k < cfg.operations; ++k) {
        // Interleave exactly the requested update count. Prefix stress alternates
        // each write with a query so stale maxima are visible immediately.
        const bool update = hot && cfg.query == Query::Prefix
            ? k % 2 == 0 : updates.below(cfg.operations - k) < remaining_updates;
        if (update) {
            --remaining_updates;
            Index pos = locations.below(cfg.n) + 1;
            const Value delta = values.range(1, 127);
            Value v = current[pos - 1] + delta;
            if (assigned % 100 < cfg.decreases) v -= 2 * delta;
            if (hot) {
                const Index rank = assigned % hot_positions.size();
                pos = hot_positions[rank];
                const bool lowering = (assigned / hot_positions.size()) % 2 == 0;
                v = lowering ? -high : high - static_cast<Value>(rank);
                if (cfg.pattern == Pattern::HotControl) v = high + static_cast<Value>(assigned) + 1;
            } else if (cfg.query == Query::Gcd || cfg.pattern == Pattern::Ties) {
                v = generated_value(cfg.pattern, values, assigned);
            }
            current[pos - 1] = v;
            trace.operations.push_back({pos, 0, v, 'U'});
            ++assigned;
            continue;
        }

        Index w = cfg.width;
        if (cfg.pattern == Pattern::Bounded) w = locations.below(cfg.width + 1);
        const Index l = cfg.query == Query::Prefix || cfg.pattern == Pattern::HardQuery
            ? 1 : locations.below(cfg.n - w + 1) + 1;
        trace.operations.push_back({l, l + w - 1});
    }
    return trace;
}

inline Index matched_length(const Trace& trace, uint64_t scan_budget) {
    uint64_t scans = 0;
    Index k = 0;
    for (const auto& op : trace.operations) {
        const Index width = op.kind == 'Q' ? op.r + 1 - op.l : 0;
        if (width > scan_budget - scans) break;
        scans += width;
        ++k;
    }
    return k;
}

}  // namespace bench
