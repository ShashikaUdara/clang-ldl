#include "clang_ldl/template_matcher.hpp"

#include "clang_ldl/struct_features.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace clang_ldl {

namespace {

float ncc_score(const std::vector<uint8_t>& a, const std::vector<uint8_t>& b) {
    int agree = 0;
    int total = 0;
    const size_t n = std::min(a.size(), b.size());
    for (size_t i = 0; i < n; ++i) {
        const bool pa = a[i] > 127;
        const bool pb = b[i] > 127;
        if (pa || pb) {
            ++total;
            if (pa == pb) {
                ++agree;
            }
        }
    }
    if (total == 0) {
        return 0.f;
    }
    return static_cast<float>(agree) / static_cast<float>(total);
}

float struct_similarity(const StructFeatures& a, const GlyphPrototype& b) {
    const auto diff = [](int x, int y) { return static_cast<float>(std::abs(x - y)); };
    const float hole_pen = diff(a.holes, b.holes) / 4.f;
    const float ep_pen = diff(a.endpoints, b.endpoints) / 8.f;
    const float jn_pen = diff(a.junctions, b.junctions) / 8.f;
    const float asp_pen = std::fabs(a.aspect - b.aspect) / 2.f;
    const float penalty = hole_pen + ep_pen + jn_pen + asp_pen;
    return std::max(0.f, 1.f - penalty * 0.25f);
}

float aspect_penalty(float a, float b) {
    return std::max(0.f, 1.f - std::fabs(a - b));
}

float score_prototype(
    const std::vector<uint8_t>& normalized,
    const StructFeatures& feats,
    const GlyphPack& pack,
    const GlyphPrototype& proto) {
    const float ncc = ncc_score(normalized, proto.bitmap);
    const float st = struct_similarity(feats, proto);
    const float asp = aspect_penalty(feats.aspect, proto.aspect);
    return pack.w_ncc * ncc + pack.w_struct * st + pack.w_aspect * asp;
}

} // namespace

std::vector<uint8_t> normalize_glyph_bitmap(const Glyph& glyph, int grid_w, int grid_h) {
    std::vector<uint8_t> out(static_cast<size_t>(grid_w) * grid_h, 0);
    if (glyph.box.w <= 0 || glyph.box.h <= 0) {
        return out;
    }
    const float sx = static_cast<float>(grid_w) / static_cast<float>(glyph.box.w);
    const float sy = static_cast<float>(grid_h) / static_cast<float>(glyph.box.h);
    const float scale = std::min(sx, sy);
    const int nw = std::max(1, static_cast<int>(glyph.box.w * scale));
    const int nh = std::max(1, static_cast<int>(glyph.box.h * scale));
    const int ox = (grid_w - nw) / 2;
    const int oy = (grid_h - nh) / 2;
    for (int y = 0; y < nh; ++y) {
        for (int x = 0; x < nw; ++x) {
            const int sx_i = std::min(glyph.box.w - 1, static_cast<int>(x / scale));
            const int sy_i = std::min(glyph.box.h - 1, static_cast<int>(y / scale));
            const uint8_t v = glyph.bitmap[static_cast<size_t>(sy_i) * glyph.box.w + sx_i];
            if (v > 127) {
                out[static_cast<size_t>(oy + y) * grid_w + (ox + x)] = 255;
            }
        }
    }
    return out;
}

MatchResult match_glyph(const std::vector<uint8_t>& normalized, const GlyphPack& pack) {
    MatchResult best;
    best.pack_id = pack.id;
    if (normalized.empty() || pack.glyphs.empty()) {
        return best;
    }
    StructFeatures feats{};
    if (pack.w_struct > 0.f) {
        feats = compute_struct_features(normalized.data(), pack.grid_w, pack.grid_h);
    }

    if (!pack.glyph_by_codepoint.empty()) {
        for (const auto& entry : pack.glyph_by_codepoint) {
            float codepoint_best = -1.f;
            uint32_t cp = entry.first;
            float best_ncc = 0.f;
            float best_st = 0.f;
            for (size_t idx : entry.second) {
                const auto& proto = pack.glyphs[idx];
                const float fused = score_prototype(normalized, feats, pack, proto);
                if (fused > codepoint_best) {
                    codepoint_best = fused;
                    cp = proto.codepoint;
                    best_ncc = ncc_score(normalized, proto.bitmap);
                    best_st = struct_similarity(feats, proto);
                }
            }
            if (codepoint_best > best.confidence) {
                best.confidence = codepoint_best;
                best.codepoint = cp;
                best.ncc_score = best_ncc;
                best.struct_score = best_st;
            }
        }
    } else {
        for (const auto& proto : pack.glyphs) {
            const float fused = score_prototype(normalized, feats, pack, proto);
            if (fused > best.confidence) {
                best.confidence = fused;
                best.codepoint = proto.codepoint;
                best.ncc_score = ncc_score(normalized, proto.bitmap);
                best.struct_score = struct_similarity(feats, proto);
            }
        }
    }

    if (best.confidence < pack.threshold) {
        best.codepoint = '?';
    }
    return best;
}

} // namespace clang_ldl
