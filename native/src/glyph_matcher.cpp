#include "clang_ldl/glyph_matcher.hpp"

#include "clang_ldl/pack_loader.hpp"
#include "clang_ldl/script_preprocess.hpp"
#include "clang_ldl/script_router.hpp"
#include "clang_ldl/segment_arabic.hpp"
#include "clang_ldl/segment_indic.hpp"
#include "clang_ldl/template_matcher.hpp"

namespace clang_ldl {

void recognize_glyphs(std::vector<Glyph>& glyphs, const Image& line_binary) {
    const std::string pack_id = route_script_for_line(line_binary);
    if (pack_is_indic(pack_id)) {
        auto indic = segment_indic_glyphs(line_binary);
        if (!indic.empty()) {
            glyphs = std::move(indic);
        }
    } else if (pack_is_arabic(pack_id)) {
        auto ar = segment_arabic_glyphs(line_binary);
        if (!ar.empty()) {
            glyphs = std::move(ar);
        }
    }
    postprocess_glyphs_for_pack(glyphs, pack_id);
    const GlyphPack& pack = pack_by_id(pack_id);
    recognize_glyphs_with_pack(glyphs, pack);
    postrecognize_glyphs_for_pack(glyphs, pack_id);
}

} // namespace clang_ldl
