#pragma once

#include "clang_ldl/image.hpp"

#include <string>
#include <vector>

namespace clang_ldl {

/** Merge Thai/Lao tone/vowel marks with adjacent base consonants after segmentation. */
std::vector<Glyph> attach_marks(std::vector<Glyph> glyphs, const std::string& pack_id);

} // namespace clang_ldl
