#ifndef CLANG_LDL_API_H
#define CLANG_LDL_API_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>

#define CLANG_LDL_OK 0
#define CLANG_LDL_ERR_LOAD -1
#define CLANG_LDL_ERR_PROCESS -2
#define CLANG_LDL_ERR_ALLOC -3
#define CLANG_LDL_ERR_NULL -4

typedef struct ClangLdlResult {
    char* text;
    size_t text_len;
    float mean_confidence;
    int glyph_count;
} ClangLdlResult;

/** Extract Unicode text from an image file path. Caller must free with clang_ldl_free_result. */
int clang_ldl_extract_text(const char* image_path, ClangLdlResult* out);

/** Extract text from raw RGB or grayscale bytes (row-major). */
int clang_ldl_extract_text_from_bytes(
    const unsigned char* data,
    int width,
    int height,
    int channels,
    ClangLdlResult* out);

void clang_ldl_free_result(ClangLdlResult* result);

const char* clang_ldl_version(void);

#ifdef __cplusplus
}
#endif

#endif /* CLANG_LDL_API_H */
