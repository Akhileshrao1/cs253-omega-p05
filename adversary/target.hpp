// adversary/target.hpp
// WHICH max-structure the find_worst_* tools probe. Edit ONLY this file.
// Default: Structure B (Chetan's DimaCeterchiMax). To probe Structure A's max
// instead, swap the two lines marked below.
#pragma once

#include <memory>

#include "../include/range_structure.hpp"
#include "../src/structB/dima_ceterchi_max.hpp"
// #include "../src/structA/segment_tree.hpp"        // <- for Structure A
// #include "../src/structA/max_count_monoid.hpp"    // <- for Structure A

inline std::unique_ptr<RangeMax> make_range_max() {
    return std::make_unique<DimaCeterchiMax>();
    // return std::make_unique<SegTree<MaxCountMonoid>>();   // <- for Structure A
}
