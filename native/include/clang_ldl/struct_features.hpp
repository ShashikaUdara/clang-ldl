#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace clang_ldl {

struct StructFeatures {
    uint8_t holes = 0;
    uint8_t endpoints = 0;
    uint8_t junctions = 0;
    float aspect = 1.f;
};

StructFeatures compute_struct_features(const uint8_t* bitmap, int width, int height);

} // namespace clang_ldl
