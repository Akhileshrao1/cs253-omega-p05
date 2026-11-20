#include <iostream>
#include <stdexcept>
#include <string_view>
#include <vector>
#include "../src/structA/segment_tree.hpp"
#include "../src/structA/max_count_monoid.hpp"
#include "../src/structA/gcd_monoid.hpp"
#include "../src/structB/dima_ceterchi_max.hpp"

static int replay(bool use_structure_b)
{
    int sensor_count;
    if (!(std::cin >> sensor_count) || sensor_count <= 0)
    {
        std::cerr << "Expected a positive sensor count\n";
        return 1;
    }

    std::vector<Value> initial_readings(sensor_count);
    std::vector<Value> initial_calibration(use_structure_b ? 0 : sensor_count);

    for (Value &reading : initial_readings)
    {
        if (!(std::cin >> reading))
        {
            std::cerr << "Missing or invalid sensor reading\n";
            return 1;
        }
    }

    for (int i = 0; i < sensor_count; ++i)
    {
        Value factor;
        if (!(std::cin >> factor))
        {
            std::cerr << "Missing or invalid calibration factor\n";
            return 1;
        }
        if (use_structure_b)
        {
            GcdMonoid::from_value(factor);
        }
        else
        {
            initial_calibration[i] = factor;
        }
    }

    SegTree<MaxCountMonoid> segment_readings;
    DimaCeterchiMax fenwick_readings;
    RangeMax &readings = use_structure_b
        ? static_cast<RangeMax &>(fenwick_readings)
        : static_cast<RangeMax &>(segment_readings);
    readings.build(initial_readings);

    SegTree<GcdMonoid> calibration;
    if (!use_structure_b)
    {
        calibration.build(initial_calibration);
    }
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
            else if (use_structure_b)
            {
                GcdMonoid::from_value(value);
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

            if (use_structure_b)
            {
                std::cout << readings.query_max(left, right) << '\n';
            }
            else
            {
                MaxCount result = segment_readings.query_max_count(left, right);
                std::cout << result.max << ' ' << result.count << ' '
                          << calibration.query_gcd(left, right) << '\n';
            }
            break;
        }

        default:
            std::cerr << "Unknown command: " << command << '\n';
            return 1;
        }
    }
    return 0;
}

int main(int argc, char **argv)
{
    std::string_view structure = "A";
    if (argc != 1)
    {
        if (argc != 3 || std::string_view(argv[1]) != "--structure")
        {
            std::cerr << "Usage: sensor_demo [--structure A|B]\n";
            return 1;
        }
        structure = argv[2];
        if (structure != "A" && structure != "B")
        {
            std::cerr << "Expected --structure A or --structure B\n";
            return 1;
        }
    }

    std::ios::sync_with_stdio(false);
    try
    {
        return replay(structure == "B");
    }
    catch (const std::invalid_argument &error)
    {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
