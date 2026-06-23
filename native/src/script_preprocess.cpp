#include "clang_ldl/script_preprocess.hpp"

#include "clang_ldl/mark_attachment.hpp"
#include "clang_ldl/rtl.hpp"

namespace clang_ldl {

void postprocess_glyphs_for_pack(std::vector<Glyph>& glyphs, const std::string& pack_id) {
    glyphs = attach_marks(std::move(glyphs), pack_id);
    apply_rtl_glyph_order(glyphs, pack_id);
}

} // namespace clang_ldl
