#include "clang_ldl/glyph_matcher.hpp"

#include "clang_ldl/pack_loader.hpp"
#include "clang_ldl/script_router.hpp"
#include "clang_ldl/template_matcher.hpp"

namespace clang_ldl {

void recognize_glyphs(std::vector<Glyph>& glyphs, const Image& line_binary) {
    const std::string pack_id = route_script_for_line(line_binary);
    const GlyphPack& pack = pack_id == "latin" ? latin_pack() : latin_pack();
    recognize_glyphs_with_pack(glyphs, pack);
}

} // namespace clang_ldl
