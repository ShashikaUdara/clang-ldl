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
    float ncc_score = 0.f;
    float struct_score = 0.f;
    std::string pack_id;
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
    Image sauvola_threshold(int window = 15, float k = 0.5f, float R = 128.f) const;
    Image invert() const;
    Image deskew(float max_degrees = 15.f) const;
    Image rotate(float radians) const;

    uint8_t at(int x, int y) const;
    void set(int x, int y, uint8_t v);
};

} // namespace clang_ldl
