#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

namespace clang_ldl {

struct GlyphPrototype {
    uint32_t codepoint = 0;
    uint8_t holes = 0;
    uint8_t endpoints = 0;
    uint8_t junctions = 0;
    float aspect = 1.f;
    std::vector<uint8_t> bitmap;
};

struct GlyphPack {
    std::string id;
    int grid_w = 20;
    int grid_h = 28;
    float threshold = 0.30f;
    float w_ncc = 0.55f;
    float w_struct = 0.25f;
    float w_aspect = 0.20f;
    std::vector<GlyphPrototype> glyphs;
    /** Codepoint → glyph indices (multi-font / ligature prototype unions). */
    std::unordered_map<uint32_t, std::vector<size_t>> glyph_by_codepoint;
};

/** Load a .clpk file from disk. Throws on error. */
GlyphPack load_pack_file(const std::string& path);

/** Resolve default packs directory (CLANG_LDL_PACKS_DIR or ../packs). */
std::string default_packs_dir();

/** Load pack by id (e.g. "latin" -> latin.clpk). */
GlyphPack load_pack_by_id(const std::string& pack_id);

/** Cached latin pack (loaded once). */
const GlyphPack& latin_pack();

/** Cached pack by id (latin, cyrillic, greek, armenian, georgian, …). */
const GlyphPack& pack_by_id(const std::string& pack_id);

/** Tier A pack ids used by script_router. */
const std::vector<std::string>& tier_a_pack_ids();

/** All native OCR pack ids (Tier A + Tier B + Indic). */
const std::vector<std::string>& native_pack_ids();

/** Indic script pack ids (shared segmentation engine). */
const std::vector<std::string>& indic_pack_ids();

} // namespace clang_ldl
