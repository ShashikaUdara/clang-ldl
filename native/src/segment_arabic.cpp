#include "clang_ldl/segment_arabic.hpp"

#include "clang_ldl/pipeline.hpp"

#include <algorithm>
#include <queue>
#include <vector>

namespace clang_ldl {

namespace {

bool looks_like_fixed_width_cells(const std::vector<Glyph>& glyphs) {
    if (glyphs.size() < 2) {
        return true;
    }
    std::vector<int> widths;
    std::vector<int> gaps;
    widths.reserve(glyphs.size());
    gaps.reserve(glyphs.size() - 1);
    for (size_t i = 0; i < glyphs.size(); ++i) {
        widths.push_back(glyphs[i].box.w);
        if (i > 0) {
            gaps.push_back(glyphs[i].box.x - (glyphs[i - 1].box.x + glyphs[i - 1].box.w));
        }
    }
    std::sort(widths.begin(), widths.end());
    std::sort(gaps.begin(), gaps.end());
    const int median_w = widths[widths.size() / 2];
    const int median_gap = gaps[gaps.size() / 2];
    if (median_w <= 0) {
        return false;
    }
    return median_gap >= median_w / 4 && median_gap <= median_w * 2;
}

struct Component {
    int minx;
    int miny;
    int maxx;
    int maxy;
    int count;
};

std::vector<Component> extract_components(const Image& line_binary) {
    const int w = line_binary.width;
    const int h = line_binary.height;
    std::vector<int> labels(static_cast<size_t>(w) * h, -1);
    int next_label = 0;
    const int dx[4] = {1, -1, 0, 0};
    const int dy[4] = {0, 0, 1, -1};

    auto idx = [w](int x, int y) { return static_cast<size_t>(y) * w + x; };

    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            if (line_binary.at(x, y) <= 127 || labels[idx(x, y)] >= 0) {
                continue;
            }
            int minx = x, maxx = x, miny = y, maxy = y;
            std::queue<std::pair<int, int>> q;
            q.push({x, y});
            labels[idx(x, y)] = next_label;
            while (!q.empty()) {
                auto [cx, cy] = q.front();
                q.pop();
                minx = std::min(minx, cx);
                maxx = std::max(maxx, cx);
                miny = std::min(miny, cy);
                maxy = std::max(maxy, cy);
                for (int k = 0; k < 4; ++k) {
                    const int nx = cx + dx[k];
                    const int ny = cy + dy[k];
                    if (nx < 0 || ny < 0 || nx >= w || ny >= h) {
                        continue;
                    }
                    if (line_binary.at(nx, ny) <= 127 || labels[idx(nx, ny)] >= 0) {
                        continue;
                    }
                    labels[idx(nx, ny)] = next_label;
                    q.push({nx, ny});
                }
            }
            ++next_label;
        }
    }

    std::vector<Component> comps(next_label, {w, h, 0, 0, 0});
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            const int lab = labels[idx(x, y)];
            if (lab < 0) {
                continue;
            }
            auto& c = comps[lab];
            c.minx = std::min(c.minx, x);
            c.maxx = std::max(c.maxx, x);
            c.miny = std::min(c.miny, y);
            c.maxy = std::max(c.maxy, y);
            c.count++;
        }
    }

    const int min_pixels = std::max(4, (h * h) / 250);
    std::vector<Component> filtered;
    for (const auto& c : comps) {
        const int gw = c.maxx - c.minx + 1;
        const int gh = c.maxy - c.miny + 1;
        if (c.count < min_pixels || gw < 2 || gh < 4 || gw > w * 9 / 10) {
            continue;
        }
        filtered.push_back(c);
    }
    std::sort(filtered.begin(), filtered.end(), [](const Component& a, const Component& b) {
        return a.minx < b.minx;
    });
    return filtered;
}

int component_baseline(const Component& c) {
    return c.maxy;
}

Glyph component_to_glyph(const Image& line, const Component& c) {
    const int gw = c.maxx - c.minx + 1;
    const int gh = c.maxy - c.miny + 1;
    Glyph g;
    g.box = {c.minx, c.miny, gw, gh};
    g.bitmap.resize(static_cast<size_t>(gw) * gh);
    for (int yy = 0; yy < gh; ++yy) {
        for (int xx = 0; xx < gw; ++xx) {
            g.bitmap[static_cast<size_t>(yy) * gw + xx] =
                line.at(c.minx + xx, c.miny + yy) > 127 ? 255 : 0;
        }
    }
    return g;
}

void merge_glyph_pair(Glyph& prev, const Glyph& cur) {
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
    blit(prev);
    blit(cur);
    prev.box = {nx, ny, nw, nh};
    prev.bitmap = std::move(nb);
}

std::vector<Glyph> cluster_baseline_glyphs(
    const std::vector<Component>& comps, const Image& line, int line_h) {
    if (comps.empty()) {
        return {};
    }

    std::vector<int> baselines;
    baselines.reserve(comps.size());
    for (const auto& c : comps) {
        baselines.push_back(component_baseline(c));
    }
    std::sort(baselines.begin(), baselines.end());
    const int median_base = baselines[baselines.size() / 2];
    const int base_tol = std::max(4, line_h / 6);

    std::vector<int> widths;
    widths.reserve(comps.size());
    for (const auto& c : comps) {
        widths.push_back(c.maxx - c.minx + 1);
    }
    std::sort(widths.begin(), widths.end());
    const int median_w = std::max(6, widths[widths.size() / 2]);
    const int word_gap = std::max(median_w, line_h / 2);

    std::vector<Glyph> glyphs;
    glyphs.push_back(component_to_glyph(line, comps.front()));
    for (size_t i = 1; i < comps.size(); ++i) {
        const Component& cur_c = comps[i];
        Glyph& prev = glyphs.back();
        const int gap = cur_c.minx - (prev.box.x + prev.box.w);
        const int prev_base = prev.box.y + prev.box.h - 1;
        const bool same_baseline = std::abs(component_baseline(cur_c) - median_base) <= base_tol
            && std::abs(prev_base - median_base) <= base_tol;
        const int x_overlap = std::min(prev.box.x + prev.box.w, cur_c.maxx + 1)
            - std::max(prev.box.x, cur_c.minx);
        const bool cursive_touch =
            same_baseline && gap <= std::max(2, median_w / 6) && gap < word_gap;
        const bool overlap_merge = same_baseline && x_overlap > 0 && gap < word_gap;
        if (cursive_touch || overlap_merge) {
            merge_glyph_pair(prev, component_to_glyph(line, cur_c));
        } else {
            glyphs.push_back(component_to_glyph(line, cur_c));
        }
    }
    return glyphs;
}

std::vector<Glyph> merge_touching_arabic_glyphs(const std::vector<Glyph>& glyphs) {
    if (glyphs.size() < 2) {
        return glyphs;
    }
    std::vector<Glyph> merged;
    merged.push_back(glyphs.front());
    for (size_t i = 1; i < glyphs.size(); ++i) {
        Glyph& prev = merged.back();
        const Glyph& cur = glyphs[i];
        const int gap = cur.box.x - (prev.box.x + prev.box.w);
        const int avg_h = (prev.box.h + cur.box.h) / 2;
        const int x_overlap = std::min(prev.box.x + prev.box.w, cur.box.x + cur.box.w)
            - std::max(prev.box.x, cur.box.x);
        const bool same_row = std::abs(prev.box.y - cur.box.y) <= std::max(4, avg_h / 2);
        if (same_row && (gap <= std::max(6, prev.box.w / 4) || x_overlap > 0)) {
            merge_glyph_pair(prev, cur);
        } else {
            merged.push_back(cur);
        }
    }
    return merged;
}

std::vector<Glyph> collapse_narrow_cluster(std::vector<Glyph> glyphs, int line_w) {
    if (glyphs.size() < 2) {
        return glyphs;
    }
    // Synthetic single-character lines (fixed-width renderer) stay narrow.
    if (line_w > 0 && line_w <= 250) {
        Glyph out = glyphs.front();
        for (size_t i = 1; i < glyphs.size(); ++i) {
            merge_glyph_pair(out, glyphs[i]);
        }
        return {out};
    }
    if (line_w <= 0) {
        return glyphs;
    }
    int minx = glyphs.front().box.x;
    int maxx = glyphs.back().box.x + glyphs.back().box.w;
    for (const auto& g : glyphs) {
        minx = std::min(minx, g.box.x);
        maxx = std::max(maxx, g.box.x + g.box.w);
    }
    if (maxx - minx > line_w * 3 / 4) {
        return glyphs;
    }
    Glyph out = glyphs.front();
    for (size_t i = 1; i < glyphs.size(); ++i) {
        merge_glyph_pair(out, glyphs[i]);
    }
    return {out};
}

} // namespace

bool pack_is_arabic(const std::string& pack_id) {
    return pack_id == "arabic";
}

std::vector<Glyph> segment_arabic_glyphs(const Image& line_binary) {
    auto glyphs = segment_glyphs(line_binary);
    glyphs = merge_touching_arabic_glyphs(glyphs);
    glyphs = collapse_narrow_cluster(std::move(glyphs), line_binary.width);
    if (!glyphs.empty() && looks_like_fixed_width_cells(glyphs)) {
        return glyphs;
    }

    const auto comps = extract_components(line_binary);
    if (comps.empty()) {
        return glyphs;
    }

    int line_h = 0;
    for (const auto& c : comps) {
        line_h = std::max(line_h, c.maxy - c.miny + 1);
    }
    return cluster_baseline_glyphs(comps, line_binary, line_h);
}

} // namespace clang_ldl
