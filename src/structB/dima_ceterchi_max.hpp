#ifndef DIMA_CETERCHI_MAX_HPP
#define DIMA_CETERCHI_MAX_HPP

#include "range_structure.hpp"

class DimaCeterchiMax final : public RangeMax {
private:
    Index n = 0;

    vector<Value> arr;
    vector<Value> bit1;
    vector<Value> bit2;

    mutable OpCounters ops;

   
    static Index lowbit(Index x) {
        return x & (~x + 1);
    }


    static Index parent1(Index x) {
        return x + lowbit(x);
    }


    static Index parent2(Index x) {
        return x - lowbit(x);
    }


    Value combine(Value a, Value b) const {
        ++ops.combines;
        return (a > b) ? a : b;
    }

 
    void initialize() {
        bit1.assign(n + 1, NEG_INF);
        bit2.assign(n + 1, NEG_INF);

        for (Index i = 1; i <= n; ++i) {
            bit1[i] = arr[i];
            bit2[i] = arr[i];
        }


        for (Index i = 1; i <= n; ++i) {
            const Index p = parent1(i);
            if (p <= n) {
                bit1[p] = combine(bit1[p], bit1[i]);
            }
        }

        for (Index i = n; i >= 1; --i) {
            const Index p = parent2(i);
            if (p >= 1) {
                bit2[p] = combine(bit2[p], bit2[i]);
            }
        }

        ops.node_writes += 2 * n;
    }


    void validate_range(Index l, Index r) const {
        if (l < 1 || l > n + 1 || r > n || l > r + 1) {
            throw out_of_range("Invalid query range");
        }
    }

public:
    DimaCeterchiMax() = default;


    void build(const vector<Value>& a) override {
        n = a.size();
        arr.assign(n + 1, 0);

        for (Index i = 1; i <= n; ++i) {
            if (a[i - 1] == NEG_INF) {
                throw invalid_argument("NEG_INF is reserved as the identity");
            }
            arr[i] = a[i - 1];
        }

        initialize();
        ops.reset();
    }


    Index size() const override {
        return n;
    }

    Value get(Index i) const override {
        if (i < 1 || i > n) {
            throw out_of_range("Invalid index");
        }

        ++ops.node_reads;
        return arr[i];
    }


    Value query_max(Index l, Index r) const override {
        validate_range(l, r);

        if (l == r + 1) {
            return NEG_INF;
        }

        Value ans = NEG_INF;
        Index x = l;
        Index y = r;


        while (parent1(x) <= r) {
            ++ops.node_reads;
            ans = combine(ans, bit2[x]);
            x = parent1(x);
        }


        while (parent2(y) >= l) {
            ++ops.node_reads;
            ans = combine(ans, bit1[y]);
            y = parent2(y);
        }


        if (x != y) {
            throw logic_error("Invalid query decomposition");
        }

        ++ops.node_reads;
        ans = combine(ans, arr[x]);
        return ans;
    }


    void assign(Index p, Value v) override {
        if (p < 1 || p > n) {
            throw out_of_range("Invalid update index");
        }

        if (v == NEG_INF) {
            throw invalid_argument("NEG_INF is reserved as the identity");
        }

        arr[p] = v;
        ++ops.node_writes;


        Index i = p;
        Index leftPos = p - 1;
        Index rightPos = p + 1;

        Value leftMax = NEG_INF;
        Value rightMax = NEG_INF;

        while (i <= n) {
            const Index x = i - lowbit(i) + 1;


            while (leftPos >= x) {
                const Index blockLeft = parent2(leftPos) + 1;

                if (blockLeft >= x) {
                    ++ops.node_reads;
                    leftMax = combine(leftMax, bit1[leftPos]);
                    leftPos = parent2(leftPos);
                } else {
                    break;
                }
            }

            while (rightPos < i) {
                const Index blockRight = min(n, parent1(rightPos) - 1);

                if (blockRight <= i) {
                    ++ops.node_reads;
                    rightMax = combine(rightMax, bit2[rightPos]);
                    rightPos = parent1(rightPos);
                } else {
                    break;
                }
            }


            if (rightPos == i) {
                ++ops.node_reads;
                rightMax = combine(rightMax, arr[i]);
                rightPos = i + 1;
            }


            bit1[i] = combine(combine(leftMax, v), rightMax);
            ++ops.node_writes;
            ++ops.recomputations;

            i = parent1(i);
        }

        i = p;
        leftPos = p - 1;
        rightPos = p + 1;

        leftMax = NEG_INF;
        rightMax = NEG_INF;

        while (i >= 1) {
            const Index y = min(n, i + lowbit(i) - 1);


            
            while (leftPos >= i) {
                const Index blockLeft = parent2(leftPos) + 1;

                if (blockLeft >= i) {
                    ++ops.node_reads;
                    leftMax = combine(leftMax, bit1[leftPos]);
                    leftPos = parent2(leftPos);
                } else {
                    break;
                }
            }

            if (leftPos == i) {
                ++ops.node_reads;
                leftMax = combine(leftMax, arr[i]);
                leftPos = i - 1;
            }
            
            while (rightPos <= y) {
                const Index blockRight = min(n, parent1(rightPos) - 1);

                if (blockRight <= y) {
                    ++ops.node_reads;
                    rightMax = combine(rightMax, bit2[rightPos]);
                    rightPos = parent1(rightPos);
                } else {
                    break;
                }
            }

            bit2[i] = combine(combine(leftMax, v), rightMax);
            ++ops.node_writes;
            ++ops.recomputations;

            i = parent2(i);
        }
    }

    const OpCounters& counters() const override {
        return ops;
    }

    void reset_counters() override {
        ops.reset();
    }

    bool check_invariants(string* why = nullptr) const override {
        for (Index i = 1; i <= n; ++i) {
            if (arr[i] == NEG_INF) {
                if (why) {
                    *why = "F3 violated at position " + to_string(i);
                }
                return false;
            }
        }

        for (Index i = 1; i <= n; ++i) {
            Value expected = NEG_INF;
            const Index left = i - lowbit(i) + 1;

            for (Index j = left; j <= i; ++j) {
                expected = max(expected, arr[j]);
            }

            if (bit1[i] != expected) {
                if (why) {
                    *why = "F1 violated at position " + to_string(i);
                }
                return false;
            }
        }

        for (Index i = 1; i <= n; ++i) {
            Value expected = NEG_INF;
            const Index right = min(n, i + lowbit(i) - 1);

            for (Index j = i; j <= right; ++j) {
                expected = max(expected, arr[j]);
            }

            if (bit2[i] != expected) {
                if (why) {
                    *why = "F2 violated at position " + to_string(i);
                }
                return false;
            }
        }

        if (why) {
            why->clear();
        }
        return true;
    }
};

#endif 
