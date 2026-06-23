#include "clang_ldl/script_preprocess.hpp"

#include "clang_ldl/arabic_ligatures.hpp"
#include "clang_ldl/mark_attachment.hpp"
#include "clang_ldl/rtl.hpp"
#include "clang_ldl/segment_arabic.hpp"

namespace clang_ldl {

void postprocess_glyphs_for_pack(std::vector<Glyph>& glyphs, const std::string& pack_id) {
    glyphs = attach_marks(std::move(glyphs), pack_id);
    apply_rtl_glyph_order(glyphs, pack_id);
}

void postrecognize_glyphs_for_pack(std::vector<Glyph>& glyphs, const std::string& pack_id) {
    if (pack_is_arabic(pack_id)) {
        apply_arabic_ligatures(glyphs);
    }
}

} // namespace clang_ldl
