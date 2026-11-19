// adversary/run_adversarial.cpp
// Replays a generated input file through the REAL implementations and the oracle:
//   * Structure A (segment tree): query_max_count on readings, query_gcd on calibration
//   * Structure B (Dima-Ceterchi):  query_max on readings
//   * Oracle (plain arrays)         = the correctness reference
// It (1) compares EVERY query answer with the oracle and stops at the first
// mismatch, (2) runs check_invariants periodically, and (3) prints the operation
// counters per structure and per command type (mean / max), which is the evidence
// for "adversarial results" in REPORT.md (experiments E4/E5).
//
// NOTE: the oracle scans every queried range, so use moderate n / ops / widths.
//
// Usage: run_adversarial --in file.txt [--check-every 0]   (0 = about 10 checks per run)
#include "../oracle/oracle.hpp"
#include "../src/structA/segment_tree.hpp"
#include "../src/structA/max_count_monoid.hpp"
#include "../src/structA/gcd_monoid.hpp"
#include "../src/structB/dima_ceterchi_max.hpp"

#include "reader.hpp"

#include <algorithm>
#include <string>

using namespace adv;

struct Stat {
    uint64_t count = 0, reads = 0, writes = 0, combines = 0, recomp = 0, max_reads = 0;
    size_t worst_cmd = 0;
    void add(const OpCounters& c, size_t cmd) {
        ++count;
        reads += c.node_reads; writes += c.node_writes; combines += c.combines; recomp += c.recomputations;
        if (c.node_reads > max_reads) { max_reads = c.node_reads; worst_cmd = cmd; }
    }
    void print(const char* structure, const char* kind) const {
        if (!count) return;
        double d = static_cast<double>(count);
        std::printf("%-22s %-14s %9llu %10.1f %9llu %9.1f %9.1f %9.1f\n", structure, kind, (unsigned long long)count,
                    reads / d, (unsigned long long)max_reads, writes / d, combines / d, recomp / d);
    }
};

int main(int argc, char** argv) {
    Args args(argc, argv);
    std::string in = args.str("in", "");
    int64_t every = args.num("check-every", 0);
    args.finish();
    if (in.empty()) die("usage: run_adversarial --in <file> [--check-every K]");

    Instance inst = read_instance(in);
    const size_t m = inst.ops.size();
    if (every <= 0) every = static_cast<int64_t>(std::max<size_t>(1, m / 10));

    OracleMaxCount oracle_r;
    OracleGcd oracle_g;
    SegTree<MaxCountMonoid> seg_r;
    SegTree<GcdMonoid> seg_g;
    DimaCeterchiMax dc;
    std::vector<Value> readings(inst.readings.begin(), inst.readings.end());
    std::vector<Value> calib(inst.calib.begin(), inst.calib.end());
    oracle_r.build(readings); oracle_g.build(calib);
    seg_r.build(readings); seg_g.build(calib);
    dc.build(readings);

    Stat a_upd, a_q, g_upd, g_q, b_upd, b_q, b_upd_dec, b_upd_inc;
    uint64_t q_checked = 0;
    std::string why;

    auto fail = [&](size_t k, const std::string& msg) {
        std::printf("MISMATCH at command %zu (%c %lld %lld): %s\n", k + 1, inst.ops[k].type,
                    (long long)inst.ops[k].a, (long long)inst.ops[k].b, msg.c_str());
        std::printf("Keep this file and its .meta: it reproduces the bug. Shrink it before reporting.\n");
        std::exit(1);
    };

    std::vector<Value> shadow = readings;  // to tell decreases from increases
    for (size_t k = 0; k < m; ++k) {
        const Op& op = inst.ops[k];
        if (op.type == 'R') {
            Index i = static_cast<Index>(op.a);
            Value v = op.b;
            bool decrease = v < shadow[i - 1];
            bool increase = v > shadow[i - 1];
            shadow[i - 1] = v;
            oracle_r.assign(i, v);
            seg_r.reset_counters(); seg_r.assign(i, v); a_upd.add(seg_r.counters(), k);
            dc.reset_counters();    dc.assign(i, v);    b_upd.add(dc.counters(), k);
            if (decrease) b_upd_dec.add(dc.counters(), k);
            else if (increase) b_upd_inc.add(dc.counters(), k);
        } else if (op.type == 'C') {
            Index i = static_cast<Index>(op.a);
            oracle_g.assign(i, op.b);
            seg_g.reset_counters(); seg_g.assign(i, op.b); g_upd.add(seg_g.counters(), k);
        } else {
            Index l = static_cast<Index>(op.a), r = static_cast<Index>(op.b);
            MaxCount expect = oracle_r.query_max_count(l, r);
            Value expect_g = oracle_g.query_gcd(l, r);

            seg_r.reset_counters(); MaxCount got_a = seg_r.query_max_count(l, r); a_q.add(seg_r.counters(), k);
            dc.reset_counters();    Value got_b = dc.query_max(l, r);              b_q.add(dc.counters(), k);
            seg_g.reset_counters(); Value got_g = seg_g.query_gcd(l, r);           g_q.add(seg_g.counters(), k);
            ++q_checked;

            if (got_a.max != expect.max || got_a.count != expect.count)
                fail(k, "Structure A (max,count) returned (" + std::to_string(got_a.max) + "," + std::to_string(got_a.count) +
                            "), oracle says (" + std::to_string(expect.max) + "," + std::to_string(expect.count) + ")");
            if (got_b != expect.max)
                fail(k, "Structure B returned " + std::to_string(got_b) + ", oracle says " + std::to_string(expect.max));
            if (got_g != expect_g)
                fail(k, "Structure A gcd returned " + std::to_string(got_g) + ", oracle says " + std::to_string(expect_g));
        }
        if ((static_cast<int64_t>(k) + 1) % every == 0 || k + 1 == m) {
            if (!seg_r.check_invariants(&why)) fail(k, "Structure A (max,count) invariant: " + why);
            if (!seg_g.check_invariants(&why)) fail(k, "Structure A (gcd) invariant: " + why);
            if (!dc.check_invariants(&why)) fail(k, "Structure B invariant: " + why);
        }
    }

    std::printf("PASS  file=%s  n=%zu  commands=%zu  queries compared with the oracle=%llu\n", in.c_str(),
                inst.readings.size(), m, (unsigned long long)q_checked);
    std::printf("invariants checked every %lld commands and at the end.\n\n", (long long)every);
    std::printf("%-22s %-14s %9s %10s %9s %9s %9s %9s\n", "structure", "command", "count", "mean reads", "max reads",
                "mean wr", "mean comb", "mean recomp");
    a_upd.print("A segment tree", "R update");
    a_q.print("A segment tree", "Q max_count");
    g_upd.print("A segment tree (gcd)", "C update");
    g_q.print("A segment tree (gcd)", "Q gcd");
    b_upd.print("B Dima-Ceterchi", "R update");
    b_upd_dec.print("B Dima-Ceterchi", "R decrease");
    b_upd_inc.print("B Dima-Ceterchi", "R increase");
    b_q.print("B Dima-Ceterchi", "Q max");
    if (b_upd_dec.count && b_upd_inc.count) {
        double dec_mean = static_cast<double>(b_upd_dec.reads) / b_upd_dec.count;
        double inc_mean = static_cast<double>(b_upd_inc.reads) / b_upd_inc.count;
        std::printf("\nB: mean reads per decrease / per increase = %.3f  (1.000 means decrease costs identical to increase)\n",
                    dec_mean / inc_mean);
    } else if (b_upd_dec.count) {
        std::printf("\nB: all updates were decreases (mean reads = %.1f)\n",
                    static_cast<double>(b_upd_dec.reads) / b_upd_dec.count);
    }
    return 0;
}
