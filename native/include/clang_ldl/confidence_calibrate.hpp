#pragma once

#include <string>

namespace clang_ldl {

/** Per-pack linear confidence calibration (Platt-style: a·raw + b). */
float calibrate_confidence(const std::string& pack_id, float raw_score);

} // namespace clang_ldl
