#include "clang_ldl/struct_features.hpp"

#include <algorithm>
#include <cmath>
#include <vector>

namespace clang_ldl {

namespace {

int holes_count(const uint8_t* bitmap, int width, int height) {
    const int pw = width + 2;
    const int ph = height + 2;
    std::vector<uint8_t> grid(static_cast<size_t>(pw) * ph, 0);
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            if (bitmap[static_cast<size_t>(y) * width + x] > 127) {
                grid[static_cast<size_t>(y + 1) * pw + (x + 1)] = 1;
            }
        }
    }
    std::vector<uint8_t> visited(grid.size(), 0);
    int holes = 0;
    auto idx = [pw](int x, int y) { return static_cast<size_t>(y) * pw + x; };

    for (int y = 0; y < ph; ++y) {
        for (int x = 0; x < pw; ++x) {
            if (grid[idx(x, y)] != 0 || visited[idx(x, y)]) {
                continue;
            }
            bool touches_border = false;
            std::vector<std::pair<int, int>> stack = {{x, y}};
            while (!stack.empty()) {
                auto [cx, cy] = stack.back();
                stack.pop_back();
                const size_t i = idx(cx, cy);
                if (visited[i] || grid[i] != 0) {
                    continue;
                }
                visited[i] = 1;
                if (cx == 0 || cy == 0 || cx == pw - 1 || cy == ph - 1) {
                    touches_border = true;
                }
                const int dx[4] = {1, -1, 0, 0};
                const int dy[4] = {0, 0, 1, -1};
                for (int k = 0; k < 4; ++k) {
                    const int nx = cx + dx[k];
                    const int ny = cy + dy[k];
                    if (nx >= 0 && ny >= 0 && nx < pw && ny < ph) {
                        stack.emplace_back(nx, ny);
                    }
                }
            }
            if (!touches_border) {
                ++holes;
            }
        }
    }
    return holes;
}

void skeleton_pass(std::vector<uint8_t>& src, int width, int height, bool endpoints) {
    std::vector<uint8_t> dst = src;
    for (int y = 1; y < height - 1; ++y) {
        for (int x = 1; x < width - 1; ++x) {
            if (src[static_cast<size_t>(y) * width + x] <= 127) {
                continue;
            }
            int n = 0;
            for (int dy = -1; dy <= 1; ++dy) {
                for (int dx = -1; dx <= 1; ++dx) {
                    if (dx == 0 && dy == 0) {
                        continue;
                    }
                    if (src[static_cast<size_t>(y + dy) * width + (x + dx)] > 127) {
                        ++n;
                    }
                }
            }
            if (endpoints ? n < 2 : n == 4) {
                dst[static_cast<size_t>(y) * width + x] = 0;
            }
        }
    }
    src.swap(dst);
}

void skeleton_stats(const uint8_t* bitmap, int width, int height, uint8_t& endpoints, uint8_t& junctions) {
    std::vector<uint8_t> skel(bitmap, bitmap + static_cast<size_t>(width) * height);
    bool changed = true;
    int iter = 0;
    while (changed && iter < 32) {
        changed = false;
        const auto before = skel;
        skeleton_pass(skel, width, height, true);
        skeleton_pass(skel, width, height, false);
        if (skel != before) {
            changed = true;
        }
        ++iter;
    }
    int ep = 0;
    int jn = 0;
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            if (skel[static_cast<size_t>(y) * width + x] <= 127) {
                continue;
            }
            int n = 0;
            for (int dy = -1; dy <= 1; ++dy) {
                for (int dx = -1; dx <= 1; ++dx) {
                    if (dx == 0 && dy == 0) {
                        continue;
                    }
                    const int nx = x + dx;
                    const int ny = y + dy;
                    if (nx < 0 || ny < 0 || nx >= width || ny >= height) {
                        continue;
                    }
                    if (skel[static_cast<size_t>(ny) * width + nx] > 127) {
                        ++n;
                    }
                }
            }
            if (n <= 1) {
                ++ep;
            } else if (n >= 3) {
                ++jn;
            }
        }
    }
    endpoints = static_cast<uint8_t>(std::min(ep, 255));
    junctions = static_cast<uint8_t>(std::min(jn, 255));
}

} // namespace

StructFeatures compute_struct_features(const uint8_t* bitmap, int width, int height) {
    StructFeatures f;
    if (!bitmap || width <= 0 || height <= 0) {
        return f;
    }
    int minx = width, miny = height, maxx = 0, maxy = 0;
    int ink = 0;
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            if (bitmap[static_cast<size_t>(y) * width + x] > 127) {
                ++ink;
                minx = std::min(minx, x);
                maxx = std::max(maxx, x);
                miny = std::min(miny, y);
                maxy = std::max(maxy, y);
            }
        }
    }
    if (ink == 0) {
        return f;
    }
    const int bw = maxx - minx + 1;
    const int bh = maxy - miny + 1;
    f.aspect = static_cast<float>(bw) / static_cast<float>(std::max(bh, 1));
    f.holes = static_cast<uint8_t>(std::min(holes_count(bitmap, width, height), 255));
    skeleton_stats(bitmap, width, height, f.endpoints, f.junctions);
    return f;
}

} // namespace clang_ldl
