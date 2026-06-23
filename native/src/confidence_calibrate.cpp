#include "clang_ldl/confidence_calibrate.hpp"

#include <algorithm>
#include <unordered_map>
#include <utility>

namespace clang_ldl {

namespace {

struct CalCoeffs {
    float a;
    float b;
};

const std::unordered_map<std::string, CalCoeffs>& calibration_table() {
    static const std::unordered_map<std::string, CalCoeffs> table = {
        {"latin", {1.05f, -0.02f}},
        {"cyrillic", {1.08f, -0.04f}},
        {"greek", {1.06f, -0.03f}},
        {"armenian", {1.06f, -0.03f}},
        {"georgian", {1.06f, -0.03f}},
        {"hebrew", {1.04f, -0.02f}},
        {"thai", {1.05f, -0.03f}},
        {"lao", {1.05f, -0.03f}},
        {"myanmar", {1.10f, -0.05f}},
        {"ethiopic", {1.04f, -0.02f}},
        {"devanagari", {1.08f, -0.04f}},
        {"bengali", {1.08f, -0.04f}},
        {"gurmukhi", {1.08f, -0.04f}},
        {"gujarati", {1.08f, -0.04f}},
        {"odia", {1.08f, -0.04f}},
        {"tamil", {1.08f, -0.04f}},
        {"telugu", {1.08f, -0.04f}},
        {"kannada", {1.08f, -0.04f}},
        {"malayalam", {1.08f, -0.04f}},
        {"sinhala", {1.08f, -0.04f}},
        {"arabic", {1.07f, -0.04f}},
    };
    return table;
}

} // namespace

float calibrate_confidence(const std::string& pack_id, float raw_score) {
    const auto& table = calibration_table();
    const auto it = table.find(pack_id);
    const float a = it != table.end() ? it->second.a : 1.f;
    const float b = it != table.end() ? it->second.b : 0.f;
    return std::clamp(a * raw_score + b, 0.f, 1.f);
}

} // namespace clang_ldl
