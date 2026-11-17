#include <iostream>

#include "../src/structA/segment_tree.hpp"
#include "../src/structA/max_count_monoid.hpp"
#include "../src/structA/gcd_monoid.hpp"

int main()
{
    SegTree<MaxCountMonoid> readings;
    readings.build({6, 12, 12, 18});

    SegTree<GcdMonoid> calibration;
    calibration.build({6, 12, 18, 24});

    Index position;
    Value new_reading;
    if (!(std::cin >> position >> new_reading))
    {
        return 1;
    }
    readings.assign(position, new_reading);
    calibration.assign(1, Value{5}); // Change the first calibration value from 6 to 5.

    MaxCount result = readings.query_max_count(1, 4);

    std::cout << "Maximum: " << result.max << '\n';
    std::cout << "Count: " << result.count << '\n';
    std::cout << "GCD: " << calibration.query_gcd(1, 4) << '\n';
}