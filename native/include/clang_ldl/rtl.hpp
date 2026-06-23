#pragma once

#include "clang_ldl/image.hpp"

#include <string>
#include <vector>

namespace clang_ldl {

/** True when the pack reads right-to-left on the page. */
bool pack_is_rtl(const std::string& pack_id);

/** Reverse glyph order for RTL scripts rendered left-to-right (optional correction). */
void apply_rtl_glyph_order(std::vector<Glyph>& glyphs, const std::string& pack_id);

} // namespace clang_ldl
