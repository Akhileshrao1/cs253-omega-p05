#include <cstdint>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <psapi.h>
#elif defined(__linux__)
#include <sys/resource.h>
#endif

#include "../src/structA/gcd_monoid.hpp"
#include "../src/structA/max_count_monoid.hpp"
#include "../src/structA/segment_tree.hpp"
#include "../src/structB/dima_ceterchi_max.hpp"
#include "../src/structB/prefix_max_fenwick.hpp"

using namespace std;

static size_t padded_capacity(size_t n) {
    size_t cap = 1;
    while (cap < n) {
        if (cap > numeric_limits<size_t>::max() / 2)
            throw length_error("n is too large for a padded segment-tree capacity");
        cap *= 2;
    }
    return cap;
}

static void print_row(const string& label, size_t n, size_t cap,
                      size_t measured_bytes, size_t model_bytes) {
    cout << "structure_arrays," << label << ',' << n << ',' << cap << ','
         << measured_bytes << ',' << model_bytes << ','
         << static_cast<double>(measured_bytes) / n << ','
         << static_cast<double>(model_bytes) / n << '\n';
}

static bool peak_process_memory_bytes(uint64_t& bytes) {
#if defined(_WIN32)
    PROCESS_MEMORY_COUNTERS_EX counters{};
    HMODULE psapi = LoadLibraryW(L"psapi.dll");
    if (psapi == nullptr) return false;
    using GetProcessMemoryInfoFn = BOOL (WINAPI *)(
        HANDLE, PPROCESS_MEMORY_COUNTERS, DWORD);
    const auto get_info = reinterpret_cast<GetProcessMemoryInfoFn>(
        GetProcAddress(psapi, "GetProcessMemoryInfo"));
    if (get_info == nullptr ||
        !get_info(GetCurrentProcess(),
                  reinterpret_cast<PPROCESS_MEMORY_COUNTERS>(&counters),
                  sizeof(counters))) {
        FreeLibrary(psapi);
        return false;
    }
    bytes = static_cast<uint64_t>(counters.PeakWorkingSetSize);
    FreeLibrary(psapi);
    return true;
#elif defined(__linux__)
    rusage usage{};
    if (getrusage(RUSAGE_SELF, &usage) != 0) return false;
    bytes = static_cast<uint64_t>(usage.ru_maxrss) * 1024;
    return true;
#else
    (void)bytes;
    return false;
#endif
}

int main(int argc, char** argv) {
    try {
        const size_t n = argc > 1 ? stoull(argv[1]) : 1'000'000;
        if (n == 0) throw invalid_argument("n must be positive");
        const size_t cap = padded_capacity(n);

        vector<Value> readings(n);
        vector<Value> calibration(n);
        for (size_t i = 0; i < n; ++i) {
            readings[i] = static_cast<Value>(i % 100'003);
            calibration[i] = static_cast<Value>(i % 1'000'000'001ULL);
        }

        SegTree<MaxCountMonoid> max_count_tree(readings);
        SegTree<GcdMonoid> gcd_tree(calibration);
        DimaCeterchiMax two_tree_fenwick;
        two_tree_fenwick.build(readings);
        PrefixMaxFenwick prefix_baseline;
        prefix_baseline.build(readings);

        const size_t max_count_model =
            2 * cap * sizeof(MaxCount) + n * sizeof(Value);
        const size_t gcd_model =
            2 * cap * sizeof(Value) + n * sizeof(Value);
        const size_t two_tree_model = 3 * (n + 1) * sizeof(Value);
        const size_t prefix_model = 2 * (n + 1) * sizeof(Value);

        cout << "record,structure,n,cap,actual_bytes,model_bytes,"
                "actual_bytes_per_element,model_bytes_per_element\n";
        print_row("A_max_count", n, cap, max_count_tree.storage_bytes(), max_count_model);
        print_row("A_gcd", n, cap, gcd_tree.storage_bytes(), gcd_model);
        print_row("A_sensor_app", n, cap,
                  max_count_tree.storage_bytes() + gcd_tree.storage_bytes(),
                  max_count_model + gcd_model);
        print_row("B_two_tree_max", n, cap,
                  two_tree_fenwick.storage_bytes(), two_tree_model);
        print_row("B_prefix_baseline", n, cap,
                  prefix_baseline.storage_bytes(), prefix_model);
        print_row("B_sensor_app_with_A_gcd", n, cap,
                  two_tree_fenwick.storage_bytes() + gcd_tree.storage_bytes(),
                  two_tree_model + gcd_model);
        const size_t readings_bytes = readings.capacity() * sizeof(Value);
        const size_t calibration_bytes = calibration.capacity() * sizeof(Value);
        cout << "input_array,readings," << n << ',' << cap << ',' << readings_bytes
             << ",,,\n";
        cout << "input_array,calibration," << n << ',' << cap << ',' << calibration_bytes
             << ",,,\n";
        cout << "input_array,both_inputs," << n << ',' << cap << ','
             << readings_bytes + calibration_bytes << ",,,\n";

        uint64_t process_peak_bytes = 0;
        if (peak_process_memory_bytes(process_peak_bytes)) {
            cout << "whole_process,all_constructed_variants," << n << ',' << cap << ','
                 << process_peak_bytes << ",,,\n";
        } else {
            cerr << "Process peak memory is not supported on this operating system.\n";
        }
        cerr << "Structure rows count dynamic vector payload by capacity. Input rows count the two input vectors. "
                "The whole_process row includes inputs, all constructed variants, runtime and other process memory. "
                "The model uses logical sizes and excludes allocator metadata.\n";
    } catch (const exception& error) {
        cerr << "memory_footprint: " << error.what() << '\n';
        return 1;
    }
}
