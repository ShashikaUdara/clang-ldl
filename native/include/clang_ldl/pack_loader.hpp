#pragma once

#include <cstdint>
#include <string>
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
};

/** Load a .clpk file from disk. Throws on error. */
GlyphPack load_pack_file(const std::string& path);

/** Resolve default packs directory (CLANG_LDL_PACKS_DIR or ../packs). */
std::string default_packs_dir();

/** Load pack by id (e.g. "latin" -> latin.clpk). */
GlyphPack load_pack_by_id(const std::string& pack_id);

/** Cached latin pack (loaded once). */
const GlyphPack& latin_pack();

} // namespace clang_ldl
