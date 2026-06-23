#include "clang_ldl/segment_indic.hpp"

#include "clang_ldl/pack_loader.hpp"
#include "clang_ldl/pipeline.hpp"

#include <algorithm>
#include <cmath>
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

struct ShirorekhaBand {
    int y0;
    int y1;
};

ShirorekhaBand detect_shirorekha(const Image& line_binary) {
    const int w = line_binary.width;
    const int h = line_binary.height;
    if (w <= 0 || h <= 0) {
        return {0, 0};
    }
    const int scan_h = std::max(4, h / 3);
    int best_y = 0;
    int best_count = 0;
    for (int y = 0; y < scan_h; ++y) {
        int count = 0;
        for (int x = 0; x < w; ++x) {
            if (line_binary.at(x, y) > 127) {
                ++count;
            }
        }
        if (count > best_count) {
            best_count = count;
            best_y = y;
        }
    }
    const int band_h = std::max(2, h / 16);
    return {std::max(0, best_y - 1), std::min(h - 1, best_y + band_h)};
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
        if (c.count < min_pixels || gw < 2 || gh < 3 || gw > w * 9 / 10) {
            continue;
        }
        filtered.push_back(c);
    }
    std::sort(filtered.begin(), filtered.end(), [](const Component& a, const Component& b) {
        if (a.minx != b.minx) {
            return a.minx < b.minx;
        }
        return a.miny < b.miny;
    });
    return filtered;
}

bool touches_shirorekha(const Component& c, const ShirorekhaBand& band) {
    return c.miny <= band.y1 && c.maxy >= band.y0;
}

bool is_matra_like(const Component& c, int line_h) {
    if (line_h <= 0) {
        return false;
    }
    const int gh = c.maxy - c.miny + 1;
    const int gw = c.maxx - c.minx + 1;
    return gh <= std::max(4, line_h / 4) || gw <= std::max(3, gh / 2);
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

std::vector<Glyph> cluster_components_to_aksharas(
    const std::vector<Component>& comps, const Image& line, const ShirorekhaBand& band) {
    if (comps.empty()) {
        return {};
    }
    int line_h = 0;
    for (const auto& c : comps) {
        line_h = std::max(line_h, c.maxy - c.miny + 1);
    }

    std::vector<Glyph> glyphs;
    glyphs.push_back(component_to_glyph(line, comps.front()));
    for (size_t i = 1; i < comps.size(); ++i) {
        const Component& cur_c = comps[i];
        Glyph& prev = glyphs.back();
        const int gap = cur_c.minx - (prev.box.x + prev.box.w);
        const int x_overlap = std::min(prev.box.x + prev.box.w, cur_c.maxx + 1)
            - std::max(prev.box.x, cur_c.minx);
        const bool headline_bridge = touches_shirorekha(cur_c, band)
            && (gap <= std::max(6, prev.box.w / 3) || x_overlap > 0);
        const bool matra_attach = is_matra_like(cur_c, line_h)
            && (gap <= 4 || x_overlap > 0)
            && std::abs(prev.box.y - cur_c.miny) <= std::max(prev.box.h, cur_c.maxy - cur_c.miny + 1);
        const bool same_row_touch = gap <= std::max(2, prev.box.w / 8)
            && std::abs(prev.box.y - cur_c.miny) <= std::max(prev.box.h, line_h) / 2;
        if (headline_bridge || matra_attach || same_row_touch) {
            merge_glyph_pair(prev, component_to_glyph(line, cur_c));
        } else {
            glyphs.push_back(component_to_glyph(line, cur_c));
        }
    }
    return glyphs;
}

std::vector<Glyph> split_wide_conjuncts(std::vector<Glyph> glyphs) {
    if (glyphs.size() < 2) {
        return glyphs;
    }
    std::vector<int> widths;
    widths.reserve(glyphs.size());
    for (const auto& g : glyphs) {
        widths.push_back(g.box.w);
    }
    std::sort(widths.begin(), widths.end());
    const int median_w = std::max(8, widths[widths.size() / 2]);
    const int split_threshold = static_cast<int>(std::lround(median_w * 1.8));

    std::vector<Glyph> out;
    out.reserve(glyphs.size());
    for (const auto& g : glyphs) {
        if (g.box.w < split_threshold) {
            out.push_back(g);
            continue;
        }
        std::vector<int> col_sum(g.box.w, 0);
        for (int yy = 0; yy < g.box.h; ++yy) {
            for (int xx = 0; xx < g.box.w; ++xx) {
                if (g.bitmap[static_cast<size_t>(yy) * g.box.w + xx] > 127) {
                    ++col_sum[xx];
                }
            }
        }
        const int margin = std::max(2, g.box.w / 8);
        int best_x = -1;
        int best_val = g.box.h + 1;
        for (int xx = margin; xx < g.box.w - margin; ++xx) {
            if (col_sum[xx] < best_val) {
                best_val = col_sum[xx];
                best_x = xx;
            }
        }
        if (best_x < 0 || best_val > g.box.h / 3) {
            out.push_back(g);
            continue;
        }
        auto make_part = [&](int x0, int x1) {
            Glyph part;
            part.box = {g.box.x + x0, g.box.y, x1 - x0, g.box.h};
            part.bitmap.resize(static_cast<size_t>(part.box.w) * part.box.h);
            for (int yy = 0; yy < part.box.h; ++yy) {
                for (int xx = 0; xx < part.box.w; ++xx) {
                    part.bitmap[static_cast<size_t>(yy) * part.box.w + xx] =
                        g.bitmap[static_cast<size_t>(yy) * g.box.w + (x0 + xx)];
                }
            }
            return part;
        };
        Glyph left = make_part(0, best_x);
        Glyph right = make_part(best_x, g.box.w);
        if (left.box.w >= 3) {
            out.push_back(std::move(left));
        }
        if (right.box.w >= 3) {
            out.push_back(std::move(right));
        }
    }
    return out;
}

} // namespace

bool pack_is_indic(const std::string& pack_id) {
    for (const std::string& id : indic_pack_ids()) {
        if (id == pack_id) {
            return true;
        }
    }
    return false;
}

std::vector<Glyph> segment_indic_glyphs(const Image& line_binary) {
    auto glyphs = segment_glyphs(line_binary);
    if (!glyphs.empty() && looks_like_fixed_width_cells(glyphs)) {
        const bool lone_oversized =
            line_binary.height > 60
            && glyphs.size() == 1
            && glyphs.front().box.w > line_binary.width * 6 / 10;
        if (!lone_oversized) {
            return glyphs;
        }
    }

    const ShirorekhaBand band = detect_shirorekha(line_binary);
    const auto comps = extract_components(line_binary);
    if (comps.empty()) {
        return glyphs;
    }

    glyphs = cluster_components_to_aksharas(comps, line_binary, band);
    glyphs = split_wide_conjuncts(std::move(glyphs));
    return glyphs;
}

} // namespace clang_ldl
