#pragma once

#include "clang_ldl/image.hpp"
#include "clang_ldl/template_matcher.hpp"

#include <string>

namespace clang_ldl {

/** True when per-glyph cross-pack fallback is enabled (default on). */
bool mixed_script_fallback_enabled();

/** Match a glyph against all native packs except skip_pack_id; returns best result. */
MatchResult match_glyph_fallback_packs(const Glyph& glyph, const std::string& skip_pack_id);

/** Recognize glyphs with line pack plus per-glyph fallback for low-confidence matches. */
void recognize_glyphs_with_pack(std::vector<Glyph>& glyphs, const GlyphPack& pack);

} // namespace clang_ldl
