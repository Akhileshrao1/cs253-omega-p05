#pragma once

#include "oracle.hpp"

#include <cstddef>
#include <cstdint>
#include <functional>
#include <random>
#include <string>
#include <vector>

enum class TestKind { MaxCount, Gcd, Max, PrefixMax };
enum class OpKind { Assign, Read, Query };

struct TestConfig {
    Index n = 1000;
    uint64_t operations = 10000;
    uint64_t seed = 20261118;
    Index max_query_width = 64;
};

struct TestOperation {
    OpKind kind = OpKind::Query;
    Index first = 1;   // index, or left endpoint, or prefix endpoint
    Index second = 1;  // right endpoint for range queries
    Value value = 0;   // assigned value
};

struct TestFailure {
    bool failed = false;
    uint64_t operation_index = 0;
    std::string message;
    std::string expected;
    std::string actual;
    std::vector<TestOperation> sequence;
};

std::vector<Value> generate_initial_array(
    TestKind kind, Index n, std::mt19937_64& rng);

std::vector<TestOperation> generate_operations(
    TestKind kind, const TestConfig& config, std::mt19937_64& rng);

TestFailure differential_test(
    RangeMaxCount& candidate, const std::vector<Value>& initial,
    const std::vector<TestOperation>& operations, Index invariant_stride = 1000);

TestFailure differential_test(
    RangeGcd& candidate, const std::vector<Value>& initial,
    const std::vector<TestOperation>& operations, Index invariant_stride = 1000);

TestFailure differential_test(
    RangeMax& candidate, const std::vector<Value>& initial,
    const std::vector<TestOperation>& operations, Index invariant_stride = 1000);

TestFailure differential_test(
    PrefixMax& candidate, const std::vector<Value>& initial,
    const std::vector<TestOperation>& operations, Index invariant_stride = 1000);

// Delta-debug the operation sequence. The predicate must rebuild the candidate
// from the same initial array and preserve the original failure class for each
// trial. Build failures carry an empty sequence and are not reduced.
std::vector<TestOperation> shrink_failure(
    const std::vector<TestOperation>& sequence,
    const std::function<bool(const std::vector<TestOperation>&)>& still_fails);
