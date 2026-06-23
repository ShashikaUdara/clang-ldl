#include "clang_ldl/pack_loader.hpp"

#include <cstdint>
#include <cstring>
#include <cstdlib>
#include <fstream>
#include <mutex>
#include <stdexcept>
#include <unordered_map>
#include <vector>

namespace clang_ldl {

namespace {

uint32_t read_u32(const uint8_t*& p) {
    uint32_t v = 0;
    std::memcpy(&v, p, 4);
    p += 4;
    return v;
}

uint16_t read_u16(const uint8_t*& p) {
    uint16_t v = 0;
    std::memcpy(&v, p, 2);
    p += 2;
    return v;
}

float read_f32(const uint8_t*& p) {
    float v = 0.f;
    std::memcpy(&v, p, 4);
    p += 4;
    return v;
}

} // namespace

GlyphPack load_pack_file(const std::string& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) {
        throw std::runtime_error("failed to open pack: " + path);
    }
    std::vector<uint8_t> data((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    if (data.size() < 10) {
        throw std::runtime_error("pack too small: " + path);
    }
    const uint8_t* p = data.data();
    if (std::memcmp(p, "CLPK", 4) != 0) {
        throw std::runtime_error("invalid pack magic: " + path);
    }
    p += 4;
    const uint32_t version = read_u32(p);
    if (version != 1) {
        throw std::runtime_error("unsupported pack version: " + std::to_string(version));
    }
    const uint16_t id_len = read_u16(p);
    if (p + id_len + 20 > data.data() + data.size()) {
        throw std::runtime_error("truncated pack header: " + path);
    }
    GlyphPack pack;
    pack.id.assign(reinterpret_cast<const char*>(p), id_len);
    p += id_len;
    pack.grid_w = static_cast<int>(read_u16(p));
    pack.grid_h = static_cast<int>(read_u16(p));
    pack.threshold = read_f32(p);
    pack.w_ncc = read_f32(p);
    pack.w_struct = read_f32(p);
    pack.w_aspect = read_f32(p);
    const uint32_t count = read_u32(p);
    pack.glyphs.reserve(count);
    for (uint32_t i = 0; i < count; ++i) {
        if (p + 20 > data.data() + data.size()) {
            throw std::runtime_error("truncated glyph header in pack: " + path);
        }
        GlyphPrototype g;
        g.codepoint = read_u32(p);
        g.holes = *p++;
        g.endpoints = *p++;
        g.junctions = *p++;
        p += 1; // padding
        g.aspect = read_f32(p);
        const uint32_t bmp_len = read_u32(p);
        if (p + bmp_len > data.data() + data.size()) {
            throw std::runtime_error("truncated glyph bitmap in pack: " + path);
        }
        g.bitmap.assign(p, p + bmp_len);
        p += bmp_len;
        pack.glyphs.push_back(std::move(g));
    }
    return pack;
}

std::string default_packs_dir() {
    const char* env = std::getenv("CLANG_LDL_PACKS_DIR");
    if (env && env[0]) {
        return env;
    }
    return "packs";
}

GlyphPack load_pack_by_id(const std::string& pack_id) {
    return load_pack_file(default_packs_dir() + "/" + pack_id + ".clpk");
}

const GlyphPack& latin_pack() {
    return pack_by_id("latin");
}

const std::vector<std::string>& tier_a_pack_ids() {
    static const std::vector<std::string> ids = {
        "latin", "cyrillic", "greek", "armenian", "georgian"};
    return ids;
}

const std::vector<std::string>& native_pack_ids() {
    static const std::vector<std::string> ids = {
        "latin", "cyrillic", "greek", "armenian", "georgian",
        "hebrew", "thai", "lao", "myanmar", "ethiopic",
        "devanagari", "bengali", "gurmukhi", "gujarati", "odia",
        "tamil", "telugu", "kannada", "malayalam", "sinhala", "arabic"};
    return ids;
}

const std::vector<std::string>& indic_pack_ids() {
    static const std::vector<std::string> ids = {
        "devanagari", "bengali", "gurmukhi", "gujarati", "odia",
        "tamil", "telugu", "kannada", "malayalam", "sinhala"};
    return ids;
}

const GlyphPack& pack_by_id(const std::string& pack_id) {
    static std::mutex mu;
    static std::unordered_map<std::string, GlyphPack> cache;
    std::lock_guard<std::mutex> lock(mu);
    const auto it = cache.find(pack_id);
    if (it != cache.end()) {
        return it->second;
    }
    try {
        cache.emplace(pack_id, load_pack_by_id(pack_id));
    } catch (const std::exception& ex) {
        throw std::runtime_error(std::string("pack load failed (") + pack_id + "): " + ex.what());
    }
    return cache.at(pack_id);
}

} // namespace clang_ldl
