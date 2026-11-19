#include "../oracle/oracle.hpp"
#include <cassert>
#include <iostream>

int main() {
    OracleMaxCount mx;
    mx.build({4, 7, 2, 7, 5, 3});
    auto a = mx.query_max_count(1, 6);
    assert(a.max == 7 && a.count == 2);
    assert(mx.query_max(2, 4) == 7);
    mx.assign(2, 1);
    a = mx.query_max_count(1, 6);
    assert(a.max == 7 && a.count == 1);
    assert(mx.query_max_count(3, 2).max == NEG_INF);
    assert(mx.query_max_count(3, 2).count == 0);

    OracleGcd g;
    g.build({12, 18, 24, 6});
    assert(g.query_gcd(1, 4) == 6);
    assert(g.query_gcd(2, 3) == 6);
    g.assign(1, 0);
    assert(g.query_gcd(1, 4) == 6);
    assert(g.query_gcd(2, 1) == 0);

    OraclePrefixMax p;
    p.build({4, 7, 2, 7, 5, 3});
    assert(p.query_prefix_max(0) == NEG_INF);
    assert(p.query_prefix_max(4) == 7);
    p.assign(2, 1);
    p.assign(4, 0);
    assert(p.query_prefix_max(4) == 4);

    string why;
    assert(mx.check_invariants(&why));
    assert(g.check_invariants(&why));
    assert(p.check_invariants(&why));

    bool threw = false;
    try { mx.assign(1, NEG_INF); }
    catch (const invalid_argument&) { threw = true; }
    assert(threw);

    threw = false;
    try { g.assign(1, -1); }
    catch (const invalid_argument&) { threw = true; }
    assert(threw);

    std::cout << "Oracle smoke tests passed.\n";
}
