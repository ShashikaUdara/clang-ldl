#include "clang_ldl/script_router.hpp"

#include "clang_ldl/pack_loader.hpp"
#include "clang_ldl/pipeline.hpp"
#include "clang_ldl/template_matcher.hpp"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <string>

namespace clang_ldl {

namespace {

std::string pack_hint_from_env() {
    const char* env = std::getenv("CLANG_LDL_PACK_ID");
    if (!env || !env[0]) {
        return {};
    }
    return std::string(env);
}

} // namespace

std::string route_script_for_line(const Image& line_binary) {
    const std::string hint = pack_hint_from_env();
    if (!hint.empty()) {
        return hint;
    }
    auto glyphs = segment_glyphs(line_binary);
    if (glyphs.empty()) {
        return "latin";
    }

    const int sample = std::min(static_cast<int>(glyphs.size()), 5);
    float best_non_latin = -1.f;
    std::string best_non_latin_id = "cyrillic";
    float latin_score = -1.f;

    for (const std::string& pack_id : tier_a_pack_ids()) {
        try {
            const GlyphPack& pack = pack_by_id(pack_id);
            float sum = 0.f;
            int count = 0;
            for (int i = 0; i < sample; ++i) {
                const auto norm =
                    normalize_glyph_bitmap(glyphs[static_cast<size_t>(i)], pack.grid_w, pack.grid_h);
                const MatchResult m = match_glyph(norm, pack);
                sum += m.confidence;
                ++count;
            }
            const float avg = count > 0 ? sum / static_cast<float>(count) : 0.f;
            if (pack_id == "latin") {
                latin_score = avg;
            } else if (avg > best_non_latin) {
                best_non_latin = avg;
                best_non_latin_id = pack_id;
            }
        } catch (...) {
            continue;
        }
    }

    if (best_non_latin >= 0.20f && best_non_latin > latin_score + 0.10f) {
        return best_non_latin_id;
    }
    if (latin_score >= 0.15f) {
        return "latin";
    }
    return best_non_latin_id;
}

} // namespace clang_ldl
