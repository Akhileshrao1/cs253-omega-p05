#include "workloads.hpp"
#include "../oracle/oracle.hpp"
#include "../src/structA/gcd_monoid.hpp"
#include "../src/structA/max_count_monoid.hpp"
#include "../src/structA/segment_tree.hpp"
#include "../src/structB/dima_ceterchi_max.hpp"
#include "../src/structB/prefix_max_fenwick.hpp"

#include <array>
#include <chrono>
#include <deque>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <optional>
#include <type_traits>

namespace bench {

using MaxTree = SegTree<MaxCountMonoid>;
using GcdTree = SegTree<GcdMonoid>;
using Clock = std::chrono::steady_clock;
namespace fs = std::filesystem;

enum class Engine { Seg, Dc, Bit, Array };
constexpr array metrics{&OpCounters::node_reads, &OpCounters::node_writes,
                        &OpCounters::combines, &OpCounters::recomputations, &OpCounters::gcd_steps};
constexpr array metric_names{"node_reads", "node_writes", "combines", "recomputations", "gcd_steps"};
using Histograms = array<map<uint64_t, Index>, metrics.size()>;
using UpdateHistograms = map<char, Histograms>;

const char* label(Engine engine, Query query) {
    switch (engine) {
    case Engine::Seg: return query == Query::Gcd ? "SEG-GCD" : "SEG-MAX-COUNT";
    case Engine::Dc: return "DC-MAX";
    case Engine::Bit: return "BIT-PREFIX-MAX";
    case Engine::Array: return "ARRAY";
    }
    throw logic_error("Unknown engine");
}

template <class F>
auto with_engine(Engine engine, Query query, F&& run) {
    if (query == Query::Gcd) {
        if (engine == Engine::Array) return run(OracleGcd{});
        return run(GcdTree{});
    }
    switch (engine) {
    case Engine::Seg: return run(MaxTree{});
    case Engine::Dc: return run(DimaCeterchiMax{});
    case Engine::Bit: return run(PrefixMaxFenwick{});
    case Engine::Array:
        if (query == Query::Prefix) return run(OraclePrefixMax{});
        return run(OracleMaxCount{});
    }
    throw logic_error("Unknown engine");
}

template <class S>
MaxCount query_answer(const S& structure, Query q, const Operation& op) {
    if constexpr (is_base_of_v<RangeGcd, S>) {
        return {structure.query_gcd(op.l, op.r), 0};
    } else if constexpr (is_base_of_v<PrefixMax, S>) {
        return {structure.query_prefix_max(op.r), 0};
    } else {
        if constexpr (is_base_of_v<RangeMaxCount, S>) {
            if (q == Query::Count) return structure.query_max_count(op.l, op.r);
        }
        return {structure.query_max(op.l, op.r), 0};
    }
}

vector<Engine> engines_for(Query q) {
    if (q == Query::Gcd || q == Query::Count) return {Engine::Seg, Engine::Array};
    if (q == Query::Prefix) return {Engine::Seg, Engine::Dc, Engine::Bit, Engine::Array};
    return {Engine::Seg, Engine::Dc, Engine::Array};
}

vector<MaxCount> expected_answers(Query query, const Trace& trace, Index width, Index matched) {
    vector<MaxCount> answers(trace.operations.size());
    // Long static traces get independent O(n) window/prefix answers. The
    // literal array oracle is still checked on every operation in the matched prefix.
    const bool static_max = (query == Query::Max || query == Query::Prefix) && width > 0 &&
        all_of(trace.operations.begin(), trace.operations.end(), [width](const Operation& op) {
            return op.kind == 'Q' && op.r + 1 - op.l == width;
        });
    if (static_max && matched < trace.operations.size()) {
        deque<Index> window;
        const auto& a = trace.initial;
        vector<Value> maxima(a.size() - width + 1);
        for (Index i = 0; i < a.size(); ++i) {
            if (!window.empty() && window.front() + width <= i) window.pop_front();
            while (!window.empty() && a[window.back()] <= a[i]) window.pop_back();
            window.push_back(i);
            if (i + 1 >= width) maxima[i + 1 - width] = a[window.front()];
        }
        for (Index i = 0; i < answers.size(); ++i)
            answers[i] = {maxima[trace.operations[i].l - 1], 0};
        return answers;
    }
    with_engine(Engine::Array, query, [&](auto oracle) {
        oracle.build(trace.initial);
        for (Index i = 0; i < answers.size(); ++i) {
            const auto& op = trace.operations[i];
            if (op.kind == 'U') {
                answers[i] = {oracle.get(op.l), 0};
                oracle.assign(op.l, op.value);
            } else if (op.kind == 'R') {
                answers[i] = {oracle.get(op.l), 0};
            } else {
                answers[i] = query_answer(oracle, query, op);
            }
        }
        return true;
    });
    return answers;
}

struct Failure {
    Index after;  // Number of operations through the failure (0 means build).
    string reason;
};

void count_update(UpdateHistograms& hist, char direction, const OpCounters& before,
                  const OpCounters& after) {
    auto& group = hist[direction];
    for (Index m = 0; m < metrics.size(); ++m)
        ++group[m][after.*metrics[m] - before.*metrics[m]];
}

optional<Failure> verify(Engine engine, Query query, const Trace& trace,
                         const vector<MaxCount>& expected, Index length, Index interval,
                         UpdateHistograms* hist = nullptr) {
    return with_engine(engine, query, [&](auto structure) -> optional<Failure> {
        Index completed = 0;
        const auto invariant = [&]() {
            string why;
            if (structure.size() != trace.initial.size()) throw logic_error("G1: size changed");
            if (!structure.check_invariants(&why)) throw logic_error(why);
        };
        try {
            structure.build(trace.initial);
            invariant();
            for (Index i = 0; i < length; ++i) {
                ++completed;
                const auto& op = trace.operations[i];
                const auto want = expected[i];
                if (op.kind == 'U') {
                    if (structure.get(op.l) != want.max) throw logic_error("G2: old value differs");
                    const auto before = structure.counters();
                    structure.assign(op.l, op.value);
                    if (hist) {
                        const char direction = op.value < want.max ? 'D' : op.value > want.max ? 'I' : 'S';
                        count_update(*hist, direction, before, structure.counters());
                    }
                    if (structure.get(op.l) != op.value) throw logic_error("G2: assignment did not persist");
                } else {
                    const MaxCount got = op.kind == 'R' ? MaxCount{structure.get(op.l), 0}
                                                        : query_answer(structure, query, op);
                    if (got.max != want.max || got.count != want.count) {
                        throw logic_error("expected (" + to_string(want.max) + "," + to_string(want.count) +
                            "), got (" + to_string(got.max) + "," + to_string(got.count) + ")");
                    }
                }
                if (interval && completed % interval == 0) invariant();
            }
            if (!interval || length % interval != 0) invariant();
        } catch (const exception& error) {
            return Failure{completed, error.what()};
        }
        return nullopt;
    });
}

void save_trace(const fs::path& path, Query q, Index width, Index interval, const Trace& trace) {
    ofstream out(path);
    out.exceptions(ios::failbit | ios::badbit);
    out << static_cast<int>(q) << ' ' << width << ' ' << interval << '\n' << trace.initial.size() << '\n';
    for (Value v : trace.initial) out << v << ' ';
    out << '\n' << trace.operations.size() << '\n';
    for (const auto& op : trace.operations)
        out << op.kind << ' ' << op.l << ' ' << op.r << ' ' << op.value << '\n';
}

void verify_or_save(Engine engine, Query q, const Trace& trace, const vector<MaxCount>& expected,
                    Index length, Index interval, Index width, UpdateHistograms* hist,
                    const fs::path& out, const string& id) {
    const auto failure = verify(engine, q, trace, expected, length, interval, hist);
    if (!failure) return;

    Trace trial{trace.initial, {trace.operations.begin(), trace.operations.begin() + failure->after}};
    const string prefix = id + "-" + label(engine, q);
    save_trace(out / (prefix + ".trace"), q, width, interval, trial);
    // Delta debugging keeps the initial array and preserves the failure message.
    // Bound diagnostic time and retain the original failing prefix regardless.
    const auto fails = [&](const Trace& candidate) {
        const auto reference = expected_answers(q, candidate, width, 0);
        const auto result = verify(engine, q, candidate, reference, candidate.operations.size(), interval);
        return result && result->reason == failure->reason;
    };
    size_t chunks = 2;
    unsigned attempts = 0;
    while (trial.operations.size() > 1 && attempts < 64) {
        const size_t length_now = trial.operations.size();
        const size_t step = (length_now + chunks - 1) / chunks;
        bool reduced = false;
        for (size_t start = 0; start < length_now && attempts < 64; start += step) {
            Trace candidate{trial.initial, trial.operations};
            candidate.operations.erase(candidate.operations.begin() + start,
                candidate.operations.begin() + min(length_now, start + step));
            ++attempts;
            if (fails(candidate)) {
                trial = move(candidate);
                chunks = max<size_t>(2, chunks - 1);
                reduced = true;
                break;
            }
        }
        if (!reduced) {
            if (chunks >= length_now) break;
            chunks = min(length_now, chunks * 2);
        }
    }
    save_trace(out / (prefix + "-reduced.trace"), q, width, interval, trial);
    throw runtime_error(prefix + " after operation " + to_string(failure->after) + ": " + failure->reason);
}

struct Sample {
    int64_t build_ns, elapsed_ns;
    uint64_t checksum;
    OpCounters counters;
};

Sample timed(Engine engine, Query q, const Trace& trace, Index length) {
    return with_engine(engine, q, [&](auto structure) {
        const auto start = Clock::now();
        structure.build(trace.initial);
        const auto built = Clock::now();
        structure.reset_counters();
        const auto replay_start = Clock::now();
        uint64_t checksum = 0;
        for (Index i = 0; i < length; ++i) {
            const auto& op = trace.operations[i];
            if (op.kind == 'U') {
                structure.assign(op.l, op.value);
            } else {
                const auto answer = query_answer(structure, q, op);
                checksum += static_cast<uint64_t>(answer.max) * 31 + answer.count;
            }
        }
        const auto end = Clock::now();
        return Sample{std::chrono::duration_cast<std::chrono::nanoseconds>(built - start).count(),
                      std::chrono::duration_cast<std::chrono::nanoseconds>(end - replay_start).count(),
                      checksum, structure.counters()};
    });
}

struct Mix { Index queries = 0, inc = 0, dec = 0, same = 0; };

Mix mix_of(const Trace& trace, const vector<MaxCount>& expected, Index length) {
    Mix mix;
    for (Index i = 0; i < length; ++i) {
        const auto& op = trace.operations[i];
        if (op.kind == 'Q') ++mix.queries;
        else if (op.value < expected[i].max) ++mix.dec;
        else if (op.value > expected[i].max) ++mix.inc;
        else ++mix.same;
    }
    return mix;
}

Targets probe_targets(Index n, uint64_t seed, ostream& probes) {
    Targets targets;
    DimaCeterchiMax dc;
    Trace trace = make_trace({"probe", Query::Max, n, 0, 1}, seed);
    dc.build(trace.initial);
    // D5's high-step family includes queries anchored at the first position.
    // Scan the entire family and retain the first endpoint attaining the maximum.
    for (Index r = 1; r <= n; ++r) {
        dc.reset_counters();
        dc.query_max(1, r);
        if (dc.counters().node_reads > targets.query_reads) {
            targets.right = r;
            targets.query_reads = dc.counters().node_reads;
        }
    }
    vector<pair<uint64_t, uint64_t>> costs(n + 1);
    for (Index i = 1; i <= n; ++i) {
        const Value old = trace.initial[i - 1];
        dc.reset_counters();
        dc.assign(i, old - 1);
        const OpCounters dec = dc.counters();
        dc.reset_counters();
        dc.assign(i, old);
        const OpCounters inc = dc.counters();
        costs[i] = {dec.recomputations, dec.node_reads};
        if (i == 1 || i == n || i % max<Index>(1, n / 1024 + 1) == 1) {
            for (const auto& [direction, counters] : {pair{'D', dec}, pair{'I', inc}}) {
                probes << n << ',' << seed << ',' << i << ',' << direction;
                for (auto metric : metrics) probes << ',' << counters.*metric;
                probes << '\n';
            }
        }
    }
    targets.positions.resize(n);
    iota(targets.positions.begin(), targets.positions.end(), Index{1});
    const Index hot = min<Index>(16, n);
    partial_sort(targets.positions.begin(), targets.positions.begin() + hot, targets.positions.end(),
        [&](Index a, Index b) { return costs[a] == costs[b] ? a < b : costs[a] > costs[b]; });
    targets.positions.resize(hot);
    return targets;
}

void correctness(bool quick, uint64_t seed, const fs::path& out, ostream& checks) {
    const Index n = quick ? 128 : 1000000;
    const Index operations = quick ? 1000 : 1000000;
    for (const auto q : {Query::Max, Query::Count, Query::Gcd, Query::Prefix}) {
        const Pattern data = q == Query::Gcd ? Pattern::LargeGcd : q == Query::Count ? Pattern::Ties : Pattern::Uniform;
        Trace edges{{-9, -3, -3, -9, -3}, {{1, 5}, {1, 0}, {6, 5}, {1, 0, -11, 'U'},
            {5, 0, 12, 'U'}, {5, 0, 0, 'R'}, {1, 5}, {5, 5}, {5, 0, -12, 'U'}, {1, 5}}};
        if (q == Query::Gcd) {
            edges.initial = {0, 6, 15, 0, 21};
            edges.operations[3].value = 433494437;
            edges.operations[4].value = 701408733;
            edges.operations[8].value = 0;
        }
        if (q == Query::Prefix) {
            edges.operations[2].l = 1;
            edges.operations[7].l = 1;
        }
        auto answers = expected_answers(q, edges, 0, edges.operations.size());
        for (const auto engine : engines_for(q)) {
            verify_or_save(engine, q, edges, answers, edges.operations.size(), 1, 0, nullptr,
                           out, "EDGE-" + query_name(q));
            checks << "edges," << query_name(q) << ',' << label(engine, q) << ",5,"
                   << edges.operations.size() << ',' << seed << ",1,pass\n";
        }
        Trace trace = make_trace({"diff", q, n, operations, 64, 40, 50, data}, seed);
        adv::Rng rng(seed ^ 0xf91d85d4ULL);
        for (Index i = 0; i < operations; ++i) {
            auto& op = trace.operations[i];
            if (i % 29 == 0) op = {i % 2 ? n : Index{1}, 0, 0, 'R'};
            if (op.kind == 'Q') {
                const Index width = rng.below(65);
                if (q == Query::Prefix) op = {1, width};
                else op.r = op.l - 1 + width;
            }
        }
        answers = expected_answers(q, trace, 0, operations);
        const Index interval = quick ? 10 : 1000;
        for (const auto engine : engines_for(q)) {
            if (engine == Engine::Array) continue;
            verify_or_save(engine, q, trace, answers, operations, interval, 0, nullptr,
                           out, "DIFF-" + query_name(q));
            checks << "differential," << query_name(q) << ',' << label(engine, q)
                   << ',' << n << ',' << operations << ',' << seed << ',' << interval << ",pass\n";
        }
        cerr << "Verified differential " << query_name(q) << " (" << operations << " operations)\n";
    }
}

void run_experiment(const Config& cfg, uint64_t seed, const Targets& targets, uint64_t budget, int runs,
                    const fs::path& out, ostream& raw, ostream& work, ostream& checks) {
    const Trace trace = make_trace(cfg, seed, targets);
    const Index matched = matched_length(trace, budget);
    if (matched == 0) throw runtime_error("Scan budget too small for " + cfg.id);
    const auto expected = expected_answers(cfg.query, trace, cfg.width, matched);
    const auto engines = engines_for(cfg.query);
    for (const auto engine : engines) {
        const Index length = engine == Engine::Array ? matched : cfg.operations;
        UpdateHistograms hist;
        const bool profile = cfg.updates == 100 || cfg.pattern == Pattern::HotMax || cfg.pattern == Pattern::HotControl;
        verify_or_save(engine, cfg.query, trace, expected, length, 0, cfg.width, profile ? &hist : nullptr,
                       out, cfg.id);
        checks << cfg.id << ',' << query_name(cfg.query) << ',' << label(engine, cfg.query) << ','
               << cfg.n << ',' << length << ',' << seed << ",0,pass\n";
        for (const auto& [direction, group] : hist) {
            for (Index m = 0; m < metrics.size(); ++m) {
                for (const auto& [value, frequency] : group[m])
                    work << cfg.id << ',' << label(engine, cfg.query) << ',' << direction << ','
                         << metric_names[m] << ',' << value << ',' << frequency << '\n';
            }
        }
    }
    const vector<Index> lengths = matched < cfg.operations ? vector<Index>{cfg.operations, matched}
                                                           : vector<Index>{cfg.operations};
    for (const Index length : lengths) {
        const auto mix = mix_of(trace, expected, length);
        const bool complete = length == cfg.operations;
        auto variants = engines;
        if (complete && matched < length) variants.pop_back();
        uint64_t expected_checksum = 0;
        for (Index i = 0; i < length; ++i) {
            if (trace.operations[i].kind == 'Q')
                expected_checksum += static_cast<uint64_t>(expected[i].max) * 31 + expected[i].count;
        }
        for (int run = 0; run <= runs; ++run) {
            for (Index i = 0; i < variants.size(); ++i) {
                const auto engine = variants[(i + run) % variants.size()];
                const auto sample = timed(engine, cfg.query, trace, length);
                if (sample.checksum != expected_checksum)
                    throw runtime_error("Timed replay checksum mismatch: " + cfg.id);
                if (run == 0) continue;
                raw << cfg.id << ',' << (complete ? "full" : "matched") << ',' << label(engine, cfg.query)
                    << ',' << query_name(cfg.query) << ',' << cfg.n << ',' << length << ',' << cfg.width
                    << ',' << cfg.value_limit << ',' << seed << ',' << run << ',' << mix.queries
                    << ',' << mix.inc << ',' << mix.dec << ',' << mix.same << ',' << sample.build_ns
                    << ',' << sample.elapsed_ns << ',' << sample.checksum;
                for (auto metric : metrics) raw << ',' << sample.counters.*metric;
                raw << '\n';
            }
        }
    }
    cerr << cfg.id << " verified and timed (" << cfg.operations << " full, " << matched << " matched)\n";
}

int replay_trace(const fs::path& path) {
    ifstream in(path);
    unsigned query_id;
    Index width, interval, n, length;
    if (!(in >> query_id >> width >> interval >> n) || query_id > static_cast<unsigned>(Query::Gcd))
        throw invalid_argument("Invalid trace header");
    const auto query = static_cast<Query>(query_id);
    Trace trace;
    trace.initial.resize(n);
    for (auto& v : trace.initial)
        if (!(in >> v)) throw invalid_argument("Invalid initial values");
    if (!(in >> length)) throw invalid_argument("Invalid trace length");
    trace.operations.resize(length);
    for (auto& op : trace.operations) {
        if (!(in >> op.kind >> op.l >> op.r >> op.value) ||
            (op.kind != 'U' && op.kind != 'R' && op.kind != 'Q'))
            throw invalid_argument("Invalid trace operation");
        if (op.l == 0 || (op.kind == 'Q' ? op.r > n || op.l > op.r + 1 : op.l > n))
            throw invalid_argument("Invalid trace endpoint");
    }
    const auto reference = expected_answers(query, trace, width, 0);
    bool passed = true;
    for (const auto engine : engines_for(query)) {
        if (engine == Engine::Array) continue;
        const auto result = verify(engine, query, trace, reference, length, interval);
        if (result) {
            cerr << label(engine, query) << " failed after operation " << result->after << ": " << result->reason << '\n';
            passed = false;
        } else {
            cout << label(engine, query) << " passed " << length << " operations\n";
        }
    }
    return passed ? 0 : 1;
}

}  // namespace bench

int main(int argc, char** argv) {
    using namespace bench;
    try {
        adv::Args args(argc, argv);
        const fs::path out(args.str("out", "build/benchmark"));
        const string profile = args.str("profile", "full");
        const string only = args.str("case", "");
        const string replay = args.str("replay", "");
        const int64_t seed_arg = args.num("seed", 20261119);
        const int64_t runs = args.num("runs", 7);
        args.finish();
        if (profile != "full" && profile != "quick") throw invalid_argument("Use --profile quick or full");
        if (seed_arg < 0 || runs < 5 || runs > 100) throw invalid_argument("Expected nonnegative seed and 5 to 100 runs");
        const auto seed = static_cast<uint64_t>(seed_arg);
        fs::create_directories(out);
        if (!replay.empty()) return replay_trace(replay);
        const bool quick = profile == "quick";
        auto configs = experiments(quick);
        if (!only.empty() && none_of(configs.begin(), configs.end(), [&](const auto& cfg) { return cfg.id == only; }))
            throw invalid_argument("Unknown experiment: " + only);
        ofstream raw(out / "runs.csv"), work(out / "update_work.csv"), probes(out / "index_probe.csv"),
                 checks(out / "checks.csv"), targets_out(out / "targets.csv");
        for (auto* file : {&raw, &work, &probes, &checks, &targets_out})
            file->exceptions(ios::badbit | ios::failbit);
        raw << "case,mode,variant,query,n,operations,width,value_limit,seed,run,queries,increases,decreases,"
               "unchanged,build_ns,elapsed_ns,checksum";
        probes << "n,seed,index,direction";
        for (auto name : metric_names) {
            raw << ',' << name;
            probes << ',' << name;
        }
        raw << '\n';
        probes << '\n';
        work << "case,variant,direction,counter,value,frequency\n";
        checks << "case,query,variant,n,operations,seed,invariant_interval,status\n";
        targets_out << "kind,start,end,seed,node_reads\n";

        if (only.empty()) correctness(quick, seed, out, checks);
        const auto targets = (only.empty() || only.find("E5-") == 0)
            ? probe_targets(configs.front().n, seed, probes) : Targets{};
        if (!targets.positions.empty()) {
            targets_out << "query,1," << targets.right << ',' << seed << ',' << targets.query_reads << '\n';
            for (Index p : targets.positions) targets_out << "hot_position," << p << ',' << p << ',' << seed << ",\n";
        }
        const uint64_t budget = quick ? 2000000 : 100000000;
        for (auto cfg : configs) {
            if (!only.empty() && cfg.id != only) continue;
            if (cfg.pattern == Pattern::HardQuery) cfg.width = targets.right;
            run_experiment(cfg, seed, targets, budget, static_cast<int>(runs), out, raw, work, checks);
        }
    } catch (const exception& error) {
        cerr << "Benchmark failed: " << error.what() << '\n';
        return 1;
    }
}
