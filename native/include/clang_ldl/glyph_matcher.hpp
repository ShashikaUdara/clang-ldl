#pragma once

#include "clang_ldl/image.hpp"

namespace clang_ldl {

/** Match segmented glyph bitmaps to Unicode code points via normalized templates. */
void recognize_glyphs(std::vector<Glyph>& glyphs);

} // namespace clang_ldl
