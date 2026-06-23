#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace clang_ldl {

struct Rect {
    int x = 0;
    int y = 0;
    int w = 0;
    int h = 0;
};

struct Glyph {
    Rect box;
    std::vector<uint8_t> bitmap; // row-major 0/255, size = box.w * box.h
    uint32_t codepoint = 0;
    float confidence = 0.f;
};

struct Image {
    int width = 0;
    int height = 0;
    int channels = 1;
    std::vector<uint8_t> pixels; // grayscale or single channel binary

    static Image load_file(const std::string& path);
    static Image from_bytes(const uint8_t* data, int w, int h, int channels);

    Image to_grayscale() const;
    Image gaussian_blur(int radius = 1) const;
    Image otsu_threshold() const;
    Image invert() const;

    uint8_t at(int x, int y) const;
    void set(int x, int y, uint8_t v);
};

} // namespace clang_ldl
