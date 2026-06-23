#pragma once

#include "clang_ldl/image.hpp"
#include "clang_ldl/pack_loader.hpp"

namespace clang_ldl {

struct MatchResult {
    uint32_t codepoint = '?';
    float confidence = 0.f;
    float ncc_score = 0.f;
    float struct_score = 0.f;
    std::string pack_id;
};

/** Normalize glyph bitmap to pack grid dimensions. */
std::vector<uint8_t> normalize_glyph_bitmap(const Glyph& glyph, int grid_w, int grid_h);

/** Match a normalized bitmap against a glyph pack. */
MatchResult match_glyph(const std::vector<uint8_t>& normalized, const GlyphPack& pack);

/** Recognize all glyphs using the given pack. */
void recognize_glyphs_with_pack(std::vector<Glyph>& glyphs, const GlyphPack& pack);

} // namespace clang_ldl
