#pragma once

#include "clang_ldl/image.hpp"

#include <vector>

namespace clang_ldl {

/** Merge adjacent recognized glyphs using the Arabic ligature bigram table. */
void apply_arabic_ligatures(std::vector<Glyph>& glyphs);

} // namespace clang_ldl
