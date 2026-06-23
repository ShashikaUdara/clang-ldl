#pragma once

#include "clang_ldl/image.hpp"

#include <string>
#include <vector>

namespace clang_ldl {

/** True when pack_id is the Arabic OCR pack. */
bool pack_is_arabic(const std::string& pack_id);

/** Segment an Arabic line: fixed-width cells, else baseline cursive clustering. */
std::vector<Glyph> segment_arabic_glyphs(const Image& line_binary);

} // namespace clang_ldl
