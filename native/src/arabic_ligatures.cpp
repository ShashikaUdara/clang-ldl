#include "clang_ldl/arabic_ligatures.hpp"

#include <algorithm>
#include <vector>

namespace clang_ldl {

namespace {

struct LigatureRule {
    uint32_t first;
    uint32_t second;
    uint32_t output;
};

// Top Arabic bigrams → presentation-form ligature codepoints (isolated).
const LigatureRule kLigatures[] = {
    {0x0644, 0x0627, 0xFEFB}, // لا
    {0x0644, 0x0622, 0xFEF5}, // لآ
    {0x0644, 0x0623, 0xFEF7}, // لأ
    {0x0644, 0x0625, 0xFEF9}, // لإ
    {0x0644, 0x0644, 0xFDF2}, // لل (Allah ligature component)
    {0x0627, 0x0644, 0xFDF2}, // الله (approx; second pass)
    {0x0644, 0x0645, 0xFEDF}, // لم
    {0x0644, 0x0647, 0xFEE0}, // له
    {0x0641, 0x064A, 0xFEF0}, // في
    {0x0628, 0x064A, 0xFE91}, // بي
};

void merge_pair_into(Glyph& dst, const Glyph& extra) {
    const int nx = std::min(dst.box.x, extra.box.x);
    const int ny = std::min(dst.box.y, extra.box.y);
    const int nmaxx = std::max(dst.box.x + dst.box.w, extra.box.x + extra.box.w);
    const int nmaxy = std::max(dst.box.y + dst.box.h, extra.box.y + extra.box.h);
    const int nw = nmaxx - nx;
    const int nh = nmaxy - ny;
    if (nw <= 0 || nh <= 0) {
        return;
    }
    std::vector<uint8_t> nb(static_cast<size_t>(nw) * nh, 0);
    auto blit = [&](const Glyph& g) {
        for (int yy = 0; yy < g.box.h; ++yy) {
            for (int xx = 0; xx < g.box.w; ++xx) {
                const uint8_t v = g.bitmap[static_cast<size_t>(yy) * g.box.w + xx];
                if (v <= 127) {
                    continue;
                }
                const int dst_x = g.box.x - nx + xx;
                const int dst_y = g.box.y - ny + yy;
                if (dst_x < 0 || dst_y < 0 || dst_x >= nw || dst_y >= nh) {
                    continue;
                }
                nb[static_cast<size_t>(dst_y) * nw + dst_x] = 255;
            }
        }
    };
    blit(dst);
    blit(extra);
    dst.box = {nx, ny, nw, nh};
    dst.bitmap = std::move(nb);
}

} // namespace

void apply_arabic_ligatures(std::vector<Glyph>& glyphs) {
    if (glyphs.size() < 2) {
        return;
    }

    std::vector<Glyph> merged;
    merged.reserve(glyphs.size());
    for (size_t i = 0; i < glyphs.size(); ++i) {
        bool fused = false;
        if (i + 1 < glyphs.size()) {
            const uint32_t a = glyphs[i].codepoint;
            const uint32_t b = glyphs[i + 1].codepoint;
            for (const LigatureRule& rule : kLigatures) {
                if (a == rule.first && b == rule.second) {
                    Glyph g = glyphs[i];
                    merge_pair_into(g, glyphs[i + 1]);
                    g.codepoint = rule.output;
                    g.confidence = std::max(g.confidence, glyphs[i + 1].confidence);
                    merged.push_back(std::move(g));
                    i += 1;
                    fused = true;
                    break;
                }
            }
        }
        if (!fused) {
            merged.push_back(glyphs[i]);
        }
    }
    glyphs = std::move(merged);
}

} // namespace clang_ldl
