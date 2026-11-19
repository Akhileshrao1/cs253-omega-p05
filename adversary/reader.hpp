// adversary/reader.hpp
// Reads and VALIDATES an input file in the sensor_demo format (see common.hpp).
// Any problem stops the program with a message naming the line/operation.
#pragma once

#include "common.hpp"

#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>

namespace adv {

struct Instance {
    std::vector<int64_t> readings, calib;
    std::vector<Op> ops;
};

inline Instance read_instance(const std::string& path) {
    FILE* f = std::fopen(path.c_str(), "r");
    if (!f) die("cannot open " + path);
    Instance in;
    long long n = 0, m = 0;
    if (std::fscanf(f, "%lld", &n) != 1 || n < 1) die("bad n");
    in.readings.resize(n);
    in.calib.resize(n);
    for (long long i = 0; i < n; ++i) {
        long long v;
        if (std::fscanf(f, "%lld", &v) != 1) die("readings line too short");
        if (v == INT64_MIN) die("reading equals INT64_MIN, which is reserved as NEG_INF");
        in.readings[i] = v;
    }
    for (long long i = 0; i < n; ++i) {
        long long v;
        if (std::fscanf(f, "%lld", &v) != 1) die("calibration line too short");
        if (v < 0 || v > VALUE_MAX) die("calibration value out of range 0..1e9 at position " + std::to_string(i + 1));
        in.calib[i] = v;
    }
    if (std::fscanf(f, "%lld", &m) != 1 || m < 0) die("bad command count");
    in.ops.reserve(m);
    for (long long k = 0; k < m; ++k) {
        char t;
        long long x, y;
        std::string at = "command " + std::to_string(k + 1);
        if (std::fscanf(f, " %c %lld %lld", &t, &x, &y) != 3) die(at + " is malformed or missing");
        if (t == 'R' || t == 'C') {
            if (x < 1 || x > n) die(at + ": position out of range");
            if (t == 'C' && (y < 0 || y > VALUE_MAX)) die(at + ": calibration value out of range 0..1e9");
            if (t == 'R' && y == INT64_MIN) die(at + ": INT64_MIN is reserved");
        } else if (t == 'Q') {
            if (x < 1 || y > n || x > y) die(at + ": bad query range");
        } else {
            die(at + ": unknown command '" + std::string(1, t) + "'");
        }
        in.ops.push_back({t, x, y});
    }
    char extra;
    if (std::fscanf(f, " %c", &extra) == 1) std::fprintf(stderr, "warning: extra data after the last command\n");
    std::fclose(f);
    return in;
}

}  // namespace adv
