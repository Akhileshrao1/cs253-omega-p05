#include <iostream>
#include <vector>
#include "../src/structA/segment_tree.hpp"
#include "../src/structA/max_count_monoid.hpp"
#include "../src/structA/gcd_monoid.hpp"

int main()
{
    int sensor_count;
    if (!(std::cin >> sensor_count) || sensor_count <= 0)
    {
        std::cerr << "Expected a positive sensor count\n";
        return 1;
    }

    std::vector<Value> initial_readings(sensor_count);
    std::vector<Value> initial_calibration(sensor_count);

    for (Value &reading : initial_readings)
    {
        if (!(std::cin >> reading))
        {
            std::cerr << "Missing or invalid sensor reading\n";
            return 1;
        }
    }

    for (Value &factor : initial_calibration)
    {
        if (!(std::cin >> factor))
        {
            std::cerr << "Missing or invalid calibration factor\n";
            return 1;
        }
    }

    SegTree<MaxCountMonoid> readings;
    readings.build(initial_readings);

    SegTree<GcdMonoid> calibration;
    calibration.build(initial_calibration);
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