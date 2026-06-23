#pragma once

#include "clang_ldl/pack_loader.hpp"

namespace clang_ldl {

/** Build codepoint → glyph index map (supports multi-font prototype unions). */
void build_glyph_index(GlyphPack& pack);

} // namespace clang_ldl
