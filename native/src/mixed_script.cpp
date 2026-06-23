#include "clang_ldl/mixed_script.hpp"

#include "clang_ldl/confidence_calibrate.hpp"
#include "clang_ldl/pack_loader.hpp"

#include <cstdlib>

namespace clang_ldl {

bool mixed_script_fallback_enabled() {
    const char* env = std::getenv("CLANG_LDL_MIXED_SCRIPT");
    if (!env) {
        return true;
    }
    return env[0] == '1' || env[0] == 'y' || env[0] == 'Y';
}

MatchResult match_glyph_fallback_packs(const Glyph& glyph, const std::string& skip_pack_id) {
    MatchResult best;
    for (const std::string& pack_id : native_pack_ids()) {
        if (pack_id == skip_pack_id) {
            continue;
        }
        try {
            const GlyphPack& pack = pack_by_id(pack_id);
            const auto norm = normalize_glyph_bitmap(glyph, pack.grid_w, pack.grid_h);
            const MatchResult m = match_glyph(norm, pack);
            if (m.confidence > best.confidence) {
                best = m;
                best.pack_id = pack_id;
            }
        } catch (...) {
            continue;
        }
    }
    return best;
}

void recognize_glyphs_with_pack(std::vector<Glyph>& glyphs, const GlyphPack& pack) {
    const bool fallback = mixed_script_fallback_enabled();
    for (auto& g : glyphs) {
        if (g.box.w > 120 && g.box.w > g.box.h * 2) {
            g.codepoint = 0;
            g.confidence = 0.f;
            g.pack_id = pack.id;
            continue;
        }
        const auto norm = normalize_glyph_bitmap(g, pack.grid_w, pack.grid_h);
        MatchResult m = match_glyph(norm, pack);
        if (fallback && m.confidence < pack.threshold) {
            const MatchResult alt = match_glyph_fallback_packs(g, pack.id);
            if (alt.confidence > m.confidence) {
                m = alt;
            }
        }
        g.codepoint = m.codepoint;
        g.confidence = calibrate_confidence(
            m.pack_id.empty() ? pack.id : m.pack_id, m.confidence);
        g.ncc_score = m.ncc_score;
        g.struct_score = m.struct_score;
        g.pack_id = m.pack_id.empty() ? pack.id : m.pack_id;
    }
}

} // namespace clang_ldl
