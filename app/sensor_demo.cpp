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

    int command_count;
    if (!(std::cin >> command_count) || command_count < 0)
    {
        return 1;
    }

    for (int i = 0; i < command_count; ++i)
    {
        char command;
        if (!(std::cin >> command))
        {
            return 1;
        }

        switch (command)
        {
        case 'R':
        case 'C':
        {
            Index position;
            Value value;
            if (!(std::cin >> position >> value) ||
                position == 0 || position > readings.size())
            {
                return 1;
            }

            if (command == 'R')
            {
                readings.assign(position, value);
            }
            else
            {
                calibration.assign(position, value);
            }
            break;
        }

        case 'Q':
        {
            Index left, right;
            if (!(std::cin >> left >> right) ||
                left == 0 || left > right || right > readings.size())
            {
                return 1;
            }

            MaxCount result = readings.query_max_count(left, right);
            std::cout << result.max << ' ' << result.count << ' '
                      << calibration.query_gcd(left, right) << '\n';
            break;
        }

        default:
            std::cerr << "Unknown command: " << command << '\n';
            return 1;
        }
    }
}