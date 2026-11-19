#include "differential_test.hpp"

#include <algorithm>
#include <sstream>
#include <stdexcept>

namespace {
Value random_value(TestKind kind, std::mt19937_64& rng) {
    if (kind == TestKind::Gcd) {
        return static_cast<Value>(rng() % 1000000001ULL);
    }
    // Keep away from NEG_INF, which is reserved as the max identity.
    return static_cast<Value>(static_cast<int64_t>(rng() % 2000001ULL) - 1000000LL);
}

Index random_index(Index low, Index high, std::mt19937_64& rng) {
    if (low > high) return low;
    return low + static_cast<Index>(rng() % (high - low + 1));
}

std::string value_text(Value value) {
    return std::to_string(value);
}

std::string max_count_text(const MaxCount& value) {
    return "{max=" + std::to_string(value.max) +
           ", count=" + std::to_string(value.count) + "}";
}

TestFailure make_failure(
    uint64_t index, const std::string& message,
    const std::string& expected, const std::string& actual,
    const std::vector<TestOperation>& operations) {
    TestFailure failure;
    failure.failed = true;
    failure.operation_index = index;
    failure.message = message;
    failure.expected = expected;
    failure.actual = actual;
    failure.sequence.assign(operations.begin(), operations.begin() + index + 1);
    return failure;
}

template <typename Candidate, typename Oracle, typename Compare>
TestFailure run_differential(
    Candidate& candidate, Oracle& oracle,
    const std::vector<Value>& initial,
    const std::vector<TestOperation>& operations,
    Index invariant_stride, Compare compare) {
    candidate.build(initial);
    oracle.build(initial);

    if (candidate.size() != initial.size()) {
        return make_failure(0, "size mismatch after build",
            std::to_string(initial.size()), std::to_string(candidate.size()), operations);
    }

    for (Index i = 0; i < operations.size(); ++i) {
        const TestOperation& op = operations[i];
        try {
            if (op.kind == OpKind::Assign) {
                candidate.assign(op.first, op.value);
                oracle.assign(op.first, op.value);
            } else if (op.kind == OpKind::Read) {
                const Value expected = oracle.get(op.first);
                const Value actual = candidate.get(op.first);
                if (expected != actual) {
                    return make_failure(i, "point read mismatch",
                        value_text(expected), value_text(actual), operations);
                }
            } else {
                const auto mismatch = compare(candidate, oracle, op);
                if (!mismatch.first.empty()) {
                    return make_failure(i, "query mismatch", mismatch.first,
                                        mismatch.second, operations);
                }
            }

            // Full invariant scans are deliberately periodic: checking after
            // every operation on a million-element array would dominate the test.
            if (invariant_stride != 0 &&
                ((i + 1) % invariant_stride == 0 || i + 1 == operations.size())) {
                std::string why;
                if (!candidate.check_invariants(&why)) {
                    return make_failure(i, "candidate invariant failure: " + why,
                                        "all invariants true", why, operations);
                }
                if (!oracle.check_invariants(&why)) {
                    return make_failure(i, "oracle invariant failure: " + why,
                                        "all invariants true", why, operations);
                }
            }
        } catch (const std::exception& error) {
            return make_failure(i, std::string("exception: ") + error.what(),
                                "operation completes", error.what(), operations);
        }
    }
    return {};
}
} // namespace

std::vector<Value> generate_initial_array(
    TestKind kind, Index n, std::mt19937_64& rng) {
    std::vector<Value> values(n);
    for (Value& value : values) value = random_value(kind, rng);
    return values;
}

std::vector<TestOperation> generate_operations(
    TestKind kind, const TestConfig& config, std::mt19937_64& rng) {
    if (config.n == 0 && config.operations != 0)
        throw std::invalid_argument("TestConfig.n must be positive for generated operations");

    std::vector<TestOperation> operations;
    operations.reserve(static_cast<std::size_t>(config.operations));
    const Index width = std::max<Index>(1, config.max_query_width);

    for (uint64_t i = 0; i < config.operations; ++i) {
        TestOperation op;
        const uint64_t choice = rng() % 100;
        if (choice < 25) {
            op.kind = OpKind::Assign;
            op.first = random_index(1, config.n, rng);
            op.value = random_value(kind, rng);
        } else if (choice < 35) {
            op.kind = OpKind::Read;
            op.first = random_index(1, config.n, rng);
        } else {
            op.kind = OpKind::Query;
            if (kind == TestKind::PrefixMax) {
                op.first = random_index(0, config.n, rng);
            } else {
                op.first = random_index(1, config.n, rng);
                const Index max_right = std::min(config.n, op.first + width - 1);
                op.second = random_index(op.first, max_right, rng);
                // Include valid empty ranges in a small fraction of queries.
                if (op.first < config.n && rng() % 20 == 0)
                    op.second = op.first - 1;
            }
        }
        operations.push_back(op);
    }
    return operations;
}

TestFailure differential_test(
    RangeMaxCount& candidate, const std::vector<Value>& initial,
    const std::vector<TestOperation>& operations, Index invariant_stride) {
    OracleMaxCount oracle;
    return run_differential(candidate, oracle, initial, operations, invariant_stride,
        [](RangeMaxCount& lhs, OracleMaxCount& rhs, const TestOperation& op) {
            const MaxCount expected = rhs.query_max_count(op.first, op.second);
            const MaxCount actual = lhs.query_max_count(op.first, op.second);
            if (expected.max == actual.max && expected.count == actual.count)
                return std::pair<std::string, std::string>{};
            return std::make_pair(max_count_text(expected), max_count_text(actual));
        });
}

TestFailure differential_test(
    RangeGcd& candidate, const std::vector<Value>& initial,
    const std::vector<TestOperation>& operations, Index invariant_stride) {
    OracleGcd oracle;
    return run_differential(candidate, oracle, initial, operations, invariant_stride,
        [](RangeGcd& lhs, OracleGcd& rhs, const TestOperation& op) {
            const Value expected = rhs.query_gcd(op.first, op.second);
            const Value actual = lhs.query_gcd(op.first, op.second);
            if (expected == actual) return std::pair<std::string, std::string>{};
            return std::make_pair(value_text(expected), value_text(actual));
        });
}

TestFailure differential_test(
    RangeMax& candidate, const std::vector<Value>& initial,
    const std::vector<TestOperation>& operations, Index invariant_stride) {
    OracleMaxCount oracle;
    oracle.build(initial);
    candidate.build(initial);
    for (Index i = 0; i < operations.size(); ++i) {
        const auto& op = operations[i];
        try {
            if (op.kind == OpKind::Assign) {
                candidate.assign(op.first, op.value);
                oracle.assign(op.first, op.value);
            } else if (op.kind == OpKind::Read) {
                Value e = oracle.get(op.first), a = candidate.get(op.first);
                if (e != a) return make_failure(i, "point read mismatch", value_text(e), value_text(a), operations);
            } else {
                Value e = oracle.query_max(op.first, op.second);
                Value a = candidate.query_max(op.first, op.second);
                if (e != a) return make_failure(i, "maximum query mismatch", value_text(e), value_text(a), operations);
            }
            if (invariant_stride != 0 &&
                ((i + 1) % invariant_stride == 0 || i + 1 == operations.size())) {
                std::string why;
                if (!candidate.check_invariants(&why))
                    return make_failure(i, "candidate invariant failure: " + why, "all invariants true", why, operations);
                if (!oracle.check_invariants(&why))
                    return make_failure(i, "oracle invariant failure: " + why, "all invariants true", why, operations);
            }
        } catch (const std::exception& error) {
            return make_failure(i, std::string("exception: ") + error.what(), "operation completes", error.what(), operations);
        }
    }
    return {};
}

TestFailure differential_test(
    PrefixMax& candidate, const std::vector<Value>& initial,
    const std::vector<TestOperation>& operations, Index invariant_stride) {
    OraclePrefixMax oracle;
    return run_differential(candidate, oracle, initial, operations, invariant_stride,
        [](PrefixMax& lhs, OraclePrefixMax& rhs, const TestOperation& op) {
            const Value expected = rhs.query_prefix_max(op.first);
            const Value actual = lhs.query_prefix_max(op.first);
            if (expected == actual) return std::pair<std::string, std::string>{};
            return std::make_pair(value_text(expected), value_text(actual));
        });
}

std::vector<TestOperation> shrink_failure(
    const std::vector<TestOperation>& sequence,
    const std::function<bool(const std::vector<TestOperation>&)>& still_fails) {
    if (sequence.empty() || !still_fails(sequence)) return sequence;

    std::vector<TestOperation> current = sequence;
    std::size_t granularity = 2;
    while (current.size() >= 2) {
        const std::size_t chunk = (current.size() + granularity - 1) / granularity;
        bool reduced = false;
        for (std::size_t start = 0; start < current.size(); start += chunk) {
            const std::size_t finish = std::min(current.size(), start + chunk);
            std::vector<TestOperation> trial;
            trial.reserve(current.size() - (finish - start));
            trial.insert(trial.end(), current.begin(), current.begin() + start);
            trial.insert(trial.end(), current.begin() + finish, current.end());
            if (!trial.empty() && still_fails(trial)) {
                current.swap(trial);
                granularity = std::max<std::size_t>(2, granularity - 1);
                reduced = true;
                break;
            }
        }
        if (!reduced) {
            if (granularity >= current.size()) break;
            granularity = std::min(current.size(), granularity * 2);
        }
    }
    return current;
}
