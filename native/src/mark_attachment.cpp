#include "clang_ldl/mark_attachment.hpp"

#include <algorithm>
#include <vector>

namespace clang_ldl {

namespace {

bool pack_uses_mark_attachment(const std::string& pack_id) {
    return pack_id == "thai" || pack_id == "lao";
}

bool is_mark_like(const Glyph& g, int line_h) {
    if (line_h <= 0) {
        return false;
    }
    const int min_h = std::max(4, line_h / 3);
    return g.box.h <= min_h || g.box.w <= std::max(3, g.box.h / 2);
}

void blit_glyph(std::vector<uint8_t>& nb, int nw, int nh, int nx, int ny, const Glyph& g) {
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
}

void merge_pair(Glyph& prev, const Glyph& cur) {
    const int nx = std::min(prev.box.x, cur.box.x);
    const int ny = std::min(prev.box.y, cur.box.y);
    const int nmaxx = std::max(prev.box.x + prev.box.w, cur.box.x + cur.box.w);
    const int nmaxy = std::max(prev.box.y + prev.box.h, cur.box.y + cur.box.h);
    const int nw = nmaxx - nx;
    const int nh = nmaxy - ny;
    if (nw <= 0 || nh <= 0) {
        return;
    }
    std::vector<uint8_t> nb(static_cast<size_t>(nw) * nh, 0);
    blit_glyph(nb, nw, nh, nx, ny, prev);
    blit_glyph(nb, nw, nh, nx, ny, cur);
    prev.box = {nx, ny, nw, nh};
    prev.bitmap = std::move(nb);
}

} // namespace

std::vector<Glyph> attach_marks(std::vector<Glyph> glyphs, const std::string& pack_id) {
    if (!pack_uses_mark_attachment(pack_id) || glyphs.size() < 2) {
        return glyphs;
    }

    int line_h = 0;
    for (const auto& g : glyphs) {
        line_h = std::max(line_h, g.box.y + g.box.h);
    }
    for (const auto& g : glyphs) {
        line_h = std::max(line_h, g.box.h);
    }

    std::vector<Glyph> merged;
    merged.push_back(glyphs.front());
    for (size_t i = 1; i < glyphs.size(); ++i) {
        Glyph& prev = merged.back();
        const Glyph& cur = glyphs[i];
        const int gap = cur.box.x - (prev.box.x + prev.box.w);
        const int x_overlap = std::min(prev.box.x + prev.box.w, cur.box.x + cur.box.w)
            - std::max(prev.box.x, cur.box.x);
        const bool mark_stack = is_mark_like(cur, line_h)
            && (gap <= 4 || x_overlap > 0)
            && std::abs(prev.box.y - cur.box.y) <= std::max(prev.box.h, cur.box.h);
        if (mark_stack) {
            merge_pair(prev, cur);
        } else {
            merged.push_back(cur);
        }
    }
    return merged;
}

} // namespace clang_ldl
