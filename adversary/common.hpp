// adversary/common.hpp
// Shared helpers for every Adversary tool: option parsing, a reproducible RNG,
// the input-file writer (format = exactly what app/sensor_demo.cpp reads), and
// the Fenwick cost formulas used in D5.
//
// Author: Koushik Reddy K (Adversary, D5)
#pragma once

#include <cerrno>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <set>
#include <string>
#include <utility>
#include <vector>

namespace adv {

// Calibration values must be 0..1e9 (GcdMonoid / INTERFACE.md).
constexpr int64_t VALUE_MAX = 1000000000LL;

[[noreturn]] inline void die(const std::string& msg) {
    std::fprintf(stderr, "error: %s\n", msg.c_str());
    std::exit(2);
}

// ---------------------------------------------------------------- options
// --key value.  Unknown options are rejected so a typo cannot silently change an experiment.
class Args {
public:
    Args(int argc, char** argv) {
        for (int i = 1; i < argc; ++i) {
            std::string k = argv[i];
            if (k.size() < 3 || k[0] != '-' || k[1] != '-')
                die("unexpected argument '" + k + "' (options look like --n 1000)");
            if (i + 1 >= argc) die("option " + k + " needs a value");
            kv_.emplace_back(k.substr(2), argv[++i]);
        }
    }
    std::string str(const std::string& key, const std::string& def) const {
        used_.insert(key);
        for (auto it = kv_.rbegin(); it != kv_.rend(); ++it)
            if (it->first == key) return it->second;
        return def;
    }
    int64_t num(const std::string& key, int64_t def) const {
        std::string s = str(key, "");
        if (s.empty()) return def;
        char* end = nullptr;
        errno = 0;
        long long v = std::strtoll(s.c_str(), &end, 10);
        if (errno != 0 || *end != '\0') die("option --" + key + " needs an integer, got '" + s + "'");
        return v;
    }
    void finish() const {  // call after reading every option you support
        for (const auto& p : kv_)
            if (!used_.count(p.first)) die("unknown option --" + p.first);
    }

private:
    std::vector<std::pair<std::string, std::string>> kv_;
    mutable std::set<std::string> used_;
};

// -------------------------------------------------------------------- RNG
// SplitMix64. Written by hand because std::uniform_int_distribution is NOT
// guaranteed to give the same numbers on different compilers, and every seed
// must be reproducible (handout, section 6).
class Rng {
public:
    explicit Rng(uint64_t seed) : s_(seed) {}
    uint64_t next() {
        uint64_t z = (s_ += 0x9E3779B97F4A7C15ULL);
        z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
        z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
        return z ^ (z >> 31);
    }
    uint64_t below(uint64_t m) { return next() % m; }  // 0..m-1, m > 0
    int64_t range(int64_t lo, int64_t hi) {            // lo..hi inclusive
        return lo + static_cast<int64_t>(below(static_cast<uint64_t>(hi - lo + 1)));
    }

private:
    uint64_t s_;
};

// ------------------------------------------------------------ file format
// IDENTICAL to what app/sensor_demo.cpp reads from standard input:
//   n
//   n initial readings
//   n initial calibration values
//   m
//   m commands:   R i x   set reading i to x
//                 C i x   set calibration value i to x
//                 Q l r   print "max count gcd" for positions l..r
// Positions are 1-based, ranges closed. There are NO comment lines (the demo
// would reject them); run parameters go into a "<file>.meta" sidecar instead.
struct Op {
    char type;  // 'R', 'C' or 'Q'
    int64_t a, b;
};

class Writer {
public:
    explicit Writer(const std::string& path) {
        if (path.empty() || path == "-") { f_ = stdout; own_ = false; }
        else {
            f_ = std::fopen(path.c_str(), "w");
            if (!f_) die("cannot open output file " + path);
            own_ = true;
        }
        std::setvbuf(f_, nullptr, _IOFBF, 1 << 16);
    }
    ~Writer() { if (own_) std::fclose(f_); else std::fflush(f_); }

    void instance(const std::vector<int64_t>& readings, const std::vector<int64_t>& calib,
                  const std::vector<Op>& ops) {
        std::fprintf(f_, "%zu\n", readings.size());
        line(readings);
        line(calib);
        std::fprintf(f_, "%zu\n", ops.size());
        for (const Op& op : ops)
            std::fprintf(f_, "%c %lld %lld\n", op.type, static_cast<long long>(op.a), static_cast<long long>(op.b));
    }

private:
    void line(const std::vector<int64_t>& v) {
        for (size_t i = 0; i < v.size(); ++i) {
            if (i) std::fprintf(f_, " %lld", static_cast<long long>(v[i]));
            else std::fprintf(f_, "%lld", static_cast<long long>(v[i]));
        }
        std::fprintf(f_, "\n");
    }
    FILE* f_;
    bool own_;
};

// Writes "<out>.meta" (or stderr when writing to stdout): parameters + predictions.
inline void write_meta(const std::string& out, const std::vector<std::string>& lines) {
    FILE* f = stderr;
    bool own = false;
    if (!out.empty() && out != "-") {
        f = std::fopen((out + ".meta").c_str(), "w");
        if (!f) die("cannot open " + out + ".meta");
        own = true;
    }
    for (const auto& l : lines) std::fprintf(f, "%s\n", l.c_str());
    if (own) std::fclose(f);
}

// Appends one line to a CSV so every seed behind a figure is recorded.
inline void append_log(const std::string& path, const std::string& line) {
    if (path.empty()) return;
    FILE* f = std::fopen(path.c_str(), "a");
    if (!f) die("cannot open log file " + path);
    std::fprintf(f, "%s\n", line.c_str());
    std::fclose(f);
}

// Calibration array. "multiples": 720720*k (gcds are non-trivial); "random": 1..1e9.
inline std::vector<int64_t> make_calibration(const std::string& mode, int64_t n, Rng& rng) {
    std::vector<int64_t> c(n);
    if (mode == "multiples") for (auto& x : c) x = 720720LL * rng.range(1, 1387);  // <= 999,638,640
    else if (mode == "random") for (auto& x : c) x = rng.range(1, VALUE_MAX);
    else die("--calib must be 'multiples' or 'random'");
    return c;
}

// ------------------------------------------------------- Fenwick formulas
inline int64_t lowbit(int64_t i) { return i & -i; }

inline int ilog2(uint64_t x) {  // floor(log2 x), x >= 1
    int k = 0;
    while (x > 1) { x >>= 1; ++k; }
    return k;
}

// Cells whose block contains position t are t, t+lowbit(t), ... (<= n).
inline uint64_t path_length(int64_t n, int64_t t) {
    uint64_t c = 0;
    for (int64_t j = t; j <= n; j += lowbit(j)) ++c;
    return c;
}
// Elements read if every cell on that path is rebuilt by rescanning its block.
inline uint64_t rescan_cost_path(int64_t n, int64_t t) {
    uint64_t c = 0;
    for (int64_t j = t; j <= n; j += lowbit(j)) c += static_cast<uint64_t>(lowbit(j));
    return c;
}

}  // namespace adv
