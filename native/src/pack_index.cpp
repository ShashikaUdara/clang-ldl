#include "clang_ldl/pack_index.hpp"

namespace clang_ldl {

void build_glyph_index(GlyphPack& pack) {
    pack.glyph_by_codepoint.clear();
    for (size_t i = 0; i < pack.glyphs.size(); ++i) {
        pack.glyph_by_codepoint[pack.glyphs[i].codepoint].push_back(i);
    }
}

} // namespace clang_ldl
