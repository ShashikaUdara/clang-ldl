#pragma once

#include "clang_ldl/image.hpp"

namespace clang_ldl {

/** Match segmented glyph bitmaps to Unicode code points via template packs. */
void recognize_glyphs(std::vector<Glyph>& glyphs, const Image& line_binary = Image{});

} // namespace clang_ldl
