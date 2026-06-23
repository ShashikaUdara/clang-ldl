#pragma once

#include "clang_ldl/image.hpp"

#include <string>
#include <vector>

namespace clang_ldl {

/** Pack-specific glyph post-processing before template matching. */
void postprocess_glyphs_for_pack(std::vector<Glyph>& glyphs, const std::string& pack_id);

} // namespace clang_ldl
